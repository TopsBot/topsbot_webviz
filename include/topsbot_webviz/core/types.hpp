// Copyright 2026 TOPSBOT contributors.
// SPDX-License-Identifier: Apache-2.0

#ifndef TOPSBOT_WEBVIZ__CORE_TYPES_HPP_
#define TOPSBOT_WEBVIZ__CORE_TYPES_HPP_

#include <cstdint>
#include <string>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "tb_det_msgs/msg/tb_perception_targets.hpp"

namespace topsbot_webviz
{

/// Image subscription message family (TOPSBOT soft convention: input_msg).
enum class InputMsg
{
  TbJpeg,
  TbImg,
  SensorImage,
  SensorCompressed,
};

struct ChannelInput
{
  InputMsg msg{InputMsg::TbJpeg};
  std::string topic{"/tbmem_jpeg"};
  std::string tb_img_profile{"auto"};
};

InputMsg parse_input_msg(const std::string & value);
InputMsg parse_legacy_input_mode(const std::string & input_mode, bool input_compressed);
std::string input_msg_to_string(InputMsg msg);
std::string resolve_tb_img_profile(const std::string & profile, int width, int height);
ChannelInput load_channel_input(
  rclcpp::Node & node,
  const std::string & prefix,
  const std::string & default_topic);

enum class StampAlign
{
  kStrict,
  kRelaxed,
};

enum class DeploymentMode
{
  kMultiInstance,
  kGateway,
};

StampAlign stamp_align_from_string(const std::string & value);
DeploymentMode deployment_from_string(const std::string & value);

struct EncodedImageFrame
{
  uint64_t stamp_ns{0};
  uint32_t width{0};
  uint32_t height{0};
  std::vector<uint8_t> jpeg;
};

struct DetectionFrame
{
  uint64_t stamp_ns{0};
  tb_det_msgs::msg::TbPerceptionTargets msg;
};

struct MatchedFrame
{
  EncodedImageFrame image;
  tb_det_msgs::msg::TbPerceptionTargets detection;
  bool has_detection{false};
};

struct ChannelFpsConfig
{
  double grid_max{0.0};
  double focus_max{0.0};
};

struct ChannelConfig
{
  uint32_t id{0};
  std::string name;
  std::string input_topic{"/tbmem_jpeg"};
  InputMsg input_msg{InputMsg::TbJpeg};
  std::string tb_img_profile{"auto"};
  int jpeg_quality{75};
  std::string detection_topic;
  bool detection_enabled{false};
  bool only_show_image{false};
  ChannelFpsConfig fps;
  double output_fps{0.0};
};

struct FpsTier
{
  uint32_t max_width{0};
  uint32_t max_height{0};
  double grid_fps{5.0};
  double focus_fps{10.0};
};

struct WebvizConfig
{
  DeploymentMode deployment{DeploymentMode::kMultiInstance};
  StampAlign stamp_align{StampAlign::kStrict};
  int ws_base_port{8080};
  int gateway_ws_port{9090};
  size_t queue_max{50};
  double max_total_kbps{0.0};
  std::vector<FpsTier> fps_tiers;
  std::vector<ChannelConfig> channels;
};

}  // namespace topsbot_webviz

#endif  // TOPSBOT_WEBVIZ__CORE_TYPES_HPP_
