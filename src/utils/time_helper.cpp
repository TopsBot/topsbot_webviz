// Copyright 2026 TOPSBOT contributors.
// SPDX-License-Identifier: Apache-2.0

#include "utils/time_helper.h"

#include <chrono>

namespace topsbot_webviz
{

uint64_t now_ms()
{
  return static_cast<uint64_t>(
    std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count());
}

uint64_t stamp_to_ns(const int32_t sec, const uint32_t nanosec)
{
  return static_cast<uint64_t>(sec) * 1000000000ULL + static_cast<uint64_t>(nanosec);
}

}  // namespace topsbot_webviz
