// Copyright 2026 TOPSBOT contributors.
// SPDX-License-Identifier: Apache-2.0

#ifndef TOPSBOT_WEBVIZ__WEBVIZ_NODE_HPP_
#define TOPSBOT_WEBVIZ__WEBVIZ_NODE_HPP_

#include <memory>
#include <vector>

#include "rclcpp/rclcpp.hpp"

#include "topsbot_webviz/channel_pipeline.hpp"
#include "topsbot_webviz/core/config.hpp"
#include "topsbot_webviz/transport/ws_server.hpp"

namespace topsbot_webviz
{

class WebvizNode : public rclcpp::Node
{
public:
  WebvizNode();

private:
  WebvizConfig config_;
  std::unique_ptr<WsServer> ws_server_;
  std::vector<std::unique_ptr<ChannelPipeline>> pipelines_;
};

}  // namespace topsbot_webviz

#endif  // TOPSBOT_WEBVIZ__WEBVIZ_NODE_HPP_
