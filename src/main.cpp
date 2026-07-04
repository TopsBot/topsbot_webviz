// Copyright 2026 TOPSBOT contributors.
// SPDX-License-Identifier: Apache-2.0

#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "topsbot_webviz/webviz_node.hpp"

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<topsbot_webviz::WebvizNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
