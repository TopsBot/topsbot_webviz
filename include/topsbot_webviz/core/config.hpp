// Copyright 2026 TOPSBOT contributors.
// SPDX-License-Identifier: Apache-2.0

#ifndef TOPSBOT_WEBVIZ__CORE_CONFIG_HPP_
#define TOPSBOT_WEBVIZ__CORE_CONFIG_HPP_

#include "topsbot_webviz/core/types.hpp"

#include "rclcpp/rclcpp.hpp"

namespace topsbot_webviz
{

WebvizConfig load_config_from_node(rclcpp::Node & node);

}  // namespace topsbot_webviz

#endif  // TOPSBOT_WEBVIZ__CORE_CONFIG_HPP_
