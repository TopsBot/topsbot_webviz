// Copyright 2026 TOPSBOT contributors.
// SPDX-License-Identifier: Apache-2.0

#include "topsbot_webviz/core/types.hpp"

#include <algorithm>
#include <cctype>

namespace topsbot_webviz
{

namespace
{

std::string to_lower(std::string value)
{
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
      return static_cast<char>(std::tolower(c));
    });
  return value;
}

template<typename T>
T get_or(
  rclcpp::Node & node, const std::string & key, const T & default_value)
{
  if (node.has_parameter(key)) {
    return node.get_parameter(key).get_value<T>();
  }
  return default_value;
}

std::string get_or_string(
  rclcpp::Node & node, const std::string & key, const std::string & default_value)
{
  if (node.has_parameter(key)) {
    const auto value = node.get_parameter(key).get_value<std::string>();
    if (!value.empty()) {
      return value;
    }
  }
  return default_value;
}

constexpr int kTbImgMax480 = 921600;
constexpr int kTbImgMax540 = 1555200;

}  // namespace

InputMsg parse_input_msg(const std::string & value)
{
  const std::string v = to_lower(value);
  if (v == "tb_jpeg" || v == "subscription_jpeg_zc") {
    return InputMsg::TbJpeg;
  }
  if (v == "tb_img" || v == "subscription_zc" || v == "tbmem") {
    return InputMsg::TbImg;
  }
  if (v == "sensor_image" || v == "sensor") {
    return InputMsg::SensorImage;
  }
  if (v == "sensor_compressed" || v == "compressed") {
    return InputMsg::SensorCompressed;
  }
  if (v == "subscription" || v == "topic" || v == "ros") {
    return InputMsg::SensorImage;
  }
  return InputMsg::SensorImage;
}

InputMsg parse_legacy_input_mode(const std::string & input_mode, const bool input_compressed)
{
  if (to_lower(input_mode) == "subscription" && input_compressed) {
    return InputMsg::SensorCompressed;
  }
  return parse_input_msg(input_mode);
}

std::string input_msg_to_string(const InputMsg msg)
{
  switch (msg) {
    case InputMsg::TbJpeg:
      return "tb_jpeg";
    case InputMsg::TbImg:
      return "tb_img";
    case InputMsg::SensorImage:
      return "sensor_image";
    case InputMsg::SensorCompressed:
      return "sensor_compressed";
    default:
      return "sensor_image";
  }
}

std::string resolve_tb_img_profile(const std::string & profile, int width, int height)
{
  const std::string v = to_lower(profile);
  if (!v.empty() && v != "auto") {
    if (v == "tbmsg480p") {
      return "480p";
    }
    if (v == "tbmsg540p") {
      return "540p";
    }
    if (v == "tbmsg1080p") {
      return "1080p";
    }
    return v;
  }
  const int payload = std::max(width, 1) * std::max(height, 1) * 3;
  // TbMsg* MAX_SIZE envelope is W×H×3 (RGB24); NV12 shares tier but is smaller.
  if (payload <= kTbImgMax480) {
    return "480p";
  }
  if (payload <= kTbImgMax540) {
    return "540p";
  }
  return "1080p";
}

ChannelInput load_channel_input(
  rclcpp::Node & node,
  const std::string & prefix,
  const std::string & default_topic)
{
  ChannelInput result;
  result.topic = default_topic;
  result.tb_img_profile = "auto";
  result.msg = InputMsg::TbJpeg;

  const std::string input_msg_str = get_or_string(node, prefix + "input_msg", "");
  if (!input_msg_str.empty()) {
    result.msg = parse_input_msg(input_msg_str);
  } else {
    const std::string input_mode_str = get_or_string(node, prefix + "input_mode", "");
    const bool input_compressed = get_or<bool>(node, prefix + "input_compressed", false);
    if (!input_mode_str.empty()) {
      result.msg = parse_legacy_input_mode(input_mode_str, input_compressed);
      RCLCPP_WARN_ONCE(
        node.get_logger(),
        "Deprecated %sinput_mode / %sinput_compressed; prefer %sinput_msg",
        prefix.c_str(), prefix.c_str(), prefix.c_str());
    }
  }

  result.topic = get_or_string(node, prefix + "input_topic", "");
  if (result.topic.empty()) {
    result.topic = get_or_string(node, prefix + "image_topic", default_topic);
  }
  result.tb_img_profile = get_or_string(node, prefix + "tb_img_profile", "auto");
  return result;
}

StampAlign stamp_align_from_string(const std::string & value)
{
  return to_lower(value) == "relaxed" ? StampAlign::kRelaxed : StampAlign::kStrict;
}

DeploymentMode deployment_from_string(const std::string & value)
{
  return to_lower(value) == "gateway" ? DeploymentMode::kGateway : DeploymentMode::kMultiInstance;
}

}  // namespace topsbot_webviz
