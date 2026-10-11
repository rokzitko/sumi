# Command-line parsing and enum checks

Build the tools and run the focused checks from the repository root:

```sh
make all
make -C test_cli check
```

The checks require a C++20 compiler and Python 3.7 or later (standard library
only). FFTW is needed by `sumi`. For Apple Clang:

```sh
make all CXX=clang++ \
  CXXFLAGS="-std=c++20 -O3 -Werror -Wall -Wextra -Wshadow -Wpedantic $(pkg-config --cflags fftw3)" \
  FFTW="$(pkg-config --libs fftw3)"
make -C test_cli check CXX=clang++
```

The C++ parser checks are independent of FFTW. The CLI checks supply binary
input through stdin and capture output in memory; the only file generated in
this directory is `check.out`.

## Argument rules

- Each tool declares its value-taking options and switches. Unknown options
  and positional arguments are rejected.
- Value-taking options may appear only once, even when repeated values agree.
  Boolean switches may be repeated.
- A value-taking option must be followed by a value, rather than another known
  option. A filename matching an option name can be written as a path such as
  `./-v`. Explicit `-i ""` retains the stdin convention.
- Signed integers must fit `int`; counts and sizes must fit `uint64_t` and have
  no minus sign. Numeric conversions must consume the complete value string.
- Real arguments must convert to finite doubles. Scientific notation, signed
  values, and negative zero are supported.
- Each tool checks its accepted noise/output/filter/direction codes and input
  word sizes before formatting diagnostics or processing data.
- Parsing and validation failures return status 1 with an `Error:` diagnostic.

These rules concern argument syntax, representation, and supported enum codes.

## Coverage

`check.cc` exercises the shared parser with defaults, integer endpoints,
scientific notation, filenames, repeated switches, malformed values, overflow,
non-finite numbers, and unsupported choices. It also checks that enum-name
helpers reject invalid values through exceptions.

`check.py` checks all declared value-taking options in each executable for
missing/repeated/malformed values, verifies option-specific diagnostics, and
checks unsupported codes both with and without `-v`. Valid repeated switches
and explicit stdin selection are compared with their ordinary forms.

To test particular executables:

```sh
python3 test_cli/check.py --sumi ./sumi --bitdump ./bitdump --bitent ./bitent
```
