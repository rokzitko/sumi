"""Phase statistics and block-aligned spectra of the actual sumi CLI output."""

import argparse
from pathlib import Path
import subprocess

import numpy as np


BS = 1024
BLOCKS = 4000
PHASE_TOL = 0.12
POWER_TOL = 0.05
SPECTRUM_TOL = 0.12


def prediction(ap, aw):
    """Exact second moments for the existing FFT-coefficient convention."""
    half = BS // 2
    f = np.arange(1, half + 1)
    s = ap / f + aw / BS
    power = np.zeros(BS)
    power[1:half] = s[:-1]
    power[half + 1:] = s[:-1][::-1]
    power[half] = s[-1] / 2  # Real Nyquist coefficient in timmer_konig_setup.
    variance = power.sum()
    covariance1 = np.dot(power, np.cos(2 * np.pi * np.arange(BS) / BS))
    step_variance = 2 * (variance - covariance1 * np.cos(np.pi / BS))

    # A block-aligned output segment contains one full source block and parts
    # of two independent, half-offset blocks. Their expected powers add.
    phase = np.arange(BS)
    w1 = np.sin(np.pi * phase / BS)
    w2 = np.sin(np.pi * ((phase + half) % BS) / BS)
    windows = (w1, np.where(phase < half, w2, 0), np.where(phase >= half, w2, 0))
    kernel = sum(np.abs(np.fft.fft(w)) ** 2 for w in windows)
    spectrum = np.fft.ifft(np.fft.fft(power) * np.fft.fft(kernel)).real / BS
    return variance, step_variance, spectrum[:half + 1], BS * power[:half + 1]


def check(sumi, name, ap, aw):
    command = [str(sumi), "-n", "120", "-ap", str(ap), "-aw", str(aw),
               "-b", str(BS), "-c", str(BS * BLOCKS), "-t", "-ot", "-1"]
    result = subprocess.run(command, check=True, stdout=subprocess.PIPE)
    samples = np.fromstring(result.stdout.decode("ascii"), sep=" ")
    if samples.size != BS * BLOCKS or not np.isfinite(samples).all():
        raise RuntimeError(f"{name}: invalid sample count or non-finite output")
    del result
    y = samples.reshape(BLOCKS, BS)
    variance, step_variance, expected_spectrum, target = prediction(ap, aw)
    phase_variance = y.var(axis=0, ddof=1) / variance
    total_variance = y.var() / variance
    delta = np.empty((BLOCKS - 1, BS))
    delta[:, 0] = y[1:, 0] - y[:-1, -1]
    delta[:, 1:] = y[1:, 1:] - y[1:, :-1]
    phase_steps = delta.var(axis=0, ddof=1) / step_variance
    half = BS // 2
    ordinary = np.ones(BS, dtype=bool)
    ordinary[[0, half]] = False
    join_ratios = phase_steps[[0, half]] / phase_steps[ordinary].mean()
    print(f"{name}: total variance / predicted = {total_variance:.4f}")
    print(f"  phase variance / predicted: {phase_variance.min():.4f} .. {phase_variance.max():.4f}")
    print(f"  step variance / predicted:  {phase_steps.min():.4f} .. {phase_steps.max():.4f}")
    print(f"  join / ordinary step variance: {join_ratios[0]:.4f}, {join_ratios[1]:.4f}")

    errors = []
    if abs(total_variance - 1) > POWER_TOL:
        errors.append("total variance")
    if np.max(np.abs(phase_variance - 1)) > PHASE_TOL:
        errors.append("phase variance")
    if np.max(np.abs(phase_steps - 1)) > PHASE_TOL:
        errors.append("phase first-difference variance")
    if np.max(np.abs(join_ratios - 1)) > PHASE_TOL:
        errors.append("boundary first-difference variance")

    spectrum = (np.abs(np.fft.rfft(y, axis=1)) ** 2).mean(axis=0) / BS
    edges = [0, 1, 2, 4, 8, 16, 32, 64, 128, 256, half, half + 1]
    for lo, hi in zip(edges[:-1], edges[1:]):
        ratio = spectrum[lo:hi].sum() / expected_spectrum[lo:hi].sum()
        print(f"  spectral bins [{lo:3d}, {hi:3d}): measured / predicted = {ratio:.4f}")
        if abs(ratio - 1) > SPECTRUM_TOL:
            errors.append(f"spectral bins [{lo}, {hi})")
    # This is a diagnostic, not a demand for exact preservation of the input
    # spectrum. DC has zero input power but nonzero windowed output power.
    ratios = expected_spectrum[1:5] / target[1:5]
    print("  predicted / unwindowed spectrum, bins 1..4:",
          ", ".join(f"{r:.4f}" for r in ratios))
    print(f"  predicted DC periodogram = {expected_spectrum[0]:.6g}")
    for error in errors:
        print(f"FAIL: {name}: {error}")
    return not errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sumi", type=Path, default=Path(__file__).resolve().parent.parent / "sumi")
    args = parser.parse_args()
    print(f"bs={BS}, blocks={BLOCKS}, seed=1234 (-t); "
          f"tolerances: phases={PHASE_TOL}, power={POWER_TOL}, spectra={SPECTRUM_TOL}", flush=True)
    passed = [check(args.sumi.resolve(), name, ap, aw) for name, ap, aw in
              (("pink", 1.0, 0.0), ("white", 0.0, 1.0), ("mixed", 0.1, 1.0))]
    if not all(passed):
        raise SystemExit(1)
    print("PASS: phase statistics and spectra")


if __name__ == "__main__":
    main()
