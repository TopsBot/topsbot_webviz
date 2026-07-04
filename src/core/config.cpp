// Copyright 2026 TOPSBOT contributors.
// SPDX-License-Identifier: Apache-2.0

#include "topsbot_webviz/core/config.hpp"

#include <algorithm>

namespace topsbot_webviz
{

namespace
{

template<typename T>
T get_or(
  rclcpp::Node & node, const std::string & key, const T & default_value)
{
  if (node.has_parameter(key)) {
    return node.get_parameter(key).get_value<T>();
  }
  return default_value;
}

ChannelConfig load_single_channel(rclcpp::Node & node)
{
  ChannelConfig ch;
  ch.id = static_cast<uint32_t>(get_or<int64_t>(node, "channel_id", 0));
  ch.name = get_or<std::string>(node, "channel_name", "Cam-01");
  const auto input = load_channel_input(node, "", "/tbmem_jpeg");
  ch.input_topic = input.topic;
  ch.input_msg = input.msg;
  ch.tb_img_profile = input.tb_img_profile;
  ch.jpeg_quality = static_cast<int>(get_or<int64_t>(node, "jpeg_quality", 75));
  ch.detection_topic = get_or<std::string>(node, "detection_topic", "/detections");
  ch.detection_enabled = get_or<bool>(node, "detection_enabled", false);
  ch.only_show_image = get_or<bool>(node, "only_show_image", false);
  return ch;
}

ChannelConfig load_indexed_channel(rclcpp::Node & node, const int64_t index)
{
  const std::string p = "ch" + std::to_string(index) + "_";
  ChannelConfig ch;
  ch.id = static_cast<uint32_t>(get_or<int64_t>(node, p + "id", index));
  ch.name = get_or<std::string>(node, p + "name", "Cam-" + std::to_string(index + 1));
  const auto input = load_channel_input(node, p, "/tbmem_jpeg");
  ch.input_topic = input.topic;
  ch.input_msg = input.msg;
  ch.tb_img_profile = input.tb_img_profile;
  ch.jpeg_quality = static_cast<int>(get_or<int64_t>(node, p + "jpeg_quality", 75));
  ch.detection_topic = get_or<std::string>(node, p + "detection_topic", "/detections");
  ch.detection_enabled = get_or<bool>(node, p + "detection_enabled", false);
  ch.only_show_image = get_or<bool>(node, p + "only_show_image", false);
  return ch;
}

}  // namespace

WebvizConfig load_config_from_node(rclcpp::Node & node)
{
  WebvizConfig config;
  config.deployment = deployment_from_string(get_or<std::string>(node, "deployment", "multi_instance"));
  config.stamp_align = stamp_align_from_string(get_or<std::string>(node, "stamp_align", "strict"));
  config.ws_base_port = static_cast<int>(get_or<int64_t>(node, "ws_base_port", 8080));
  config.gateway_ws_port = static_cast<int>(get_or<int64_t>(node, "gateway_ws_port", 9090));
  config.queue_max = static_cast<size_t>(get_or<int64_t>(node, "queue_max", 50));

  const int64_t channel_count = get_or<int64_t>(node, "channel_count", 1);
  if (channel_count <= 1) {
    config.channels.push_back(load_single_channel(node));
  } else {
    for (int64_t i = 0; i < channel_count; ++i) {
      config.channels.push_back(load_indexed_channel(node, i));
    }
  }

  std::sort(
    config.channels.begin(), config.channels.end(),
    [](const ChannelConfig & a, const ChannelConfig & b) {return a.id < b.id;});
  return config;
}

}  // namespace topsbot_webviz
