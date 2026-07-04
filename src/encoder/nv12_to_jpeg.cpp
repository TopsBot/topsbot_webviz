// Copyright 2026 TOPSBOT contributors.
// SPDX-License-Identifier: Apache-2.0

#include "topsbot_webviz/encoder/nv12_to_jpeg.hpp"

#include <sys/mman.h>
#include <unistd.h>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#if defined(TOPSBOT_WEBVIZ_HAS_TACV)
#include "ta_cv_api_ext_c.h"
#endif

namespace topsbot_webviz
{

namespace
{

bool encode_with_opencv_nv12(
  const uint8_t * nv12, const uint32_t width, const uint32_t height, const int quality,
  std::vector<uint8_t> & jpeg_out)
{
  const cv::Mat yuv(
    static_cast<int>(height * 3 / 2), static_cast<int>(width), CV_8UC1,
    const_cast<uint8_t *>(nv12));
  cv::Mat bgr;
  cv::cvtColor(yuv, bgr, cv::COLOR_YUV2BGR_NV12);
  std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, quality};
  return cv::imencode(".jpg", bgr, jpeg_out, params);
}

}  // namespace

#if defined(TOPSBOT_WEBVIZ_HAS_TACV)

Nv12ToJpegEncoder::~Nv12ToJpegEncoder()
{
  release_jpeg_enc_fd();
}

void Nv12ToJpegEncoder::release_jpeg_enc_fd()
{
  if (jpeg_enc_mapped_ != nullptr && jpeg_enc_mapped_ != MAP_FAILED) {
    munmap(jpeg_enc_mapped_, jpeg_enc_size_);
  }
  jpeg_enc_mapped_ = nullptr;
  if (jpeg_enc_fd_ >= 0) {
    close(jpeg_enc_fd_);
    jpeg_enc_fd_ = -1;
    jpeg_enc_size_ = 0;
  }
}

int Nv12ToJpegEncoder::ensure_jpeg_enc_fd(const size_t size)
{
  if (jpeg_enc_fd_ >= 0 && jpeg_enc_size_ == size && jpeg_enc_mapped_ != nullptr) {
    return jpeg_enc_fd_;
  }
  release_jpeg_enc_fd();
  jpeg_enc_fd_ = alloc_.alloc(size);
  if (jpeg_enc_fd_ < 0) {
    return -1;
  }
  jpeg_enc_mapped_ = mmap(
    nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, jpeg_enc_fd_, 0);
  if (jpeg_enc_mapped_ == MAP_FAILED) {
    release_jpeg_enc_fd();
    return -1;
  }
  jpeg_enc_size_ = size;
  return jpeg_enc_fd_;
}

bool Nv12ToJpegEncoder::encode_tacv(
  const uint8_t * nv12, const uint32_t width, const uint32_t height, const int quality,
  std::vector<uint8_t> & jpeg_out)
{
  const int w = static_cast<int>(width);
  const int h = static_cast<int>(height);
  if (w <= 0 || h <= 0) {
    return false;
  }

  if (w != last_width_ || h != last_height_ || nv12_image_.fd() < 0) {
    nv12_image_.reset();
    nv12_image_ = TaImage::create_nv12(alloc_, w, h);
    last_width_ = w;
    last_height_ = h;
  }
  if (nv12_image_.fd() < 0 || !nv12_image_.upload_nv12(nv12, w, h)) {
    return false;
  }

  const size_t buf_size = nv12_image_.byte_size();
  const int enc_fd = ensure_jpeg_enc_fd(buf_size);
  if (enc_fd < 0 || jpeg_enc_mapped_ == nullptr) {
    return false;
  }

  size_t jpeg_size = buf_size;
  ta_image_t src = nv12_image_.native();
  const auto ret = ta_cv_image_jpeg_enc(
    &src, static_cast<unsigned int>(enc_fd), &jpeg_size, quality);
  if (ret != TACV_SUCCESS || jpeg_size == 0 || jpeg_size > buf_size) {
    return false;
  }

  jpeg_out.assign(
    static_cast<const uint8_t *>(jpeg_enc_mapped_),
    static_cast<const uint8_t *>(jpeg_enc_mapped_) + jpeg_size);
  return true;
}
#endif

bool Nv12ToJpegEncoder::encode(
  const uint8_t * nv12, const uint32_t width, const uint32_t height, const int quality,
  std::vector<uint8_t> & jpeg_out)
{
  if (nv12 == nullptr || width == 0 || height == 0) {
    return false;
  }

#if defined(TOPSBOT_WEBVIZ_HAS_TACV)
  if (encode_tacv(nv12, width, height, quality, jpeg_out)) {
    return true;
  }
#endif

  return encode_with_opencv_nv12(nv12, width, height, quality, jpeg_out);
}

bool Nv12ToJpegEncoder::encode_rgb(
  const uint8_t * rgb, const uint32_t width, const uint32_t height, const int quality,
  std::vector<uint8_t> & jpeg_out) const
{
  if (rgb == nullptr || width == 0 || height == 0) {
    return false;
  }
  const cv::Mat rgb_mat(
    static_cast<int>(height), static_cast<int>(width), CV_8UC3,
    const_cast<uint8_t *>(rgb));
  cv::Mat bgr;
  cv::cvtColor(rgb_mat, bgr, cv::COLOR_RGB2BGR);
  std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, quality};
  return cv::imencode(".jpg", bgr, jpeg_out, params);
}

}  // namespace topsbot_webviz
