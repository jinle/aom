/*
 * Copyright (c) 2016, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#include "./aom_config.h"
#include "aom/aom_integer.h"
#include "aom_mem/aom_mem.h"
#include "av1/common/blockd.h"
#include "av1/common/entropy.h"
#include "av1/common/entropymode.h"
#include "av1/common/onyxc_int.h"
#include "av1/common/scan.h"
#include "av1/common/token_cdfs.h"
#if CONFIG_LV_MAP
#include "av1/common/txb_common.h"
#endif

/* Extra bits coded from LSB to MSB */
const aom_cdf_prob av1_cat1_cdf0[CDF_SIZE(2)] = { AOM_CDF2(20352) };
const aom_cdf_prob *av1_cat1_cdf[] = { av1_cat1_cdf0 };

const aom_cdf_prob av1_cat2_cdf0[CDF_SIZE(4)] = { AOM_CDF4(11963, 21121,
                                                           27719) };
const aom_cdf_prob *av1_cat2_cdf[] = { av1_cat2_cdf0 };
const aom_cdf_prob av1_cat3_cdf0[CDF_SIZE(8)] = { AOM_CDF8(
    7001, 12802, 17911, 22144, 25503, 28286, 30737) };
const aom_cdf_prob *av1_cat3_cdf[] = { av1_cat3_cdf0 };

const aom_cdf_prob av1_cat4_cdf0[CDF_SIZE(16)] = { AOM_CDF16(
    3934, 7460, 10719, 13640, 16203, 18500, 20624, 22528, 24316, 25919, 27401,
    28729, 29894, 30938, 31903) };
const aom_cdf_prob *av1_cat4_cdf[] = { av1_cat4_cdf0 };

const aom_cdf_prob av1_cat5_cdf0[CDF_SIZE(16)] = { AOM_CDF16(
    2942, 5794, 8473, 11069, 13469, 15795, 17980, 20097, 21952, 23750, 25439,
    27076, 28589, 30056, 31434) };
const aom_cdf_prob av1_cat5_cdf1[CDF_SIZE(2)] = { AOM_CDF2(23040) };
const aom_cdf_prob *av1_cat5_cdf[] = { av1_cat5_cdf0, av1_cat5_cdf1 };

const aom_cdf_prob av1_cat6_cdf0[CDF_SIZE(16)] = { AOM_CDF16(
    2382, 4727, 7036, 9309, 11512, 13681, 15816, 17918, 19892, 21835, 23748,
    25632, 27458, 29255, 31024) };
const aom_cdf_prob av1_cat6_cdf1[CDF_SIZE(16)] = { AOM_CDF16(
    9314, 15584, 19741, 22540, 25391, 27310, 28583, 29440, 30493, 31202, 31672,
    31988, 32310, 32527, 32671) };
const aom_cdf_prob av1_cat6_cdf2[CDF_SIZE(16)] = { AOM_CDF16(
    29548, 31129, 31960, 32004, 32473, 32498, 32511, 32512, 32745, 32757, 32763,
    32764, 32765, 32766, 32767) };
const aom_cdf_prob av1_cat6_cdf3[CDF_SIZE(16)] = { AOM_CDF16(
    32006, 32258, 32510, 32512, 32638, 32639, 32640, 32641, 32761, 32762, 32763,
    32764, 32765, 32766, 32767) };
const aom_cdf_prob av1_cat6_cdf4[CDF_SIZE(4)] = { AOM_CDF4(32513, 32641,
                                                           32767) };
const aom_cdf_prob *av1_cat6_cdf[] = {
  av1_cat6_cdf0, av1_cat6_cdf1, av1_cat6_cdf2, av1_cat6_cdf3, av1_cat6_cdf4
};

const uint8_t av1_coefband_trans_8x8plus[MAX_TX_SQUARE] = {
  0, 1, 1, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5,
  // beyond MAXBAND_INDEX+1 all values are filled as 5
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
#if CONFIG_TX64X64
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5
#endif  // CONFIG_TX64X64
};

const uint8_t av1_coefband_trans_4x8_8x4[32] = {
  0, 1, 1, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4,
  4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
};

const uint8_t av1_coefband_trans_4x4[16] = {
  0, 1, 1, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 5, 5, 5,
};

const uint8_t av1_pt_energy_class[ENTROPY_TOKENS] = { 0, 1, 2, 3, 3, 4,
                                                      4, 5, 5, 5, 5, 5 };

// Model obtained from a 2-sided zero-centered distribution derived
// from a Pareto distribution. The cdf of the distribution is:
// cdf(x) = 0.5 + 0.5 * sgn(x) * [1 - {alpha/(alpha + |x|)} ^ beta]
//
// For a given beta and a given probability of the 1-node, the alpha
// is first solved, and then the {alpha, beta} pair is used to generate
// the probabilities for the rest of the nodes.
//
// The full source code of the generating program is available in:
// tools/gen_constrained_tokenset.py
//
// Values for tokens TWO_TOKEN through CATEGORY6_TOKEN included
// in the table here : the ONE_TOKEN probability is
// removed and the probabilities rescaled.
//
// ZERO_TOKEN and ONE_TOKEN are coded as one CDF,
// and EOB_TOKEN is coded as flags outside this coder.
const aom_cdf_prob av1_pareto8_tail_probs[COEFF_PROB_MODELS][TAIL_NODES] = {
  { 128, 127, 127, 252, 497, 969, 1839, 3318, 25511 },
  { 256, 254, 251, 496, 966, 1834, 3308, 5408, 19995 },
  { 383, 378, 373, 732, 1408, 2605, 4470, 6646, 15773 },
  { 511, 502, 493, 961, 1824, 3289, 5373, 7298, 12517 },
  { 638, 625, 611, 1182, 2215, 3894, 6064, 7548, 9991 },
  { 766, 746, 726, 1396, 2582, 4428, 6578, 7529, 8017 },
  { 893, 866, 839, 1603, 2927, 4896, 6945, 7332, 6467 },
  { 1020, 984, 950, 1803, 3250, 5305, 7191, 7022, 5243 },
  { 1147, 1102, 1059, 1996, 3552, 5659, 7338, 6646, 4269 },
  { 1274, 1218, 1166, 2183, 3835, 5963, 7403, 6234, 3492 },
  { 1400, 1334, 1270, 2363, 4099, 6223, 7401, 5809, 2869 },
  { 1527, 1447, 1372, 2537, 4345, 6442, 7346, 5386, 2366 },
  { 1654, 1560, 1473, 2704, 4574, 6624, 7247, 4973, 1959 },
  { 1780, 1672, 1571, 2866, 4787, 6771, 7114, 4579, 1628 },
  { 1906, 1782, 1667, 3022, 4984, 6889, 6954, 4206, 1358 },
  { 2032, 1891, 1762, 3172, 5167, 6979, 6773, 3856, 1136 },
  { 2158, 2000, 1854, 3316, 5335, 7044, 6577, 3530, 954 },
  { 2284, 2106, 1944, 3455, 5490, 7087, 6370, 3229, 803 },
  { 2410, 2212, 2032, 3588, 5632, 7109, 6155, 2951, 679 },
  { 2535, 2317, 2119, 3717, 5761, 7113, 5936, 2695, 575 },
  { 2661, 2420, 2203, 3840, 5880, 7101, 5714, 2461, 488 },
  { 2786, 2522, 2286, 3958, 5987, 7074, 5493, 2246, 416 },
  { 2911, 2624, 2367, 4072, 6083, 7033, 5273, 2050, 355 },
  { 3037, 2724, 2446, 4180, 6170, 6981, 5055, 1871, 304 },
  { 3162, 2822, 2523, 4284, 6247, 6919, 4842, 1708, 261 },
  { 3286, 2920, 2599, 4384, 6315, 6848, 4633, 1559, 224 },
  { 3411, 3017, 2672, 4478, 6374, 6768, 4430, 1424, 194 },
  { 3536, 3112, 2745, 4569, 6426, 6681, 4232, 1300, 167 },
  { 3660, 3207, 2815, 4656, 6469, 6588, 4040, 1188, 145 },
  { 3785, 3300, 2883, 4738, 6505, 6490, 3855, 1086, 126 },
  { 3909, 3392, 2950, 4817, 6534, 6387, 3677, 993, 109 },
  { 4033, 3483, 3015, 4891, 6557, 6281, 3505, 908, 95 },
  { 4157, 3573, 3079, 4962, 6573, 6170, 3340, 831, 83 },
  { 4281, 3662, 3141, 5029, 6584, 6058, 3181, 760, 72 },
  { 4405, 3750, 3201, 5093, 6588, 5943, 3029, 696, 63 },
  { 4529, 3837, 3260, 5152, 6587, 5826, 2883, 638, 56 },
  { 4652, 3922, 3317, 5209, 6582, 5709, 2744, 584, 49 },
  { 4775, 4007, 3373, 5262, 6572, 5590, 2610, 536, 43 },
  { 4899, 4090, 3427, 5312, 6557, 5470, 2483, 492, 38 },
  { 5022, 4173, 3480, 5359, 6538, 5351, 2361, 451, 33 },
  { 5145, 4254, 3531, 5403, 6515, 5231, 2246, 414, 29 },
  { 5268, 4334, 3581, 5443, 6489, 5112, 2135, 380, 26 },
  { 5391, 4414, 3629, 5481, 6458, 4993, 2029, 350, 23 },
  { 5514, 4492, 3676, 5515, 6425, 4875, 1929, 321, 21 },
  { 5637, 4569, 3721, 5548, 6388, 4758, 1833, 296, 18 },
  { 5759, 4645, 3766, 5577, 6349, 4642, 1742, 272, 16 },
  { 5881, 4720, 3808, 5604, 6307, 4528, 1656, 250, 14 },
  { 6004, 4794, 3849, 5628, 6262, 4414, 1573, 231, 13 },
  { 6126, 4867, 3890, 5649, 6215, 4302, 1495, 213, 11 },
  { 6248, 4939, 3928, 5669, 6166, 4192, 1420, 196, 10 },
  { 6370, 5010, 3966, 5686, 6114, 4083, 1349, 181, 9 },
  { 6492, 5080, 4002, 5700, 6061, 3976, 1282, 167, 8 },
  { 6614, 5149, 4037, 5712, 6006, 3871, 1218, 154, 7 },
  { 6735, 5217, 4070, 5723, 5950, 3767, 1157, 142, 7 },
  { 6857, 5284, 4103, 5731, 5891, 3666, 1099, 131, 6 },
  { 6978, 5351, 4134, 5737, 5832, 3566, 1044, 121, 5 },
  { 7099, 5415, 4164, 5741, 5771, 3469, 992, 112, 5 },
  { 7221, 5479, 4192, 5743, 5709, 3373, 943, 104, 4 },
  { 7342, 5542, 4220, 5743, 5646, 3279, 896, 96, 4 },
  { 7462, 5604, 4246, 5742, 5583, 3187, 851, 89, 4 },
  { 7584, 5665, 4272, 5739, 5518, 3097, 808, 82, 3 },
  { 7704, 5725, 4296, 5734, 5453, 3009, 768, 76, 3 },
  { 7825, 5784, 4318, 5727, 5386, 2924, 730, 71, 3 },
  { 7945, 5843, 4341, 5719, 5320, 2840, 693, 65, 2 },
  { 8066, 5900, 4361, 5709, 5252, 2758, 659, 61, 2 },
  { 8186, 5956, 4381, 5698, 5185, 2678, 626, 56, 2 },
  { 8306, 6011, 4400, 5685, 5117, 2600, 595, 52, 2 },
  { 8426, 6066, 4418, 5671, 5049, 2523, 565, 48, 2 },
  { 8547, 6119, 4434, 5655, 4981, 2449, 537, 45, 1 },
  { 8666, 6171, 4450, 5638, 4912, 2377, 511, 42, 1 },
  { 8786, 6223, 4465, 5620, 4843, 2306, 485, 39, 1 },
  { 8906, 6274, 4478, 5600, 4775, 2237, 461, 36, 1 },
  { 9025, 6323, 4491, 5580, 4706, 2170, 438, 34, 1 },
  { 9144, 6372, 4503, 5558, 4637, 2105, 417, 31, 1 },
  { 9264, 6420, 4514, 5535, 4568, 2041, 396, 29, 1 },
  { 9383, 6467, 4524, 5511, 4500, 1979, 376, 27, 1 },
  { 9502, 6513, 4532, 5486, 4432, 1919, 358, 25, 1 },
  { 9621, 6558, 4541, 5460, 4364, 1860, 340, 23, 1 },
  { 9740, 6602, 4548, 5433, 4296, 1803, 323, 22, 1 },
  { 9859, 6645, 4554, 5405, 4229, 1748, 307, 20, 1 },
  { 9978, 6688, 4559, 5376, 4161, 1694, 292, 19, 1 },
  { 10096, 6729, 4564, 5347, 4094, 1641, 278, 18, 1 },
  { 10215, 6770, 4568, 5316, 4028, 1590, 264, 16, 1 },
  { 10333, 6809, 4571, 5285, 3962, 1541, 251, 15, 1 },
  { 10452, 6848, 4573, 5253, 3896, 1492, 239, 14, 1 },
  { 10570, 6886, 4574, 5220, 3831, 1446, 227, 13, 1 },
  { 10688, 6923, 4575, 5186, 3767, 1400, 216, 12, 1 },
  { 10806, 6959, 4575, 5152, 3702, 1356, 205, 12, 1 },
  { 10924, 6994, 4574, 5117, 3639, 1313, 195, 11, 1 },
  { 11041, 7029, 4572, 5082, 3576, 1271, 186, 10, 1 },
  { 11159, 7062, 4570, 5046, 3513, 1231, 177, 9, 1 },
  { 11277, 7095, 4566, 5009, 3451, 1192, 168, 9, 1 },
  { 11394, 7127, 4563, 4972, 3390, 1153, 160, 8, 1 },
  { 11512, 7158, 4558, 4934, 3329, 1116, 152, 8, 1 },
  { 11629, 7188, 4553, 4896, 3269, 1080, 145, 7, 1 },
  { 11746, 7217, 4547, 4857, 3210, 1045, 138, 7, 1 },
  { 11864, 7245, 4540, 4818, 3151, 1012, 131, 6, 1 },
  { 11980, 7273, 4533, 4779, 3093, 979, 124, 6, 1 },
  { 12097, 7300, 4525, 4739, 3035, 947, 118, 6, 1 },
  { 12215, 7326, 4516, 4698, 2978, 916, 113, 5, 1 },
  { 12331, 7351, 4507, 4658, 2922, 886, 107, 5, 1 },
  { 12448, 7375, 4497, 4617, 2866, 857, 102, 5, 1 },
  { 12564, 7398, 4487, 4576, 2812, 829, 97, 4, 1 },
  { 12681, 7421, 4476, 4534, 2757, 802, 92, 4, 1 },
  { 12797, 7443, 4464, 4492, 2704, 775, 88, 4, 1 },
  { 12914, 7464, 4452, 4450, 2651, 749, 84, 3, 1 },
  { 13030, 7484, 4439, 4408, 2599, 725, 79, 3, 1 },
  { 13147, 7503, 4426, 4365, 2547, 700, 76, 3, 1 },
  { 13262, 7522, 4412, 4322, 2497, 677, 72, 3, 1 },
  { 13378, 7539, 4398, 4280, 2447, 654, 68, 3, 1 },
  { 13494, 7556, 4383, 4237, 2397, 632, 65, 3, 1 },
  { 13610, 7573, 4368, 4193, 2348, 611, 62, 2, 1 },
  { 13726, 7588, 4352, 4150, 2300, 590, 59, 2, 1 },
  { 13841, 7602, 4335, 4107, 2253, 571, 56, 2, 1 },
  { 13957, 7616, 4318, 4063, 2207, 551, 53, 2, 1 },
  { 14072, 7629, 4301, 4019, 2161, 532, 51, 2, 1 },
  { 14188, 7641, 4283, 3976, 2115, 514, 48, 2, 1 },
  { 14302, 7652, 4265, 3932, 2071, 497, 46, 2, 1 },
  { 14418, 7663, 4246, 3888, 2027, 480, 44, 1, 1 },
  { 14533, 7673, 4227, 3844, 1984, 463, 42, 1, 1 },
  { 14649, 7682, 4207, 3800, 1941, 447, 40, 1, 1 },
  { 14763, 7690, 4187, 3757, 1899, 432, 38, 1, 1 },
  { 14878, 7698, 4166, 3713, 1858, 417, 36, 1, 1 },
  { 14993, 7705, 4146, 3669, 1817, 402, 34, 1, 1 },
  { 15109, 7711, 4124, 3625, 1777, 388, 32, 1, 1 },
  { 15223, 7715, 4103, 3581, 1738, 375, 31, 1, 1 },
  { 15337, 7720, 4081, 3538, 1699, 362, 29, 1, 1 },
  { 15452, 7724, 4058, 3494, 1661, 349, 28, 1, 1 },
  { 15567, 7727, 4035, 3450, 1624, 337, 26, 1, 1 },
  { 15681, 7729, 4012, 3407, 1587, 325, 25, 1, 1 },
  { 15795, 7730, 3989, 3364, 1551, 313, 24, 1, 1 },
  { 15909, 7731, 3965, 3320, 1516, 302, 23, 1, 1 },
  { 16024, 7731, 3940, 3277, 1481, 291, 22, 1, 1 },
  { 16138, 7730, 3916, 3234, 1446, 281, 21, 1, 1 },
  { 16252, 7728, 3891, 3191, 1413, 271, 20, 1, 1 },
  { 16366, 7726, 3866, 3148, 1380, 261, 19, 1, 1 },
  { 16480, 7723, 3840, 3106, 1347, 252, 18, 1, 1 },
  { 16594, 7720, 3814, 3063, 1315, 243, 17, 1, 1 },
  { 16708, 7715, 3788, 3021, 1284, 234, 16, 1, 1 },
  { 16822, 7710, 3762, 2979, 1253, 225, 15, 1, 1 },
  { 16936, 7704, 3735, 2937, 1223, 217, 14, 1, 1 },
  { 17050, 7697, 3708, 2895, 1193, 209, 14, 1, 1 },
  { 17162, 7690, 3681, 2854, 1164, 202, 13, 1, 1 },
  { 17276, 7682, 3654, 2812, 1136, 194, 12, 1, 1 },
  { 17389, 7673, 3626, 2771, 1108, 187, 12, 1, 1 },
  { 17504, 7663, 3598, 2730, 1080, 180, 11, 1, 1 },
  { 17617, 7653, 3570, 2689, 1053, 173, 11, 1, 1 },
  { 17730, 7642, 3541, 2649, 1027, 167, 10, 1, 1 },
  { 17843, 7630, 3513, 2608, 1001, 161, 10, 1, 1 },
  { 17957, 7618, 3484, 2569, 975, 154, 9, 1, 1 },
  { 18069, 7605, 3455, 2529, 950, 149, 9, 1, 1 },
  { 18183, 7591, 3426, 2489, 926, 143, 8, 1, 1 },
  { 18296, 7576, 3396, 2450, 902, 138, 8, 1, 1 },
  { 18410, 7562, 3366, 2411, 878, 132, 7, 1, 1 },
  { 18523, 7545, 3337, 2372, 855, 127, 7, 1, 1 },
  { 18636, 7529, 3306, 2333, 833, 122, 7, 1, 1 },
  { 18749, 7511, 3276, 2295, 811, 118, 6, 1, 1 },
  { 18862, 7493, 3246, 2257, 789, 113, 6, 1, 1 },
  { 18975, 7474, 3215, 2219, 768, 109, 6, 1, 1 },
  { 19088, 7455, 3185, 2182, 747, 104, 5, 1, 1 },
  { 19201, 7435, 3154, 2144, 727, 100, 5, 1, 1 },
  { 19314, 7414, 3123, 2107, 707, 96, 5, 1, 1 },
  { 19427, 7392, 3092, 2071, 687, 92, 5, 1, 1 },
  { 19541, 7370, 3060, 2034, 668, 89, 4, 1, 1 },
  { 19654, 7347, 3029, 1998, 649, 85, 4, 1, 1 },
  { 19766, 7323, 2997, 1963, 631, 82, 4, 1, 1 },
  { 19878, 7299, 2966, 1927, 613, 79, 4, 1, 1 },
  { 19991, 7274, 2934, 1892, 596, 75, 4, 1, 1 },
  { 20105, 7248, 2902, 1857, 579, 72, 3, 1, 1 },
  { 20218, 7222, 2870, 1822, 562, 69, 3, 1, 1 },
  { 20331, 7195, 2838, 1788, 545, 66, 3, 1, 1 },
  { 20443, 7167, 2806, 1754, 529, 64, 3, 1, 1 },
  { 20556, 7138, 2774, 1720, 514, 61, 3, 1, 1 },
  { 20670, 7109, 2741, 1687, 498, 58, 3, 1, 1 },
  { 20783, 7079, 2709, 1654, 483, 56, 2, 1, 1 },
  { 20895, 7049, 2676, 1621, 469, 54, 2, 1, 1 },
  { 21008, 7017, 2644, 1589, 455, 51, 2, 1, 1 },
  { 21121, 6985, 2611, 1557, 441, 49, 2, 1, 1 },
  { 21234, 6953, 2578, 1525, 427, 47, 2, 1, 1 },
  { 21347, 6919, 2545, 1494, 414, 45, 2, 1, 1 },
  { 21460, 6885, 2513, 1462, 401, 43, 2, 1, 1 },
  { 21573, 6850, 2480, 1432, 388, 41, 2, 1, 1 },
  { 21687, 6815, 2447, 1401, 375, 39, 2, 1, 1 },
  { 21801, 6778, 2414, 1371, 363, 38, 1, 1, 1 },
  { 21914, 6741, 2381, 1341, 352, 36, 1, 1, 1 },
  { 22028, 6704, 2348, 1311, 340, 34, 1, 1, 1 },
  { 22141, 6665, 2315, 1282, 329, 33, 1, 1, 1 },
  { 22255, 6626, 2282, 1253, 318, 31, 1, 1, 1 },
  { 22368, 6586, 2249, 1225, 307, 30, 1, 1, 1 },
  { 22482, 6546, 2216, 1196, 297, 28, 1, 1, 1 },
  { 22595, 6505, 2183, 1169, 286, 27, 1, 1, 1 },
  { 22709, 6463, 2149, 1141, 277, 26, 1, 1, 1 },
  { 22823, 6420, 2116, 1114, 267, 25, 1, 1, 1 },
  { 22938, 6377, 2083, 1087, 257, 23, 1, 1, 1 },
  { 23053, 6332, 2050, 1060, 248, 22, 1, 1, 1 },
  { 23167, 6287, 2017, 1034, 239, 21, 1, 1, 1 },
  { 23280, 6242, 1984, 1008, 231, 20, 1, 1, 1 },
  { 23396, 6195, 1951, 982, 222, 19, 1, 1, 1 },
  { 23510, 6148, 1918, 957, 214, 18, 1, 1, 1 },
  { 23625, 6100, 1885, 932, 206, 17, 1, 1, 1 },
  { 23741, 6051, 1852, 907, 198, 16, 1, 1, 1 },
  { 23855, 6002, 1819, 883, 190, 16, 1, 1, 1 },
  { 23971, 5951, 1786, 859, 183, 15, 1, 1, 1 },
  { 24087, 5900, 1753, 835, 176, 14, 1, 1, 1 },
  { 24203, 5848, 1720, 812, 169, 13, 1, 1, 1 },
  { 24318, 5796, 1687, 789, 162, 13, 1, 1, 1 },
  { 24435, 5742, 1655, 766, 155, 12, 1, 1, 1 },
  { 24552, 5688, 1622, 743, 149, 11, 1, 1, 1 },
  { 24669, 5632, 1589, 721, 143, 11, 1, 1, 1 },
  { 24786, 5576, 1557, 699, 137, 10, 1, 1, 1 },
  { 24903, 5519, 1524, 678, 131, 10, 1, 1, 1 },
  { 25021, 5462, 1491, 657, 125, 9, 1, 1, 1 },
  { 25139, 5403, 1459, 636, 120, 8, 1, 1, 1 },
  { 25258, 5343, 1427, 615, 114, 8, 1, 1, 1 },
  { 25376, 5283, 1394, 595, 109, 8, 1, 1, 1 },
  { 25496, 5221, 1362, 575, 104, 7, 1, 1, 1 },
  { 25614, 5159, 1330, 556, 99, 7, 1, 1, 1 },
  { 25735, 5096, 1298, 536, 94, 6, 1, 1, 1 },
  { 25856, 5031, 1265, 517, 90, 6, 1, 1, 1 },
  { 25977, 4966, 1233, 499, 85, 5, 1, 1, 1 },
  { 26098, 4899, 1202, 480, 81, 5, 1, 1, 1 },
  { 26220, 4831, 1170, 462, 77, 5, 1, 1, 1 },
  { 26343, 4763, 1138, 444, 73, 4, 1, 1, 1 },
  { 26466, 4693, 1106, 427, 69, 4, 1, 1, 1 },
  { 26589, 4622, 1075, 410, 65, 4, 1, 1, 1 },
  { 26713, 4550, 1043, 393, 62, 4, 1, 1, 1 },
  { 26840, 4476, 1012, 376, 58, 3, 1, 1, 1 },
  { 26966, 4401, 980, 360, 55, 3, 1, 1, 1 },
  { 27092, 4325, 949, 344, 52, 3, 1, 1, 1 },
  { 27220, 4248, 918, 328, 48, 3, 1, 1, 1 },
  { 27350, 4169, 886, 313, 45, 2, 1, 1, 1 },
  { 27480, 4088, 855, 298, 42, 2, 1, 1, 1 },
  { 27610, 4006, 824, 283, 40, 2, 1, 1, 1 },
  { 27743, 3922, 793, 268, 37, 2, 1, 1, 1 },
  { 27876, 3837, 762, 254, 34, 2, 1, 1, 1 },
  { 28011, 3749, 731, 240, 32, 2, 1, 1, 1 },
  { 28147, 3659, 701, 227, 30, 1, 1, 1, 1 },
  { 28286, 3568, 670, 213, 27, 1, 1, 1, 1 },
  { 28426, 3474, 639, 200, 25, 1, 1, 1, 1 },
  { 28569, 3377, 608, 187, 23, 1, 1, 1, 1 },
  { 28714, 3278, 577, 174, 21, 1, 1, 1, 1 },
  { 28860, 3176, 547, 162, 19, 1, 1, 1, 1 },
  { 29010, 3071, 516, 150, 17, 1, 1, 1, 1 },
  { 29163, 2962, 485, 138, 16, 1, 1, 1, 1 },
  { 29320, 2849, 454, 127, 14, 1, 1, 1, 1 },
  { 29483, 2731, 423, 115, 12, 1, 1, 1, 1 },
  { 29650, 2608, 391, 104, 11, 1, 1, 1, 1 },
  { 29823, 2479, 360, 93, 9, 1, 1, 1, 1 },
  { 30002, 2343, 328, 83, 8, 1, 1, 1, 1 },
  { 30192, 2198, 295, 72, 7, 1, 1, 1, 1 },
  { 30393, 2041, 262, 62, 6, 1, 1, 1, 1 },
  { 30612, 1869, 227, 52, 4, 1, 1, 1, 1 },
  { 30853, 1676, 191, 41, 3, 1, 1, 1, 1 },
  { 31131, 1448, 152, 31, 2, 1, 1, 1, 1 },
  { 31486, 1150, 107, 20, 1, 1, 1, 1, 1 },
};

static void build_tail_cdfs(aom_cdf_prob cdf_tail[CDF_SIZE(ENTROPY_TOKENS)],
                            aom_cdf_prob cdf_head[CDF_SIZE(ENTROPY_TOKENS)],
                            int band_zero) {
  int probNZ, prob1, prob_idx, i;
  int phead[HEAD_TOKENS + 1], sum;
  const int is_dc = !!band_zero;
  aom_cdf_prob prev_cdf;
  prev_cdf = 0;
  for (i = 0; i < HEAD_TOKENS + is_dc; ++i) {
    phead[i] = AOM_ICDF(cdf_head[i]) - prev_cdf;
    prev_cdf = AOM_ICDF(cdf_head[i]);
  }
  // Do the tail
  probNZ = CDF_PROB_TOP - phead[ZERO_TOKEN + is_dc] - (is_dc ? phead[0] : 0);
  prob1 = phead[is_dc + ONE_TOKEN_EOB] + phead[is_dc + ONE_TOKEN_NEOB];
  prob_idx =
      AOMMIN(COEFF_PROB_MODELS - 1, AOMMAX(0, (256 * prob1 / probNZ) - 1));

  sum = 0;
  for (i = 0; i < TAIL_TOKENS; ++i) {
    sum += av1_pareto8_tail_probs[prob_idx][i];
    cdf_tail[i] = AOM_ICDF(
        ((sum - (i + 1)) * ((CDF_INIT_TOP >> CDF_SHIFT) - TAIL_TOKENS) +
         ((CDF_INIT_TOP - TAIL_TOKENS) >> 1)) /
            ((CDF_INIT_TOP - TAIL_TOKENS)) +
        (i + 1));
  }
}

void av1_coef_pareto_cdfs(FRAME_CONTEXT *fc) {
  /* Build the tail based on a Pareto distribution */
  TX_SIZE t;
  int i, j, k, l;
  for (t = 0; t < TX_SIZES; ++t)
    for (i = 0; i < PLANE_TYPES; ++i)
      for (j = 0; j < REF_TYPES; ++j)
        for (k = 0; k < COEF_BANDS; ++k)
          for (l = 0; l < BAND_COEFF_CONTEXTS(k); ++l)
            build_tail_cdfs(fc->coef_tail_cdfs[t][i][j][k][l],
                            fc->coef_head_cdfs[t][i][j][k][l], k == 0);
}

void av1_default_coef_probs(AV1_COMMON *cm) {
  const int index = AOMMIN(TOKEN_CDF_Q_CTXS - 1, cm->base_qindex / 64);
  av1_copy(cm->fc->coef_head_cdfs[TX_4X4],
           (*av1_default_qctx_coef_cdfs[index])[TX_4X4]);
  av1_copy(cm->fc->coef_head_cdfs[TX_8X8],
           (*av1_default_qctx_coef_cdfs[index])[TX_8X8]);
  av1_copy(cm->fc->coef_head_cdfs[TX_16X16],
           (*av1_default_qctx_coef_cdfs[index])[TX_16X16]);
  av1_copy(cm->fc->coef_head_cdfs[TX_32X32],
           (*av1_default_qctx_coef_cdfs[index])[TX_32X32]);
#if CONFIG_TX64X64
  av1_copy(cm->fc->coef_head_cdfs[TX_64X64],
           (*av1_default_qctx_coef_cdfs[index])[TX_32X32]);
#endif  // CONFIG_TX64X64
  av1_coef_pareto_cdfs(cm->fc);
}

static void av1_average_cdf(aom_cdf_prob *cdf_ptr[], aom_cdf_prob *fc_cdf_ptr,
                            int cdf_size, const int num_tiles) {
  int i;
  for (i = 0; i < cdf_size;) {
    do {
      int sum = 0;
      int j;
      assert(i < cdf_size);
      for (j = 0; j < num_tiles; ++j) sum += AOM_ICDF(cdf_ptr[j][i]);
      fc_cdf_ptr[i] = AOM_ICDF(sum / num_tiles);
    } while (fc_cdf_ptr[i++] != AOM_ICDF(CDF_PROB_TOP));
    // Zero symbol counts for the next frame
    assert(i < cdf_size);
    fc_cdf_ptr[i++] = 0;
    // Skip trailing zeros until the start of the next CDF.
    for (; i < cdf_size && fc_cdf_ptr[i] == 0; ++i) {
    }
  }
}

#define AVERAGE_TILE_CDFS(cname)                               \
  do {                                                         \
    for (i = 0; i < num_tiles; ++i)                            \
      cdf_ptr[i] = (aom_cdf_prob *)&ec_ctxs[i]->cname;         \
    fc_cdf_ptr = (aom_cdf_prob *)&fc->cname;                   \
    cdf_size = (int)sizeof(fc->cname) / sizeof(aom_cdf_prob);  \
    av1_average_cdf(cdf_ptr, fc_cdf_ptr, cdf_size, num_tiles); \
  } while (0);

void av1_average_tile_coef_cdfs(FRAME_CONTEXT *fc, FRAME_CONTEXT *ec_ctxs[],
                                aom_cdf_prob *cdf_ptr[], int num_tiles) {
  int i, cdf_size;
  aom_cdf_prob *fc_cdf_ptr;
  assert(num_tiles == 1);

#if CONFIG_LV_MAP
  AVERAGE_TILE_CDFS(txb_skip_cdf)
  AVERAGE_TILE_CDFS(eob_extra_cdf)
  AVERAGE_TILE_CDFS(dc_sign_cdf)
  AVERAGE_TILE_CDFS(coeff_base_cdf)
  AVERAGE_TILE_CDFS(eob_flag_cdf16)
  AVERAGE_TILE_CDFS(eob_flag_cdf32)
  AVERAGE_TILE_CDFS(eob_flag_cdf64)
  AVERAGE_TILE_CDFS(eob_flag_cdf128)
  AVERAGE_TILE_CDFS(eob_flag_cdf256)
  AVERAGE_TILE_CDFS(eob_flag_cdf512)
  AVERAGE_TILE_CDFS(eob_flag_cdf1024)
  AVERAGE_TILE_CDFS(coeff_base_eob_cdf)
  AVERAGE_TILE_CDFS(coeff_br_cdf)
#else  // CONFI_LV_MAP
  AVERAGE_TILE_CDFS(coef_head_cdfs)
  AVERAGE_TILE_CDFS(coef_tail_cdfs)
#endif
}

void av1_average_tile_mv_cdfs(FRAME_CONTEXT *fc, FRAME_CONTEXT *ec_ctxs[],
                              aom_cdf_prob *cdf_ptr[], int num_tiles) {
  int i, k, cdf_size;

  aom_cdf_prob *fc_cdf_ptr;

  assert(num_tiles == 1);

  int j;
  for (j = 0; j < NMV_CONTEXTS; ++j) {
    AVERAGE_TILE_CDFS(nmvc[j].joints_cdf)

    for (k = 0; k < 2; ++k) {
      AVERAGE_TILE_CDFS(nmvc[j].comps[k].classes_cdf)
      AVERAGE_TILE_CDFS(nmvc[j].comps[k].class0_fp_cdf)
      AVERAGE_TILE_CDFS(nmvc[j].comps[k].fp_cdf)
      AVERAGE_TILE_CDFS(nmvc[j].comps[k].sign_cdf)
      AVERAGE_TILE_CDFS(nmvc[j].comps[k].hp_cdf)
      AVERAGE_TILE_CDFS(nmvc[j].comps[k].class0_hp_cdf)
      AVERAGE_TILE_CDFS(nmvc[j].comps[k].class0_cdf)
      AVERAGE_TILE_CDFS(nmvc[j].comps[k].bits_cdf)
    }
  }
}

void av1_average_tile_loopfilter_cdfs(FRAME_CONTEXT *fc,
                                      FRAME_CONTEXT *ec_ctxs[],
                                      aom_cdf_prob *cdf_ptr[], int num_tiles) {
  (void)fc;
  (void)ec_ctxs;
  (void)num_tiles;
  (void)cdf_ptr;

  assert(num_tiles == 1);

  int i, cdf_size;
  aom_cdf_prob *fc_cdf_ptr;
  (void)i;
  (void)cdf_size;
  (void)fc_cdf_ptr;

#if CONFIG_LOOP_RESTORATION
  AVERAGE_TILE_CDFS(switchable_restore_cdf)
  AVERAGE_TILE_CDFS(wiener_restore_cdf)
  AVERAGE_TILE_CDFS(sgrproj_restore_cdf)
#endif  // CONFIG_LOOP_RESTORATION
}

void av1_average_tile_intra_cdfs(FRAME_CONTEXT *fc, FRAME_CONTEXT *ec_ctxs[],
                                 aom_cdf_prob *cdf_ptr[], int num_tiles) {
  int i, cdf_size;

  assert(num_tiles == 1);
  aom_cdf_prob *fc_cdf_ptr;

  AVERAGE_TILE_CDFS(tx_size_cdf)

  AVERAGE_TILE_CDFS(intra_ext_tx_cdf)
  AVERAGE_TILE_CDFS(inter_ext_tx_cdf)

  AVERAGE_TILE_CDFS(seg.tree_cdf)
  AVERAGE_TILE_CDFS(seg.pred_cdf)
  AVERAGE_TILE_CDFS(uv_mode_cdf)

#if CONFIG_CFL
  AVERAGE_TILE_CDFS(cfl_sign_cdf)
  AVERAGE_TILE_CDFS(cfl_alpha_cdf)
#endif

  AVERAGE_TILE_CDFS(partition_cdf)

  AVERAGE_TILE_CDFS(delta_q_cdf)
#if CONFIG_EXT_DELTA_Q
  AVERAGE_TILE_CDFS(delta_lf_cdf)
#if CONFIG_LOOPFILTER_LEVEL
  AVERAGE_TILE_CDFS(delta_lf_multi_cdf)
#endif
#endif

  AVERAGE_TILE_CDFS(skip_cdfs)
  AVERAGE_TILE_CDFS(txfm_partition_cdf)
  AVERAGE_TILE_CDFS(palette_y_size_cdf)
  AVERAGE_TILE_CDFS(palette_uv_size_cdf)
  AVERAGE_TILE_CDFS(palette_y_color_index_cdf)
  AVERAGE_TILE_CDFS(palette_uv_color_index_cdf)
#if CONFIG_FILTER_INTRA
  AVERAGE_TILE_CDFS(filter_intra_cdfs)
  AVERAGE_TILE_CDFS(filter_intra_mode_cdf)
#endif
  AVERAGE_TILE_CDFS(palette_y_mode_cdf)
  AVERAGE_TILE_CDFS(palette_uv_mode_cdf)
#if CONFIG_EXT_INTRA_MOD
  AVERAGE_TILE_CDFS(angle_delta_cdf)
#endif  // CONFIG_EXT_INTRA_MOD
#if CONFIG_SPATIAL_SEGMENTATION
  int j;
  for (j = 0; j < SPATIAL_PREDICTION_PROBS; j++) {
    AVERAGE_TILE_CDFS(seg.spatial_pred_seg_cdf[j]);
  }
#endif
}

void av1_average_tile_inter_cdfs(AV1_COMMON *cm, FRAME_CONTEXT *fc,
                                 FRAME_CONTEXT *ec_ctxs[],
                                 aom_cdf_prob *cdf_ptr[], int num_tiles) {
  int i, cdf_size;

  assert(num_tiles == 1);
  aom_cdf_prob *fc_cdf_ptr;

  AVERAGE_TILE_CDFS(comp_inter_cdf)
  AVERAGE_TILE_CDFS(comp_ref_cdf)
  AVERAGE_TILE_CDFS(comp_bwdref_cdf)

  AVERAGE_TILE_CDFS(single_ref_cdf)

  AVERAGE_TILE_CDFS(newmv_cdf)
  AVERAGE_TILE_CDFS(zeromv_cdf)
  AVERAGE_TILE_CDFS(refmv_cdf)
  AVERAGE_TILE_CDFS(drl_cdf)
#if CONFIG_EXT_COMP_REFS
  AVERAGE_TILE_CDFS(uni_comp_ref_cdf)
  AVERAGE_TILE_CDFS(comp_ref_type_cdf)
#endif
  AVERAGE_TILE_CDFS(inter_compound_mode_cdf)

  AVERAGE_TILE_CDFS(compound_type_cdf)

  AVERAGE_TILE_CDFS(interintra_cdf)
  AVERAGE_TILE_CDFS(wedge_interintra_cdf)
  AVERAGE_TILE_CDFS(interintra_mode_cdf)

  /* NB: kf_y_cdf is discarded after use, so no need
     for backwards update */
  AVERAGE_TILE_CDFS(y_mode_cdf)

  if (cm->interp_filter == SWITCHABLE) {
    AVERAGE_TILE_CDFS(switchable_interp_cdf)
  }
  AVERAGE_TILE_CDFS(intra_inter_cdf)
  AVERAGE_TILE_CDFS(motion_mode_cdf)
  AVERAGE_TILE_CDFS(obmc_cdf)
#if CONFIG_JNT_COMP
  AVERAGE_TILE_CDFS(compound_index_cdf);
#endif  // CONFIG_JNT_COMP
}
