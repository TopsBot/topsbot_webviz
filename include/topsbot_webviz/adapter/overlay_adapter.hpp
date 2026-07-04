// Copyright 2026 TOPSBOT contributors.
// SPDX-License-Identifier: Apache-2.0

#ifndef TOPSBOT_WEBVIZ__ADAPTER_OVERLAY_ADAPTER_HPP_
#define TOPSBOT_WEBVIZ__ADAPTER_OVERLAY_ADAPTER_HPP_

#include "topsbot_web.pb.h"
#include "tb_det_msgs/msg/tb_perception_targets.hpp"

namespace topsbot_webviz
{

class OverlayAdapter
{
public:
  static void fill_overlay(
    const tb_det_msgs::msg::TbPerceptionTargets & src,
    topsbot::web::OverlayPayload * dst);

  static void fill_stats(
    const tb_det_msgs::msg::TbPerceptionTargets & src,
    topsbot::web::SystemStats * dst);
};

}  // namespace topsbot_webviz

#endif  // TOPSBOT_WEBVIZ__ADAPTER_OVERLAY_ADAPTER_HPP_
