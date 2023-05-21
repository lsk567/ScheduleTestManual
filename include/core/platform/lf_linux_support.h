/* Linux API support for the C target of Lingua Franca. */

/*************
Copyright (c) 2021, The University of California at Berkeley.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************/

/** Linux API support for the C target of Lingua Franca.
 *
 *  @author{Soroush Bateni <soroush@utdallas.edu>}
 */

#ifndef LF_LINUX_SUPPORT_H
#define LF_LINUX_SUPPORT_H

#if defined LF_THREADED || defined _LF_TRACE
    #if __STDC_VERSION__ < 201112L || defined (__STDC_NO_THREADS__)
// (Not C++11 or later) or no threads support
#include "lf_POSIX_threads_support.h"
    #else
#include "lf_C11_threads_support.h"
    #endif
#endif

#include <stdint.h> // For fixed-width integral types
#include <time.h>   // For CLOCK_MONOTONIC

// Use 64-bit times and 32-bit unsigned microsteps
#include "lf_tag_64_32.h"

// The underlying physical clock for Linux
// FIXME: This is a problem. When the program first starts,
// the value reported by CLOCK_MONOTONIC (and lf_clock_gettime()) is ~24034861235297147.
// Then _lf_time_epoch_offset is updated to 1660079548929325548,
// which brings the next lf_clock_gettime() value up to 1660079548929325548.
// This huge gap basically makes `physical_start_time` useless!
// #define _LF_CLOCK CLOCK_MONOTONIC
#define _LF_CLOCK CLOCK_REALTIME

#endif // LF_LINUX_SUPPORT_H
