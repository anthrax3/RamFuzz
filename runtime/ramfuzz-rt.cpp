// Copyright 2016-2017 The RamFuzz contributors. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "ramfuzz-rt.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <iostream>
#include <limits>

using std::cout;
using std::endl;
using std::generate;
using std::hex;
using std::isprint;
using std::istream;
using std::numeric_limits;
using std::ofstream;
using std::ranlux24;
using std::size_t;
using std::streamsize;
using std::string;
using std::vector;
using std::uniform_int_distribution;
using std::uniform_real_distribution;

namespace {

template <typename IntegralT>
IntegralT ibetween(IntegralT lo, IntegralT hi, ranlux24 &gen) {
  return uniform_int_distribution<IntegralT>{lo, hi}(gen);
}

template <typename RealT> RealT rbetween(RealT lo, RealT hi, ranlux24 &gen) {
  return uniform_real_distribution<RealT>{lo, hi}(gen);
}

/// Declares and initializes an unwind context and cursor.
#define CURSORINIT(context_var, cursor_var)                                    \
  unw_context_t context_var;                                                   \
  unw_getcontext(&context_var);                                                \
  unw_cursor_t cursor_var;                                                     \
  unw_init_local(&cursor_var, &context_var);

/// Returns the value of the PC (program counter) register inside itself.
/// Useful as an arbitrary base PC from which to calculate relative offsets of
/// all other code's PCs.
unw_word_t get_pc() {
  CURSORINIT(ctx, curs);
  unw_word_t pc;
  unw_get_reg(&curs, UNW_REG_IP, &pc);
  return pc;
}

} // anonymous namespace

namespace ramfuzz {
namespace runtime {

gen::gen(const string &ologname)
    : runmode(generate), olog(ologname), base_pc(get_pc()) {
  if (!olog)
    throw file_error("Cannot open " + ologname);
}

gen::gen(const string &ilogname, const string &ologname)
    : runmode(replay), olog(ologname), ilog(ilogname), base_pc(get_pc()) {
  if (!olog)
    throw file_error("Cannot open " + ologname);
  if (!ilog)
    throw file_error("Cannot open " + ilogname);
}

gen::gen(int argc, const char *const *argv, size_t k) : base_pc(get_pc()) {
  if (k < static_cast<size_t>(argc) && argv[k]) {
    runmode = replay;
    const string argstr(argv[k]);
    ilog.open(argstr);
    if (!ilog)
      throw file_error("Cannot open " + argstr);
    olog.open(argstr + "+");
    if (!olog)
      throw file_error("Cannot open " + argstr + "+");
  } else {
    runmode = generate;
    olog.open("fuzzlog");
    if (!olog)
      throw file_error("Cannot open fuzzlog");
  }
}

size_t gen::valueid() {
  CURSORINIT(ctx, curs);
  size_t stacktrace_hash = 0; // "Stack trace" = a vector of all callers' PCs.
  while (unw_step(&curs)) {
    unw_word_t pc;
    unw_get_reg(&curs, UNW_REG_IP, &pc);
    // Cribbed from boost::hash_combine().
    stacktrace_hash ^= pc - base_pc + 0x9e3779b9 + (stacktrace_hash << 6) +
                       (stacktrace_hash >> 2);
  }
  return stacktrace_hash;
}

template <> bool gen::uniform_random<bool>(bool lo, bool hi) {
  return uniform_int_distribution<int>{lo, hi}(rgen);
}

template <> double gen::uniform_random<double>(double lo, double hi) {
  return rbetween(lo, hi, rgen);
}

template <> float gen::uniform_random<float>(float lo, float hi) {
  return rbetween(lo, hi, rgen);
}

// Depending on your C++ implementation, some of the below definitions may have
// to be commented out or uncommented.  Implementations typically define certain
// integer types to be identical (eg, size_t and unsigned long), so you can't
// keep both definitions.  The code below compiles successfully with MacOS
// clang; that requires commenting out some integer types that are identical to
// uncommented ones.  But your implementation may differ.
//
// If you get an error like "redefinition of uniform_random" for a type, just
// comment out that type's definition.  If, OTOH, you get an error like
// "undefined symbol uniform_random" for a type, then uncomment that type's
// definition.

template <> short gen::uniform_random<short>(short lo, short hi) {
  return ibetween(lo, hi, rgen);
}

template <>
unsigned short gen::uniform_random<unsigned short>(unsigned short lo,
                                                   unsigned short hi) {
  return ibetween(lo, hi, rgen);
}

template <> int gen::uniform_random<int>(int lo, int hi) {
  return ibetween(lo, hi, rgen);
}

template <> unsigned gen::uniform_random<unsigned>(unsigned lo, unsigned hi) {
  return ibetween(lo, hi, rgen);
}

template <> long gen::uniform_random<long>(long lo, long hi) {
  return ibetween(lo, hi, rgen);
}

template <>
unsigned long gen::uniform_random<unsigned long>(unsigned long lo,
                                                 unsigned long hi) {
  return ibetween(lo, hi, rgen);
}

template <>
long long gen::uniform_random<long long>(long long lo, long long hi) {
  return ibetween(lo, hi, rgen);
}

template <>
unsigned long long
gen::uniform_random<unsigned long long>(unsigned long long lo,
                                        unsigned long long hi) {
  return ibetween(lo, hi, rgen);
}
/*
template <> size_t gen::uniform_random<size_t>(size_t lo, size_t hi) {
  return ibetween(lo, hi, rgen);
}

template <> int64_t gen::uniform_random<int64_t>(int64_t lo, int64_t hi) {
  return ibetween(lo, hi, rgen);
}
*/
template <> char gen::uniform_random<char>(char lo, char hi) {
  return ibetween(lo, hi, rgen);
}

template <>
unsigned char gen::uniform_random<unsigned char>(unsigned char lo,
                                                 unsigned char hi) {
  return ibetween(lo, hi, rgen);
}

} // namespace runtime
} // namespace ramfuzz
