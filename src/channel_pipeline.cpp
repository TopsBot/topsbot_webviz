// Copyright 2026 TOPSBOT contributors.
// SPDX-License-Identifier: Apache-2.0

#include "topsbot_webviz/channel_pipeline.hpp"

#include <cstring>

#include "sensor_msgs/msg/compressed_image.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "tb_det_msgs/msg/tb_perception_targets.hpp"
#include "tb_img_msgs/msg/tb_jpeg_frame.hpp"
#include "tb_img_msgs/msg/tb_msg1080_p.hpp"
#include "tb_img_msgs/msg/tb_msg480_p.hpp"
#include "tb_img_msgs/msg/tb_msg540_p.hpp"

#include "topsbot_web.pb.h"
#include "topsbot_webviz/core/types.hpp"
#include "topsbot_webviz/adapter/overlay_adapter.hpp"
#include "utils/time_helper.h"

namespace topsbot_webviz
{

namespace
{

rclcpp::QoS sensor_qos()
{
  return rclcpp::SensorDataQoS().keep_last(10);
}

bool is_nv12_encoding(const std::string & encoding)
{
  return encoding == "nv12" || encoding == "NV12";
}

bool is_rgb_encoding(const std::string & encoding)
{
  return encoding == "rgb8" || encoding == "RGB8";
}

std::string tb_encoding_to_string(const std::array<uint8_t, 12> & raw)
{
  const auto end = std::find(raw.begin(), raw.end(), static_cast<uint8_t>(0));
  return std::string(raw.begin(), end);
}

std::string effective_tb_img_profile(const ChannelConfig & channel, rclcpp::Logger logger)
{
  std::string profile = channel.tb_img_profile;
  if (profile == "auto") {
    RCLCPP_WARN(
      logger,
      "Channel %u: tb_img_profile=auto defaulting to 480p; "
      "set tb_img_profile in yaml or resolve via launch",
      channel.id);
    profile = "480p";
  }
  return resolve_tb_img_profile(profile, 640, 480);
}

size_t expected_nv12_size(const uint32_t width, const uint32_t height)
{
  return static_cast<size_t>(width) * static_cast<size_t>(height) * 3U / 2U;
}

}  // namespace

ChannelPipeline::ChannelPipeline(
  rclcpp::Node & node,
  const ChannelConfig & channel,
  const WebvizConfig & config,
  WsServer & ws_server)
: node_(node),
  channel_(channel),
  config_(config),
  match_queue_(
    config.queue_max,
    (channel.detection_enabled && !channel.only_show_image)
    ? config.stamp_align
    : StampAlign::kRelaxed),
  encoder_(config.enable_tacv_jpeg),
  ws_server_(ws_server)
{
}

ChannelPipeline::~ChannelPipeline()
{
  stop();
}

void ChannelPipeline::start()
{
  start_ms_ = now_ms();
  setup_subscriptions();
  running_ = true;
  worker_ = std::thread(&ChannelPipeline::process_loop, this);

  health_timer_ = node_.create_wall_timer(
    std::chrono::seconds(5),
    [this]() {
      const auto now = now_ms();
      if (last_image_ms_.load() == 0 && now - start_ms_ >= 5000) {
        RCLCPP_WARN(
          node_.get_logger(),
          "Channel %u: no image received on %s yet (check usb_cam / topic name)",
          channel_.id, channel_.input_topic.c_str());
      } else if (last_image_ms_.load() > 0 && now - last_image_ms_.load() >= 5000) {
        RCLCPP_WARN(
          node_.get_logger(),
          "Channel %u: no image on %s for 5s",
          channel_.id, channel_.input_topic.c_str());
      }
      if (channel_.detection_enabled && last_detection_ms_.load() > 0 &&
        now - last_detection_ms_.load() >= 5000)
      {
        RCLCPP_WARN(
          node_.get_logger(),
          "Channel %u: no detection on %s for 5s",
          channel_.id, channel_.detection_topic.c_str());
      }
      const size_t depth = match_queue_.image_depth();
      if (depth > config_.queue_max / 2) {
        RCLCPP_WARN(
          node_.get_logger(),
          "Channel %u: image queue depth %zu",
          channel_.id, depth);
      }
    });
}

void ChannelPipeline::stop()
{
  running_ = false;
  if (worker_.joinable()) {
    worker_.join();
  }
  health_timer_.reset();
  image_sub_.reset();
  detection_sub_.reset();
}

void ChannelPipeline::setup_subscriptions()
{
  const auto qos = sensor_qos();

  switch (channel_.input_msg) {
    case InputMsg::TbJpeg:
      RCLCPP_INFO(
        node_.get_logger(), "Channel %u: subscribe TbJpegFrame on %s",
        channel_.id, channel_.input_topic.c_str());
      image_sub_ = node_.create_subscription<tb_img_msgs::msg::TbJpegFrame>(
        channel_.input_topic, qos,
        [this](tb_img_msgs::msg::TbJpegFrame::ConstSharedPtr msg) {
          if (msg->data_size == 0) {
            return;
          }
          EncodedImageFrame frame;
          frame.stamp_ns = stamp_to_ns(msg->time_stamp.sec, msg->time_stamp.nanosec);
          frame.width = msg->width;
          frame.height = msg->height;
          frame.jpeg.assign(msg->data.begin(), msg->data.begin() + msg->data_size);
          on_encoded_image(std::move(frame));
        });
      break;

    case InputMsg::TbImg: {
        const std::string profile = effective_tb_img_profile(channel_, node_.get_logger());
        if (profile == "540p") {
          RCLCPP_INFO(
            node_.get_logger(), "Channel %u: subscribe TbMsg540P on %s",
            channel_.id, channel_.input_topic.c_str());
          image_sub_ = node_.create_subscription<tb_img_msgs::msg::TbMsg540P>(
            channel_.input_topic, qos,
            [this](tb_img_msgs::msg::TbMsg540P::ConstSharedPtr msg) {
              on_tb_img_frame(msg->data.data(), msg->data_size, msg->width, msg->height,
                msg->encoding, msg->time_stamp);
            });
        } else if (profile == "1080p") {
          RCLCPP_INFO(
            node_.get_logger(), "Channel %u: subscribe TbMsg1080P on %s",
            channel_.id, channel_.input_topic.c_str());
          image_sub_ = node_.create_subscription<tb_img_msgs::msg::TbMsg1080P>(
            channel_.input_topic, qos,
            [this](tb_img_msgs::msg::TbMsg1080P::ConstSharedPtr msg) {
              on_tb_img_frame(msg->data.data(), msg->data_size, msg->width, msg->height,
                msg->encoding, msg->time_stamp);
            });
        } else {
          RCLCPP_INFO(
            node_.get_logger(), "Channel %u: subscribe TbMsg480P on %s",
            channel_.id, channel_.input_topic.c_str());
          image_sub_ = node_.create_subscription<tb_img_msgs::msg::TbMsg480P>(
            channel_.input_topic, qos,
            [this](tb_img_msgs::msg::TbMsg480P::ConstSharedPtr msg) {
              on_tb_img_frame(msg->data.data(), msg->data_size, msg->width, msg->height,
                msg->encoding, msg->time_stamp);
            });
        }
        break;
      }

    case InputMsg::SensorCompressed:
      RCLCPP_INFO(
        node_.get_logger(), "Channel %u: subscribe CompressedImage on %s",
        channel_.id, channel_.input_topic.c_str());
      image_sub_ = node_.create_subscription<sensor_msgs::msg::CompressedImage>(
        channel_.input_topic, qos,
        [this](sensor_msgs::msg::CompressedImage::ConstSharedPtr msg) {
          if (msg->data.empty()) {
            return;
          }
          EncodedImageFrame frame;
          frame.stamp_ns = stamp_to_ns(msg->header.stamp.sec, msg->header.stamp.nanosec);
          frame.jpeg = msg->data;
          on_encoded_image(std::move(frame));
        });
      break;

    case InputMsg::SensorImage:
      RCLCPP_INFO(
        node_.get_logger(), "Channel %u: subscribe sensor_msgs/Image on %s",
        channel_.id, channel_.input_topic.c_str());
      image_sub_ = node_.create_subscription<sensor_msgs::msg::Image>(
        channel_.input_topic, qos,
        [this](sensor_msgs::msg::Image::ConstSharedPtr msg) {
          std::vector<uint8_t> jpeg;
          if (is_nv12_encoding(msg->encoding)) {
            const auto expected = expected_nv12_size(msg->width, msg->height);
            if (msg->data.size() < expected) {
              RCLCPP_WARN_THROTTLE(
                node_.get_logger(), *node_.get_clock(), 5000,
                "Channel %u: NV12 payload %zu < expected %zu",
                channel_.id, msg->data.size(), expected);
              return;
            }
            if (!encoder_.encode(
                msg->data.data(), msg->width, msg->height, channel_.jpeg_quality, jpeg))
            {
              RCLCPP_WARN(node_.get_logger(), "Channel %u: NV12 encode failed", channel_.id);
              return;
            }
          } else if (is_rgb_encoding(msg->encoding)) {
            if (!encoder_.encode_rgb(
                msg->data.data(), msg->width, msg->height, channel_.jpeg_quality, jpeg))
            {
              RCLCPP_WARN(node_.get_logger(), "Channel %u: RGB encode failed", channel_.id);
              return;
            }
          } else {
            RCLCPP_WARN_THROTTLE(
              node_.get_logger(), *node_.get_clock(), 5000,
              "Channel %u: unsupported sensor_msgs encoding '%s'",
              channel_.id, msg->encoding.c_str());
            return;
          }
          EncodedImageFrame frame;
          frame.stamp_ns = stamp_to_ns(msg->header.stamp.sec, msg->header.stamp.nanosec);
          frame.width = msg->width;
          frame.height = msg->height;
          frame.jpeg = std::move(jpeg);
          on_encoded_image(std::move(frame));
        });
      break;

    default:
      throw std::runtime_error(
        "unsupported input_msg for channel " + std::to_string(channel_.id));
  }

  if (channel_.detection_enabled && !channel_.only_show_image) {
    detection_sub_ = node_.create_subscription<tb_det_msgs::msg::TbPerceptionTargets>(
      channel_.detection_topic, qos,
      [this](tb_det_msgs::msg::TbPerceptionTargets::ConstSharedPtr msg) {
        DetectionFrame frame;
        frame.stamp_ns = stamp_to_ns(msg->header.stamp.sec, msg->header.stamp.nanosec);
        frame.msg = *msg;
        on_detection(std::move(frame));
      });
  }
}

void ChannelPipeline::on_tb_img_frame(
  const uint8_t * data, uint32_t data_size, uint32_t width, uint32_t height,
  const std::array<uint8_t, 12> & encoding_field,
  const builtin_interfaces::msg::Time & stamp)
{
  const std::string encoding = tb_encoding_to_string(encoding_field);
  std::vector<uint8_t> jpeg;
  if (is_nv12_encoding(encoding)) {
    const auto expected = expected_nv12_size(width, height);
    if (data_size < expected) {
      RCLCPP_WARN_THROTTLE(
        node_.get_logger(), *node_.get_clock(), 5000,
        "Channel %u: TbMsg NV12 payload %u < expected %zu",
        channel_.id, data_size, expected);
      return;
    }
    if (!encoder_.encode(data, width, height, channel_.jpeg_quality, jpeg)) {
      RCLCPP_WARN(node_.get_logger(), "Channel %u: TbMsg NV12 encode failed", channel_.id);
      return;
    }
  } else if (is_rgb_encoding(encoding)) {
    if (!encoder_.encode_rgb(data, width, height, channel_.jpeg_quality, jpeg)) {
      RCLCPP_WARN(node_.get_logger(), "Channel %u: TbMsg RGB encode failed", channel_.id);
      return;
    }
  } else {
    RCLCPP_WARN_THROTTLE(
      node_.get_logger(), *node_.get_clock(), 5000,
      "Channel %u: unsupported TbMsg encoding '%s' (data_size=%u)",
      channel_.id, encoding.c_str(), data_size);
    return;
  }
  EncodedImageFrame frame;
  frame.stamp_ns = stamp_to_ns(stamp.sec, stamp.nanosec);
  frame.width = width;
  frame.height = height;
  frame.jpeg = std::move(jpeg);
  on_encoded_image(std::move(frame));
}

void ChannelPipeline::process_loop()
{
  while (running_) {
    auto matched = match_queue_.try_match();
    if (!matched.has_value()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
      continue;
    }
    if (channel_.detection_enabled && !channel_.only_show_image &&
      !matched->has_detection)
    {
      continue;
    }
    publish_frame(*matched);
  }
}

void ChannelPipeline::on_encoded_image(EncodedImageFrame frame)
{
  last_image_ms_.store(now_ms());
  if (!logged_first_image_.exchange(true)) {
    RCLCPP_INFO(
      node_.get_logger(),
      "Channel %u: first image %ux%u jpeg=%zu bytes",
      channel_.id, frame.width, frame.height, frame.jpeg.size());
  }
  match_queue_.push_image(std::move(frame));
}

void ChannelPipeline::on_detection(DetectionFrame frame)
{
  last_detection_ms_.store(now_ms());
  match_queue_.push_detection(std::move(frame));
}

void ChannelPipeline::publish_frame(const MatchedFrame & matched)
{
  const std::string payload = serialize_frame(matched, sequence_id_++);
  if (payload.empty()) {
    return;
  }
  if (!logged_first_emit_.exchange(true)) {
    RCLCPP_INFO(
      node_.get_logger(),
      "Channel %u: first WebSocket frame %zu bytes (jpeg=%zu)",
      channel_.id, payload.size(), matched.image.jpeg.size());
  }
  ws_server_.broadcast(payload);
}

std::string ChannelPipeline::serialize_frame(
  const MatchedFrame & matched, const uint64_t sequence_id) const
{
  topsbot::web::WebFrameMessage frame;
  frame.set_channel_id(channel_.id);
  frame.set_timestamp_ns(matched.image.stamp_ns);
  frame.set_sequence_id(sequence_id);
  frame.mutable_image()->set_jpeg(
    matched.image.jpeg.data(), matched.image.jpeg.size());
  frame.mutable_image()->set_width(matched.image.width);
  frame.mutable_image()->set_height(matched.image.height);
  frame.mutable_image()->set_encoding("jpeg");

  if (matched.has_detection) {
    OverlayAdapter::fill_overlay(matched.detection, frame.mutable_overlay());
    OverlayAdapter::fill_stats(matched.detection, frame.mutable_stats());
  }
  return frame.SerializeAsString();
}

}  // namespace topsbot_webviz
