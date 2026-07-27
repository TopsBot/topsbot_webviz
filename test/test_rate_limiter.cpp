// Copyright 2026 TOPSBOT contributors.
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "topsbot_webviz/core/rate_limiter.hpp"

TEST(RateLimiterTest, TierSelectionUses1080pDefaults)
{
  topsbot_webviz::WebvizConfig config;
  topsbot_webviz::ChannelConfig channel;
  channel.id = 0;
  topsbot_webviz::AdaptiveRateLimiter limiter(config);

  const double fps = limiter.target_fps(0, 1920, 1080, 1);
  EXPECT_GT(fps, 0.0);
  EXPECT_LE(fps, 10.0);
}

TEST(RateLimiterTest, OverrideFpsWins)
{
  topsbot_webviz::WebvizConfig config;
  topsbot_webviz::ChannelConfig channel;
  channel.id = 1;
  config.channels.push_back(channel);
  topsbot_webviz::AdaptiveRateLimiter limiter(config);
  limiter.set_override_fps(1, 2.0);

  EXPECT_DOUBLE_EQ(limiter.target_fps(1, 640, 480, 1), 2.0);
}
