// Miscellenous routines

#ifndef _misc_h_
#define _misc_h_

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <coroutine>

// Simple command line parser
// https://stackoverflow.com/questions/865668/parsing-command-line-arguments-in-c
class InputParser {
 public:
   InputParser (int argc, char *argv[]) {
     for (int i = 1; i < argc; ++i)
       this->tokens.push_back(std::string(argv[i]));
   }
   std::string get(const std::string &option) const {
     auto itr = std::find(this->tokens.begin(), this->tokens.end(), option);
     if (itr != this->tokens.end() && ++itr != this->tokens.end())
       return *itr;
     return "";
   }
   bool exists(const std::string &option) const {
     return std::find(this->tokens.begin(), this->tokens.end(), option) != this->tokens.end();
   }
//   template <typename T>
//     T get(const std::string &option, const T def) const {
//       return exists(option) ? T(get(option)) : def;
//     }
   auto get_double(const std::string &option, const double def) const {
       return exists(option) ? std::stod(get(option)) : def;
     }
 private:
   std::vector<std::string> tokens;
};

// Copied from https://en.cppreference.com/w/cpp/language/coroutines
template <typename T>
struct Generator
{
  // The class name 'Generator' is our choice and it is not required for coroutine
  // magic. Compiler recognizes coroutine by the presence of 'co_yield' keyword.
  // You can use name 'MyGenerator' (or any other name) instead as long as you include
  // nested struct promise_type with 'MyGenerator get_return_object()' method.

  struct promise_type;
  using handle_type = std::coroutine_handle<promise_type>;

  struct promise_type // required
  {
    T value_;
    std::exception_ptr exception_;

    Generator get_return_object()
    {
      return Generator(handle_type::from_promise(*this));
    }
    std::suspend_always initial_suspend() { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void unhandled_exception() { exception_ = std::current_exception(); } // saving exception

    template <std::convertible_to<T> From> // C++20 concept
    std::suspend_always yield_value(From&& from)
    {
      value_ = std::forward<From>(from); // caching the result in promise
      return {};
    }
    void return_void() { }
  };

  handle_type h_;

  Generator(handle_type h)
    : h_(h)
  {
  }
  ~Generator() { h_.destroy(); }
  explicit operator bool()
  {
    fill(); // The only way to reliably find out whether or not we finished coroutine,
    // whether or not there is going to be a next value generated (co_yield)
    // in coroutine via C++ getter (operator () below) is to execute/resume
    // coroutine until the next co_yield point (or let it fall off end).
    // Then we store/cache result in promise to allow getter (operator() below
    // to grab it without executing coroutine).
    return !h_.done();
  }
  T operator()()
  {
    fill();
    full_ = false; // we are going to move out previously cached
    // result to make promise empty again
    return std::move(h_.promise().value_);
  }

private:
  bool full_ = false;

  void fill()
  {
    if (!full_)
      {
	h_();
	if (h_.promise().exception_)
	  std::rethrow_exception(h_.promise().exception_);
	// propagate coroutine exception in called context
 
	full_ = true;
      }
  }
};

// Robert Jenkins' 96 bit Mix Function
unsigned long mix(unsigned long a, unsigned long b, unsigned long c)
{
  a=a-b;  a=a-c;  a=a^(c >> 13);
  b=b-c;  b=b-a;  b=b^(a << 8);
  c=c-a;  c=c-b;  c=c^(b >> 13);
  a=a-b;  a=a-c;  a=a^(c >> 12);
  b=b-c;  b=b-a;  b=b^(a << 16);
  c=c-a;  c=c-b;  c=c^(b >> 5);
  a=a-b;  a=a-c;  a=a^(c >> 3);
  b=b-c;  b=b-a;  b=b^(a << 10);
  c=c-a;  c=c-b;  c=c^(b >> 15);
  return c;
}

bool file_exists(std::string filename)
{
  std::ifstream F(filename);
  return bool(F);
}

#endif
