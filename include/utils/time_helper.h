// Copyright 2026 TOPSBOT contributors.
// SPDX-License-Identifier: Apache-2.0

#ifndef UTILS_TIME_HELPER_H_
#define UTILS_TIME_HELPER_H_

#include <cstdint>

namespace topsbot_webviz
{

uint64_t now_ms();

uint64_t stamp_to_ns(int32_t sec, uint32_t nanosec);

}  // namespace topsbot_webviz

#endif  // UTILS_TIME_HELPER_H_
