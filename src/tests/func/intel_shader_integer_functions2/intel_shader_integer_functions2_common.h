// Copyright 2019 Intel Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice (including the next
// paragraph) shall be included in all copies or substantial portions of the
// Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

#ifndef INTEL_SHADER_INTEGER_FUNCTIONS2_COMMON_H
#define INTEL_SHADER_INTEGER_FUNCTIONS2_COMMON_H

#define SUM_N_to_1(x)  (((x + 1) * x) / 2)
#define ARRAY_SIZE(x)  (sizeof(x) / sizeof(x[0]))

typedef void (*results_cb)(void *, unsigned, const void *, unsigned, unsigned);

void generate_results_commutative_no_diagonal(void *dest, const void *src_data,
                                              unsigned num_srcs, results_cb f);

void generate_results_commutative(void *dest, const void *src_data,
                                  unsigned num_srcs, results_cb f);

void run_integer_functions2_test(const QoShaderModuleCreateInfo *fs_info,
                                 const void *src, unsigned src_size,
                                 const void *expected, unsigned expected_size);

#endif /* INTEL_SHADER_INTEGER_FUNCTIONS2_COMMON_H */
