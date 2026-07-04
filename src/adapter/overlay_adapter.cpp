// Copyright 2026 TOPSBOT contributors.
// SPDX-License-Identifier: Apache-2.0

#include "topsbot_webviz/adapter/overlay_adapter.hpp"

namespace topsbot_webviz
{

void OverlayAdapter::fill_overlay(
  const tb_det_msgs::msg::TbPerceptionTargets & src,
  topsbot::web::OverlayPayload * dst)
{
  if (dst == nullptr) {
    return;
  }
  dst->clear_targets();
  for (const auto & target : src.targets) {
    auto * out = dst->add_targets();
    out->set_type(target.type);
    for (const auto & roi : target.rois) {
      auto * box = out->add_boxes();
      box->set_type(roi.type);
      box->set_x(static_cast<float>(roi.rect.x_offset));
      box->set_y(static_cast<float>(roi.rect.y_offset));
      box->set_w(static_cast<float>(roi.rect.width));
      box->set_h(static_cast<float>(roi.rect.height));
      box->set_score(roi.confidence);
    }
  }
}

void OverlayAdapter::fill_stats(
  const tb_det_msgs::msg::TbPerceptionTargets & src,
  topsbot::web::SystemStats * dst)
{
  if (dst == nullptr) {
    return;
  }
  dst->clear_items();
  if (src.fps > 0) {
    auto * fps_item = dst->add_items();
    fps_item->set_type("fps");
    fps_item->set_value(static_cast<float>(src.fps));
  }
  for (const auto & perf : src.perfs) {
    auto * item = dst->add_items();
    item->set_type(perf.type);
    item->set_value(static_cast<float>(perf.time_ms_duration));
  }
}

}  // namespace topsbot_webviz
