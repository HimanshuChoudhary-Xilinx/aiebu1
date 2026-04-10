// SPDX-License-Identifier: MIT
// Copyright (C) 2024-2025, Advanced Micro Devices, Inc. All rights reserved.

#ifndef _AIEBU_COMMON_TIME_UTIL_H_
#define _AIEBU_COMMON_TIME_UTIL_H_

#include <ctime>

namespace aiebu {

/** Thread-safe local time breakdown for @p t into @p out. */
inline void
localtime_to_tm(std::time_t t, std::tm& out)
{
#if defined(_MSC_VER)
  (void)localtime_s(&out, &t);
#else
  (void)localtime_r(&t, &out);
#endif
}

} // namespace aiebu

#endif
