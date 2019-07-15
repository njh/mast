/*

  mast-assert.h

  MAST: Multicast Audio Streaming Toolkit
  Copyright (C) 2019  Nicholas Humfrey
  License: MIT

*/

#ifndef MAST_ASSERT_H
#define MAST_ASSERT_H

// Travis-CI doesn't currently have this
#ifndef ck_assert_mem_eq
#define ck_assert_mem_eq(X, Y, L) ck_assert(memcmp(X, Y, L) == 0)
#endif

// Assert that two floating point numbers are the same to 3 decimal places
#define mast_assert_float_eq_3dp(X, Y) do { \
  float _ck_x = (X); \
  float _ck_y = (Y); \
  ck_assert_msg((int)(_ck_x*1000) == (int)(_ck_y*1000), "Assertion '%s' failed: %s == %f, %s == %f", #X" == "#Y, #X, _ck_x, #Y, _ck_y); \
} while (0)

#endif
