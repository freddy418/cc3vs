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
#include <stdio.h>
#include <stdlib.h>

#include <limits.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdarg.h>

#include "include/bitset.h"

#define BITSET_BITS \
	( CHAR_BIT * sizeof(size_t) )

#define BITSET_MASK(pos) \
	( ((size_t)1) << ((pos) % BITSET_BITS) )

#define BITSET_WORD(set, pos) \
	( (set)->bits[(pos) / BITSET_BITS] )

#define BITSET_USED(nbits) \
	( ((nbits) + (BITSET_BITS - 1)) / BITSET_BITS )

#define MIN(a, b) \
        (a<b)?a:b

#define MAX(a, b) \
        (a<b)?a:b

#define HMASK 15

bitset2 *bitset_init(size_t nbits) {
  bitset2 *set = (bitset2 *) malloc(sizeof(bitset2));
  
  assert(set);
  
#ifdef RANGE
  set->base = 0;
  set->bound = 0;
  set->d = 0;
#else
  set->bits = (size_t *) calloc(BITSET_USED(nbits), sizeof(size_t));
  set->nbits = nbits;
  assert(set->bits);
#endif
  
  return set;
}

bitset2 *bitset_copy(const bitset2 *set) {
  bitset2 *clone;

#ifdef RANGE
  clone = bitset_init(256);
  clone->base = set->base;
  clone->bound = set->bound;
  clone->d = set->d;
#else
  clone = bitset_init(set->nbits);  
  memcpy(clone->bits, set->bits, BITSET_USED(set->nbits) * sizeof(*set->bits));
#endif

  return clone;
}

void bitset_reset(bitset2 *set) {
#ifdef RANGE
  set->base = 0;
  set->bound = 0;
  set->d = 0;
#else
  memset(set->bits, 0, BITSET_USED(set->nbits) * sizeof(*set->bits));
#endif
}

void bitset_free(bitset2 *set) {
#ifndef RANGE
  free(set->bits);
#endif
  free(set);
}

size_t bitset_size(const bitset2 *set) {
#ifdef RANGE
  return 0;
#else
  return set->nbits;
#endif
}

void bitset_clear_bit(bitset2 *set, size_t pos) {
#ifdef RANGE
  if (pos == set->base && pos == set->bound){
    set->base = 0;
    set->bound = 0;
  }
  else if (set->d == 1){
    if (pos == set->base){
      set->base = set->bound;
    }
    else if (pos == set->bound){
      set->bound = set->base;
    }
    set->d = 0;
  } // else do nothing
#else
  assert(pos < set->nbits);
  
  BITSET_WORD(set, pos) &= ~BITSET_MASK(pos);
#endif
}

void bitset_set_bit(bitset2 *set, size_t pos) {
#ifdef RANGE
  if (set->base == 0 && set->bound == 0){
    set->base = pos;
    set->bound = pos;
  }
  else if (set->d == 0){
    set->d = 1;
    if (set->base > pos){
      set->base = pos;
    }
    else if (set->bound < pos){
      set->bound = pos;
    }
  }
  else if (set->d == 1){
    if (set->base > pos){
      set->base = pos;
    }
    else if (set->bound < pos){
      set->bound = pos;
    }
    set->d = 2;
    // else it's already in the range
  }
#else
  assert(pos < set->nbits);
  
  BITSET_WORD(set, pos) |= BITSET_MASK(pos);
#endif
}

void bitset_set_bits(bitset2 *dest, const bitset2 *src) {
#ifdef RANGE
  if (dest->base == 0 && dest->bound == 0){
    dest->base = src->base;
    dest->bound = src->bound;
    dest->d = src->d;
  }
  else if (src->base != 0){
    dest->d = 2;
    if (dest->base > src->base){
      dest->base = src->base;
    }
    else if (dest->bound < src->bound){
      dest->bound = src->bound;
    }
  }
  else {
    // src is somewhere within dest already
    dest->d = 2;
  }
#else
  assert(dest->nbits >= src->nbits);
  memcpy(dest->bits, src->bits, BITSET_USED(src->nbits) * sizeof(*src->bits));
#endif
}

bool bitset_test_bit(const bitset2 *set, size_t pos) {
#ifdef RANGE
  return ((set->base <= pos) && (set->bound >= pos));
#else
  //printf("pos %d, nbits %d\n", pos, set->nbits);
  assert(pos < set->nbits); 
  return (BITSET_WORD(set, pos) & BITSET_MASK(pos)) != 0;
#endif
}

void bitset_toggle_bit(bitset2 *set, size_t pos) {
#ifdef RANGE
  if (pos < set->base){
    set->base = pos;
    if (set->d = 0){
      set->d = 1;
    }
  }
  else if (pos > set->bound){
    set->bound = pos;
    if (set->d = 0){
      set->d = 1;
    }
  }
  else if (pos == set->base && set->d == 1){
    set->base = set->bound;
    set->d = 0;
  }
  else if (pos = set->bound && set->d == 1){
    set->bound = set->base;
    set->d = 0;
  }
  // else do nothing?
#else
  assert(pos < set->nbits);
  
  if(bitset_test_bit(set, pos)) {
    bitset_clear_bit(set, pos);
  }
  else {
    bitset_set_bit(set, pos);
  }
#endif
}

void bitset_union(bitset2 *dest, const bitset2 *src) {
#ifdef RANGE
  if (dest->base == 0 || src->base == 0){
    dest->base = MIN(dest->base, src->base);
  }
  dest->bound = MAX(dest->bound, src->bound);
  dest->d |= src->d;
#else
  if(dest->nbits != src->nbits){
    printf("source(%d) and dest(%d) nbits differ\n", src->nbits, dest->nbits);
    return;
  }
  //assert(dest->nbits == src->nbits);
  
  int i = BITSET_USED(dest->nbits);

  while(i--){
    dest->bits[i] |= src->bits[i];
  }
#endif
}

void bitset_union_n(bitset2 *dest, ...) {
#ifdef RANGE
  // TODO
#else
  va_list ap;
  bitset2 *src;

  va_start(ap, dest);

  while((src = va_arg(ap, bitset2 *)) != NULL) {
    bitset_union(dest, src);
  }

  va_end(ap);
#endif
} 

void bitset_intersection(bitset2 *dest, const bitset2 *src) {
#ifdef RANGE
  // TODO
#else
  assert(dest->nbits == src->nbits);

  int i = BITSET_USED(dest->nbits);
  
  while(i--)
    dest->bits[i] &= src->bits[i];
#endif
}

void bitset_difference(bitset2 *dest, const bitset2 *src) {
#ifdef RANGE
  // TODO
#else
  assert(dest->nbits == src->nbits);

  int i = BITSET_USED(dest->nbits);
  
  while(i--)
    dest->bits[i] &= ~src->bits[i];
#endif
}

void bitset_xor(bitset2 *dest, const bitset2 * src) {
#ifdef RANGE
  // TODO
#else
  assert(dest->nbits == src->nbits);
  
  int i = BITSET_USED(dest->nbits);
  
  while(i--)
    dest->bits[i] ^= src->bits[i];
#endif
}

bool bitset_equal(const bitset2 *a, const bitset2 *b) {
#ifdef RANGE
  return ((a->base == b->base) && (b->bound == b->bound));
#else
  if(a->nbits != b->nbits) {
    return false;
  }

  int i = BITSET_USED(a->nbits);
  
  while(i-- && (a->bits[i] == b->bits[i]))
    ;

  return i == -1;
#endif
}

bool bitset_is_subset(const bitset2 *a, const bitset2 *b) {
#ifdef RANGE
  return ((a->base <= b->base) && (a->bound >= b->bound));
#else
  assert(a->nbits == b->nbits);

  int i = BITSET_USED(a->nbits);
  
  while(i-- && ((a->bits[i] | b->bits[i]) == a->bits[i]));
    ;

  return i == -1;
#endif
}

bool bitset_is_empty(const bitset2 *set) {
#ifdef RANGE
  return ((set->base == 0) && (set->bound == 0));
#else
  int i = BITSET_USED(set->nbits);
  
  while(i-- && !set->bits[i])
    ;

  return i == -1;
#endif
}

size_t bitset_population(const bitset2 * s) {
#ifdef RANGE
  if ((s->base == 0) && (s->bound == 0)){
    return 0;
  }
  else if (s->base == s->bound){
    return 1;
  }
  else {
    return 2;
  }
#else
  if (s != NULL){
    size_t total = 0;
    
    unsigned int i;
    for(i = 0; i < s->nbits; i++) {
      if(bitset_test_bit(s, i)) total++;
    }

    return total;
  }else{
    return 0;
  }
#endif
}

void bitset_print(FILE *f, const bitset2 *set) {
#ifdef RANGE
  printf("%X%08X\n", set->base, set->bound);
#else
  unsigned int i;
  for(i = 0; i < set->nbits; i++) {
    printf("%d", bitset_test_bit(set, i));    
  }
  printf("\n");
#endif
}

char * bitset_str(const bitset2 *set) {

#ifdef RANGE
  char * buf = (char *)malloc(16 * sizeof(char));			      
  if (buf == NULL) return "0";
  sprintf(buf, "%X%08X", set->base, set->bound);
  return buf;
#else
  unsigned int i;
  char *buf = (char *) malloc((set->nbits) + 1);

  if (buf == NULL) return '\0';

  buf[0] = '\0';

  for(i = 0; i < set->nbits; i++) {
    if(bitset_test_bit(set, i)) {
      strncat(buf, "1", 1);
    }
    else {
      strncat(buf, "0", 1);
    }
  }
  
  return buf;
#endif
}

std::string bitset_tostring(const bitset2 *set){
  std::string buf = "";
  char * hv = (char *)malloc(16 * sizeof(char));			      
  if (hv == NULL) return '\0';

#ifdef RANGE

  if (set != NULL){
    sprintf(hv, "%X%08X", set->base, set->bound);
    return std::string(hv);
  }else{
    return std::string();
  }

#else
  unsigned int i;
  unsigned int word_size = CHAR_BIT * sizeof(size_t);
  unsigned int word; 
  unsigned int begin, end;
  std::string buf = "";
  char hv[32];

  /*for(i = 0; i < set->nbits; i+=word_size) {
    char hv[32];
    word = BITSET_WORD(set, i);
    sprintf(hv, "%08X", word);
    buf = std::string(hv) + buf;
    }*/
  if (bitset_is_empty(set)){
    buf += std::string("0");
  }else{
    for(i = 0; i < set->nbits; i++) {
      if(bitset_test_bit(set, i)) {
	begin = i;
	break;
      }
    }
    for(i = set->nbits-1; i >= 0; i--){
      if (bitset_test_bit(set, i)){
	end = i;
	break;
      }
    }
    sprintf(hv, "%X%08X", begin, end);
    buf += std::string(hv);
  }

  return buf;
#endif
}
