// Copyright 2026 TOPSBOT contributors.
// SPDX-License-Identifier: Apache-2.0

#ifndef TOPSBOT_WEBVIZ__CORE_RATE_LIMITER_HPP_
#define TOPSBOT_WEBVIZ__CORE_RATE_LIMITER_HPP_

#include <cstdint>
#include <mutex>
#include <unordered_map>

#include "topsbot_webviz/core/types.hpp"

namespace topsbot_webviz
{

class AdaptiveRateLimiter
{
public:
  explicit AdaptiveRateLimiter(const WebvizConfig & config);

  void set_focus_channel(int channel_id);
  void set_override_fps(uint32_t channel_id, double fps);
  void update_jpeg_size(uint32_t channel_id, size_t bytes);
  bool should_emit(uint32_t channel_id, const ChannelConfig & channel, uint32_t width, uint32_t height);
  double target_fps(
    uint32_t channel_id, uint32_t width, uint32_t height, size_t active_channels) const;

private:
  double channel_cap_fps(const ChannelConfig * channel, bool focus) const;
  double tier_fps(uint32_t width, uint32_t height, bool focus) const;
  double target_fps_unlocked(
    uint32_t channel_id, uint32_t width, uint32_t height, size_t active_channels) const;

  WebvizConfig config_;
  mutable std::mutex mutex_;
  int focus_channel_{-1};
  std::unordered_map<uint32_t, double> override_fps_;
  std::unordered_map<uint32_t, double> ema_kbps_;
  std::unordered_map<uint32_t, uint64_t> last_emit_ms_;
};

}  // namespace topsbot_webviz

#endif  // TOPSBOT_WEBVIZ__CORE_RATE_LIMITER_HPP_
