// Copyright 2026 TOPSBOT contributors.
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "topsbot_webviz/core/match_queue.hpp"

TEST(MatchQueueTest, StrictRequiresEqualStamp)
{
  topsbot_webviz::MatchQueue queue(10, topsbot_webviz::StampAlign::kStrict);

  topsbot_webviz::EncodedImageFrame image;
  image.stamp_ns = 100;
  image.jpeg = {1, 2, 3};
  queue.push_image(image);

  topsbot_webviz::DetectionFrame detection;
  detection.stamp_ns = 200;
  queue.push_detection(detection);

  // Stale image (100) is dropped when detection is newer (200).
  EXPECT_FALSE(queue.try_match().has_value());

  image.stamp_ns = 100;
  queue.push_image(image);
  detection.stamp_ns = 100;
  queue.push_detection(detection);
  const auto matched = queue.try_match();
  ASSERT_TRUE(matched.has_value());
  EXPECT_TRUE(matched->has_detection);
  EXPECT_EQ(matched->image.stamp_ns, 100u);
}

TEST(MatchQueueTest, StrictDropsStaleDetection)
{
  topsbot_webviz::MatchQueue queue(10, topsbot_webviz::StampAlign::kStrict);

  topsbot_webviz::EncodedImageFrame image;
  image.stamp_ns = 300;
  image.jpeg = {1};
  queue.push_image(image);

  topsbot_webviz::DetectionFrame detection;
  detection.stamp_ns = 100;
  queue.push_detection(detection);

  detection.stamp_ns = 300;
  queue.push_detection(detection);

  const auto matched = queue.try_match();
  ASSERT_TRUE(matched.has_value());
  EXPECT_TRUE(matched->has_detection);
  EXPECT_EQ(matched->image.stamp_ns, 300u);
}

TEST(MatchQueueTest, StrictWaitsWhenOneQueueEmpty)
{
  topsbot_webviz::MatchQueue queue(10, topsbot_webviz::StampAlign::kStrict);

  topsbot_webviz::EncodedImageFrame image;
  image.stamp_ns = 100;
  image.jpeg = {1};
  queue.push_image(image);

  EXPECT_FALSE(queue.try_match().has_value());
}

TEST(MatchQueueTest, RelaxedPairsLatestDetection)
{
  topsbot_webviz::MatchQueue queue(10, topsbot_webviz::StampAlign::kRelaxed);

  topsbot_webviz::EncodedImageFrame image;
  image.stamp_ns = 50;
  image.jpeg = {9};
  queue.push_image(image);

  topsbot_webviz::DetectionFrame detection;
  detection.stamp_ns = 999;
  queue.push_detection(detection);

  const auto matched = queue.try_match();
  ASSERT_TRUE(matched.has_value());
  EXPECT_TRUE(matched->has_detection);
}
