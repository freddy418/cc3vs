/**

Copyright 2007
Georgia Tech Research Corporation
Atlanta, GA  30332-0415
All Rights Reserved

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

   * Redistributions of source code must retain the above copyright
   * notice, this list of conditions and the following disclaimer.

   * Redistributions in binary form must reproduce the above
   * copyright notice, this list of conditions and the following
   * disclaimer in the documentation and/or other materials provided
   * with the distribution.

   * Neither the name of the Goergia Tech Research Coporation nor the
   * names of its contributors may be used to endorse or promote
   * products derived from this software without specific prior
   * written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/

#ifndef _BITSET_H_
#define _BITSET_H_

#include <string>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define RANGE 1

typedef struct {
#ifdef RANGE
  int base;
  int bound;
  char d;
#else
  size_t *bits;
  size_t nbits;
#endif
} bitset2;

bitset2* bitset_init(size_t nbits);
bitset2* bitset_copy(const bitset2 *set);
void bitset_free(bitset2 *set);

void bitset_reset(bitset2 *set);

size_t bitset_size(const bitset2 *set);

void bitset_clear_bit(bitset2 *set, size_t pos);
void bitset_set_bit(bitset2 *set, size_t pos);
void bitset_set_bits(bitset2 *dest, const bitset2 *src);
bool bitset_test_bit(const bitset2 *set, size_t pos);
void bitset_toggle_bit(bitset2 *set, size_t pos);

void bitset_union(bitset2 *dest, const bitset2 *src);
void bitset_union_n(bitset2 *dest, ...);
void bitset_intersection(bitset2 *dest, const bitset2 *src);
void bitset_difference(bitset2 *dest, const bitset2 *src);
void bitset_xor(bitset2 *dest, const bitset2 * src);

bool bitset_equal(const bitset2 *a, const bitset2 *b);
bool bitset_is_subset(const bitset2 *a, const bitset2 *b);
bool bitset_is_empty(const bitset2 *set);

size_t bitset_population(const bitset2 *a);

void bitset_print(FILE *f, const bitset2 *set);
char *bitset_str(const bitset2 *set);
std::string bitset_tostring(const bitset2 *set);
#endif
