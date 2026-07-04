// Copyright 2026 TOPSBOT contributors.
// SPDX-License-Identifier: Apache-2.0

#include "topsbot_webviz/core/match_queue.hpp"

namespace topsbot_webviz
{

MatchQueue::MatchQueue(const size_t max_size, const StampAlign align)
: max_size_(max_size), align_(align)
{
}

void MatchQueue::push_image(EncodedImageFrame frame)
{
  std::lock_guard<std::mutex> lock(mutex_);
  images_.push(std::move(frame));
  trim_locked();
}

void MatchQueue::push_detection(DetectionFrame frame)
{
  std::lock_guard<std::mutex> lock(mutex_);
  detections_.push(std::move(frame));
  trim_locked();
}

std::optional<MatchedFrame> MatchQueue::try_match()
{
  std::lock_guard<std::mutex> lock(mutex_);

  if (align_ == StampAlign::kStrict) {
    // Same as hobot_websocket (aarch64): drop stale side until stamps equal.
    while (!images_.empty() && !detections_.empty()) {
      const uint64_t img_stamp = images_.top().stamp_ns;
      const uint64_t det_stamp = detections_.top().stamp_ns;
      if (img_stamp == det_stamp) {
        MatchedFrame matched;
        matched.image = images_.top();
        images_.pop();
        matched.detection = detections_.top().msg;
        detections_.pop();
        matched.has_detection = true;
        return matched;
      }
      if (img_stamp < det_stamp) {
        images_.pop();
      } else {
        detections_.pop();
      }
    }
    return std::nullopt;
  }

  if (images_.empty()) {
    return std::nullopt;
  }

  MatchedFrame matched;
  matched.image = images_.top();
  images_.pop();

  if (detections_.empty()) {
    matched.has_detection = false;
    return matched;
  }

  matched.detection = detections_.top().msg;
  detections_.pop();
  matched.has_detection = true;
  return matched;
}

size_t MatchQueue::image_depth() const
{
  std::lock_guard<std::mutex> lock(mutex_);
  return images_.size();
}

size_t MatchQueue::detection_depth() const
{
  std::lock_guard<std::mutex> lock(mutex_);
  return detections_.size();
}

void MatchQueue::trim_locked()
{
  while (images_.size() > max_size_) {
    images_.pop();
  }
  while (detections_.size() > max_size_) {
    detections_.pop();
  }
}

}  // namespace topsbot_webviz
