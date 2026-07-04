// Copyright 2026 TOPSBOT contributors.
// SPDX-License-Identifier: Apache-2.0

#ifndef TOPSBOT_WEBVIZ__ENCODER_NV12_TO_JPEG_HPP_
#define TOPSBOT_WEBVIZ__ENCODER_NV12_TO_JPEG_HPP_

#include <cstdint>
#include <vector>

#if defined(TOPSBOT_WEBVIZ_HAS_TACV)
#include "topsbot_webviz/encoder/dmabuf_allocator.hpp"
#include "topsbot_webviz/encoder/ta_image.hpp"
#endif

namespace topsbot_webviz
{

class Nv12ToJpegEncoder
{
public:
#if defined(TOPSBOT_WEBVIZ_HAS_TACV)
  Nv12ToJpegEncoder() = default;
  ~Nv12ToJpegEncoder();
#endif

  bool encode(
    const uint8_t * nv12, uint32_t width, uint32_t height, int quality,
    std::vector<uint8_t> & jpeg_out);

  bool encode_rgb(
    const uint8_t * rgb, uint32_t width, uint32_t height, int quality,
    std::vector<uint8_t> & jpeg_out) const;

private:
#if defined(TOPSBOT_WEBVIZ_HAS_TACV)
  bool encode_tacv(
    const uint8_t * nv12, uint32_t width, uint32_t height, int quality,
    std::vector<uint8_t> & jpeg_out);

  DmabufAllocator alloc_;
  TaImage nv12_image_;
  int jpeg_enc_fd_{-1};
  size_t jpeg_enc_size_{0};
  void * jpeg_enc_mapped_{nullptr};
  int last_width_{0};
  int last_height_{0};

  int ensure_jpeg_enc_fd(const size_t size);
  void release_jpeg_enc_fd();
#endif
};

}  // namespace topsbot_webviz

#endif  // TOPSBOT_WEBVIZ__ENCODER_NV12_TO_JPEG_HPP_
