// Copyright 2026 TOPSBOT contributors.
// SPDX-License-Identifier: Apache-2.0

#include "topsbot_webviz/core/rate_limiter.hpp"

#include <algorithm>
#include <cmath>

#include "utils/time_helper.h"

namespace topsbot_webviz
{

AdaptiveRateLimiter::AdaptiveRateLimiter(const WebvizConfig & config)
: config_(config)
{
}

void AdaptiveRateLimiter::set_focus_channel(const int channel_id)
{
  std::lock_guard<std::mutex> lock(mutex_);
  focus_channel_ = channel_id;
}

void AdaptiveRateLimiter::set_override_fps(const uint32_t channel_id, const double fps)
{
  std::lock_guard<std::mutex> lock(mutex_);
  override_fps_[channel_id] = fps;
}

void AdaptiveRateLimiter::update_jpeg_size(const uint32_t channel_id, const size_t bytes)
{
  const double kbps = static_cast<double>(bytes) * 8.0 / 1000.0;
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = ema_kbps_.find(channel_id);
  if (it == ema_kbps_.end()) {
    ema_kbps_[channel_id] = kbps;
  } else {
    it->second = 0.8 * it->second + 0.2 * kbps;
  }
}

bool AdaptiveRateLimiter::should_emit(
  const uint32_t channel_id, const ChannelConfig & channel,
  const uint32_t width, const uint32_t height)
{
  (void)channel;
  const uint64_t now = now_ms();
  std::lock_guard<std::mutex> lock(mutex_);
  const double fps = target_fps_unlocked(channel_id, width, height, config_.channels.size());
  if (fps <= 0.0) {
    last_emit_ms_[channel_id] = now;
    return true;
  }
  const uint64_t min_interval_ms = static_cast<uint64_t>(1000.0 / fps);
  const auto it = last_emit_ms_.find(channel_id);
  if (it != last_emit_ms_.end() && now - it->second < min_interval_ms) {
    return false;
  }
  last_emit_ms_[channel_id] = now;
  return true;
}

double AdaptiveRateLimiter::target_fps(
  const uint32_t channel_id, const uint32_t width, const uint32_t height,
  const size_t active_channels) const
{
  std::lock_guard<std::mutex> lock(mutex_);
  return target_fps_unlocked(channel_id, width, height, active_channels);
}

double AdaptiveRateLimiter::channel_cap_fps(const ChannelConfig * channel, const bool focus) const
{
  if (channel == nullptr) {
    return 0.0;
  }
  if (channel->output_fps > 0.0) {
    return channel->output_fps;
  }
  const double cap = focus ? channel->fps.focus_max : channel->fps.grid_max;
  return cap > 0.0 ? cap : 0.0;
}

double AdaptiveRateLimiter::tier_fps(
  const uint32_t width, const uint32_t height, const bool focus) const
{
  const uint64_t pixels = static_cast<uint64_t>(width) * static_cast<uint64_t>(height);
  for (const auto & tier : config_.fps_tiers) {
    const uint64_t tier_pixels = static_cast<uint64_t>(tier.max_width) * tier.max_height;
    if (pixels <= tier_pixels) {
      return focus ? tier.focus_fps : tier.grid_fps;
    }
  }
  if (!config_.fps_tiers.empty()) {
    const auto & last = config_.fps_tiers.back();
    return focus ? last.focus_fps : last.grid_fps;
  }
  return focus ? 10.0 : 5.0;
}

double AdaptiveRateLimiter::target_fps_unlocked(
  const uint32_t channel_id, const uint32_t width, const uint32_t height,
  const size_t active_channels) const
{
  const auto override_it = override_fps_.find(channel_id);
  if (override_it != override_fps_.end() && override_it->second > 0.0) {
    return override_it->second;
  }

  const ChannelConfig * channel = nullptr;
  for (const auto & ch : config_.channels) {
    if (ch.id == channel_id) {
      channel = &ch;
      break;
    }
  }

  const bool focus = focus_channel_ >= 0 && static_cast<uint32_t>(focus_channel_) == channel_id;
  double fps = tier_fps(width, height, focus);
  const double cap = channel_cap_fps(channel, focus);
  if (cap > 0.0) {
    fps = std::min(fps, cap);
  }

  if (config_.max_total_kbps > 0.0 && active_channels > 0) {
    double total_kbps = 0.0;
    for (const auto & entry : ema_kbps_) {
      total_kbps += entry.second;
    }
    if (total_kbps <= 0.0) {
      total_kbps = static_cast<double>(width * height) / 15000.0 * 8.0;
    }
    const double budget_fps = config_.max_total_kbps / std::max(total_kbps, 1.0);
    fps = std::min(fps, budget_fps / static_cast<double>(active_channels));
  }
  return fps;
}

}  // namespace topsbot_webviz
