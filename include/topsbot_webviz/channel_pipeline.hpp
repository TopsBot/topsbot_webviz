// Copyright 2026 TOPSBOT contributors.
// SPDX-License-Identifier: Apache-2.0

#ifndef TOPSBOT_WEBVIZ__CHANNEL_PIPELINE_HPP_
#define TOPSBOT_WEBVIZ__CHANNEL_PIPELINE_HPP_

#include <array>
#include <atomic>
#include <memory>
#include <string>
#include <thread>

#include "builtin_interfaces/msg/time.hpp"
#include "rclcpp/rclcpp.hpp"

#include "topsbot_webviz/core/config.hpp"
#include "topsbot_webviz/core/match_queue.hpp"
#include "topsbot_webviz/core/types.hpp"
#include "topsbot_webviz/encoder/nv12_to_jpeg.hpp"
#include "topsbot_webviz/transport/ws_server.hpp"

namespace topsbot_webviz
{

class ChannelPipeline
{
public:
  ChannelPipeline(
    rclcpp::Node & node,
    const ChannelConfig & channel,
    const WebvizConfig & config,
    WsServer & ws_server);
  ~ChannelPipeline();

  void start();
  void stop();

private:
  void setup_subscriptions();
  void on_tb_img_frame(
    const uint8_t * data, uint32_t data_size, uint32_t width, uint32_t height,
    const std::array<uint8_t, 12> & encoding_field,
    const builtin_interfaces::msg::Time & stamp);
  void process_loop();
  void on_encoded_image(EncodedImageFrame frame);
  void on_detection(DetectionFrame frame);
  void publish_frame(const MatchedFrame & matched);
  std::string serialize_frame(const MatchedFrame & matched, uint64_t sequence_id) const;

  rclcpp::Node & node_;
  ChannelConfig channel_;
  WebvizConfig config_;
  MatchQueue match_queue_;
  Nv12ToJpegEncoder encoder_;
  WsServer & ws_server_;

  rclcpp::TimerBase::SharedPtr health_timer_;
  rclcpp::SubscriptionBase::SharedPtr image_sub_;
  rclcpp::SubscriptionBase::SharedPtr detection_sub_;

  std::thread worker_;
  std::atomic<bool> running_{false};
  std::atomic<uint64_t> last_image_ms_{0};
  std::atomic<uint64_t> last_detection_ms_{0};
  std::atomic<bool> logged_first_image_{false};
  std::atomic<bool> logged_first_emit_{false};
  uint64_t start_ms_{0};
  uint64_t sequence_id_{0};
};

}  // namespace topsbot_webviz

#endif  // TOPSBOT_WEBVIZ__CHANNEL_PIPELINE_HPP_
