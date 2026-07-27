// Copyright 2026 TOPSBOT contributors.
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "tb_det_msgs/msg/tb_perception_targets.hpp"
#include "tb_det_msgs/msg/tb_point.hpp"
#include "tb_det_msgs/msg/tb_roi.hpp"
#include "tb_det_msgs/msg/tb_target.hpp"
#include "geometry_msgs/msg/point32.hpp"
#include "topsbot_webviz/adapter/overlay_adapter.hpp"

TEST(OverlayAdapterTest, MapsTargetsAndBoxes)
{
  tb_det_msgs::msg::TbPerceptionTargets src;
  src.fps = 30;
  tb_det_msgs::msg::TbTarget target;
  target.type = "person";
  target.track_id = 0;
  tb_det_msgs::msg::TbRoi roi;
  roi.type = "body";
  roi.rect.x_offset = 10;
  roi.rect.y_offset = 20;
  roi.rect.width = 30;
  roi.rect.height = 40;
  roi.confidence = 0.9f;
  target.rois.push_back(roi);

  tb_det_msgs::msg::TbPoint kps;
  kps.type = "body_kps";
  geometry_msgs::msg::Point32 p0;
  p0.x = 15.0f;
  p0.y = 25.0f;
  kps.point.push_back(p0);
  kps.confidence.push_back(0.8f);
  target.points.push_back(kps);

  src.targets.push_back(target);

  topsbot::web::OverlayPayload overlay;
  topsbot::web::SystemStats stats;
  topsbot_webviz::OverlayAdapter::fill_overlay(src, &overlay);
  topsbot_webviz::OverlayAdapter::fill_stats(src, &stats);

  ASSERT_EQ(overlay.targets_size(), 1);
  EXPECT_EQ(overlay.targets(0).type(), "person");
  ASSERT_EQ(overlay.targets(0).boxes_size(), 1);
  EXPECT_FLOAT_EQ(overlay.targets(0).boxes(0).score(), 0.9f);
  ASSERT_EQ(overlay.targets(0).points_size(), 1);
  EXPECT_EQ(overlay.targets(0).points(0).type(), "body_kps");
  ASSERT_EQ(overlay.targets(0).points(0).points_size(), 1);
  EXPECT_FLOAT_EQ(overlay.targets(0).points(0).points(0).x(), 15.0f);
  ASSERT_GE(stats.items_size(), 1);
}
