// Copyright 2026 TOPSBOT contributors.
// SPDX-License-Identifier: Apache-2.0

#ifndef TOPSBOT_WEBVIZ__CORE_MATCH_QUEUE_HPP_
#define TOPSBOT_WEBVIZ__CORE_MATCH_QUEUE_HPP_

#include <cstddef>
#include <mutex>
#include <optional>
#include <queue>
#include <vector>

#include "topsbot_webviz/core/types.hpp"

namespace topsbot_webviz
{

class MatchQueue
{
public:
  struct ImageCompare
  {
    bool operator()(const EncodedImageFrame & a, const EncodedImageFrame & b) const
    {
      return a.stamp_ns > b.stamp_ns;
    }
  };

  struct DetectionCompare
  {
    bool operator()(const DetectionFrame & a, const DetectionFrame & b) const
    {
      return a.stamp_ns > b.stamp_ns;
    }
  };

  explicit MatchQueue(size_t max_size, StampAlign align);

  void push_image(EncodedImageFrame frame);
  void push_detection(DetectionFrame frame);
  std::optional<MatchedFrame> try_match();
  size_t image_depth() const;
  size_t detection_depth() const;

private:
  void trim_locked();

  const size_t max_size_;
  const StampAlign align_;
  mutable std::mutex mutex_;
  std::priority_queue<EncodedImageFrame, std::vector<EncodedImageFrame>, ImageCompare> images_;
  std::priority_queue<DetectionFrame, std::vector<DetectionFrame>, DetectionCompare> detections_;
};

}  // namespace topsbot_webviz

#endif  // TOPSBOT_WEBVIZ__CORE_MATCH_QUEUE_HPP_
