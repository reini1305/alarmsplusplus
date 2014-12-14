#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef MIN
  #define MIN(a, b) ((a) < (b)) ? (a) : (b)
#endif

// Required for inlining static with -Os during pre-processor
#define FORCE_INLINE __attribute__((always_inline)) inline

// Supports strings up to 128 long by using 8 16-byte chunks
#define HASH_DJB2_128(s, len) \
  (\
    (hash_djb2(s+112, MIN((len - 112),16), \
      (hash_djb2(s+96, MIN((len - 96),16), \
        (hash_djb2(s+80, MIN((len - 80),16), \
          (hash_djb2(s+64, MIN((len - 64),16), \
            (hash_djb2(s+48, MIN((len - 48),16), \
              (hash_djb2(s+32, MIN((len - 32),16), \
                (hash_djb2(s+16, MIN((len - 16),16), \
                  (hash_djb2(s,MIN(len,16),5381)))))))))))))))) \
  )

#define HASH_DJB2_NX(s) (HASH_DJB2_128(s, (int32_t)strlen(s)) & 0x7FFFFFFF)
#define HASH_DJB2(s) HASH_DJB2_NX(s)

// GCC preprocessor will only preprocess this up to 18 characters, so only do 16
// Based on DJB2 Hash, use 5381 as initial seed
static FORCE_INLINE uint32_t hash_djb2(
    const char* bytes, const int32_t length, const uint32_t oldhash) {
  uint32_t hash = oldhash;

  if (length <= 0) return hash;

  // Manually unrolled with scope 1 to be compatible with Os
  if(length > 0) hash = ((hash << 5) + hash) + bytes[0];
  if(length > 1) hash = ((hash << 5) + hash) + bytes[1];
  if(length > 2) hash = ((hash << 5) + hash) + bytes[2];
  if(length > 3) hash = ((hash << 5) + hash) + bytes[3];
  if(length > 4) hash = ((hash << 5) + hash) + bytes[4];
  if(length > 5) hash = ((hash << 5) + hash) + bytes[5];
  if(length > 6) hash = ((hash << 5) + hash) + bytes[6];
  if(length > 7) hash = ((hash << 5) + hash) + bytes[7];
  if(length > 8) hash = ((hash << 5) + hash) + bytes[8];
  if(length > 9) hash = ((hash << 5) + hash) + bytes[9];
  if(length > 10) hash = ((hash << 5) + hash) + bytes[10];
  if(length > 11) hash = ((hash << 5) + hash) + bytes[11];
  if(length > 12) hash = ((hash << 5) + hash) + bytes[12];
  if(length > 13) hash = ((hash << 5) + hash) + bytes[13];
  if(length > 14) hash = ((hash << 5) + hash) + bytes[14];
  if(length > 15) hash = ((hash << 5) + hash) + bytes[15];

  return hash;
}

