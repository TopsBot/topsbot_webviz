// Copyright 2026 TOPSBOT contributors.
// SPDX-License-Identifier: Apache-2.0

#ifndef TOPSBOT_WEBVIZ__ENCODER_DMABUF_ALLOCATOR_HPP_
#define TOPSBOT_WEBVIZ__ENCODER_DMABUF_ALLOCATOR_HPP_

#include <cstddef>

struct BufferAllocator;

namespace topsbot_webviz
{

class DmabufAllocator
{
public:
  DmabufAllocator() = default;
  ~DmabufAllocator();

  DmabufAllocator(const DmabufAllocator &) = delete;
  DmabufAllocator & operator=(const DmabufAllocator &) = delete;

  BufferAllocator * get();
  int alloc(size_t size, const char * heap = "carveout-heap");

private:
  bool ensure_initialized();
  BufferAllocator * allocator_{nullptr};
};

}  // namespace topsbot_webviz

#endif  // TOPSBOT_WEBVIZ__ENCODER_DMABUF_ALLOCATOR_HPP_
