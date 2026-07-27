// Copyright 2026 TOPSBOT contributors.
// SPDX-License-Identifier: Apache-2.0

#include "topsbot_webviz/webviz_node.hpp"

#include "topsbot_webviz/core/types.hpp"

namespace topsbot_webviz
{

WebvizNode::WebvizNode()
: Node(
    "webviz_node",
    rclcpp::NodeOptions().automatically_declare_parameters_from_overrides(true))
{
  config_ = load_config_from_node(*this);

  int ws_port = config_.gateway_ws_port;
  if (config_.deployment == DeploymentMode::kMultiInstance) {
    if (config_.channels.empty()) {
      throw std::runtime_error("webviz_node: no channels configured");
    }
    ws_port = config_.ws_base_port + static_cast<int>(config_.channels.front().id) * 2;
  }

  ws_server_ = std::make_unique<WsServer>(ws_port);
  ws_server_->start();

  RCLCPP_INFO(
    get_logger(),
    "webviz_node started deployment=%s channels=%zu ws_port=%d nv12_jpeg_encoder=%s",
    config_.deployment == DeploymentMode::kGateway ? "gateway" : "multi_instance",
    config_.channels.size(), ws_port,
    config_.enable_tacv_jpeg ? "ta-cv (OpenCV fallback)" : "OpenCV");

  for (const auto & channel : config_.channels) {
    RCLCPP_INFO(
      get_logger(),
      "Channel %u: input topic=%s msg=%s detection=%s",
      channel.id, channel.input_topic.c_str(),
      input_msg_to_string(channel.input_msg).c_str(),
      channel.detection_enabled ? "on" : "off");
    if (channel.input_msg == InputMsg::TbImg) {
      RCLCPP_INFO(
        get_logger(),
        "Channel %u: tb_img_profile=%s",
        channel.id, channel.tb_img_profile.c_str());
    }
    auto pipeline = std::make_unique<ChannelPipeline>(
      *this, channel, config_, *ws_server_);
    pipeline->start();
    pipelines_.push_back(std::move(pipeline));
  }
}

}  // namespace topsbot_webviz
