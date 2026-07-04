// Copyright 2026 TOPSBOT contributors.
// SPDX-License-Identifier: Apache-2.0

#include "topsbot_webviz/transport/ws_server.hpp"

#include <cstring>

#include "libwebsockets.h"
#include "rclcpp/rclcpp.hpp"

namespace topsbot_webviz
{

namespace
{

constexpr int kServiceTimeoutMs = 50;

}  // namespace

WsServer::WsServer(const int port)
: port_(port)
{
}

WsServer::~WsServer()
{
  stop();
}

void WsServer::start()
{
  if (running_.exchange(true)) {
    return;
  }

  static struct lws_protocols protocols[] = {
    {
      "topsbot-webviz",
      WsServer::lws_callback,
      sizeof(PerSessionData),
      256 * 1024,
      0,
      nullptr,
      0,
    },
    {nullptr, nullptr, 0, 0, 0, nullptr, 0},
  };

  struct lws_context_creation_info info;
  std::memset(&info, 0, sizeof(info));
  info.port = port_;
  info.protocols = protocols;
  info.user = this;
  info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;

  context_ = lws_create_context(&info);
  if (context_ == nullptr) {
    running_ = false;
    throw std::runtime_error("libwebsockets: failed to create context on port " + std::to_string(port_));
  }

  worker_ = std::thread(&WsServer::service_loop, this);
  RCLCPP_INFO(
    rclcpp::get_logger("webviz_ws"),
    "WebSocket listening on ws://0.0.0.0:%d", port_);
}

void WsServer::stop()
{
  if (!running_.exchange(false)) {
    return;
  }
  if (context_ != nullptr) {
    lws_cancel_service(context_);
  }
  if (worker_.joinable()) {
    worker_.join();
  }
  if (context_ != nullptr) {
    lws_context_destroy(context_);
    context_ = nullptr;
  }
  std::lock_guard<std::mutex> lock(clients_mutex_);
  clients_.clear();
}

void WsServer::service_loop()
{
  while (running_) {
    lws_service(context_, kServiceTimeoutMs);
  }
}

size_t WsServer::client_count() const
{
  std::lock_guard<std::mutex> lock(clients_mutex_);
  return clients_.size();
}

uint64_t WsServer::bump_sequence()
{
  return ++sequence_;
}

std::shared_ptr<const std::vector<uint8_t>> WsServer::latest_payload() const
{
  std::lock_guard<std::mutex> lock(payload_mutex_);
  return latest_;
}

void WsServer::register_client(struct lws * wsi)
{
  std::lock_guard<std::mutex> lock(clients_mutex_);
  clients_.insert(wsi);
  RCLCPP_INFO(rclcpp::get_logger("webviz_ws"), "client connected (total=%zu)", clients_.size());
}

void WsServer::unregister_client(struct lws * wsi)
{
  std::lock_guard<std::mutex> lock(clients_mutex_);
  clients_.erase(wsi);
  RCLCPP_INFO(rclcpp::get_logger("webviz_ws"), "client disconnected (total=%zu)", clients_.size());
}

void WsServer::schedule_writable_all()
{
  std::lock_guard<std::mutex> lock(clients_mutex_);
  for (auto * wsi : clients_) {
    lws_callback_on_writable(wsi);
  }
  if (context_ != nullptr) {
    lws_cancel_service(context_);
  }
}

void WsServer::broadcast(const std::string & payload)
{
  if (payload.empty()) {
    return;
  }
  if (client_count() == 0) {
    return;
  }

  auto bytes = std::make_shared<std::vector<uint8_t>>(payload.begin(), payload.end());
  {
    std::lock_guard<std::mutex> lock(payload_mutex_);
    latest_ = bytes;
  }
  bump_sequence();
  schedule_writable_all();
}

int WsServer::lws_callback(
  struct lws * wsi, enum lws_callback_reasons reason, void * user, void * /*in*/, size_t /*len*/)
{
  auto * self = static_cast<WsServer *>(lws_context_user(lws_get_context(wsi)));
  if (self == nullptr) {
    return 0;
  }

  auto * session = static_cast<PerSessionData *>(user);

  switch (reason) {
    case LWS_CALLBACK_ESTABLISHED:
      self->register_client(wsi);
      lws_callback_on_writable(wsi);
      break;

    case LWS_CALLBACK_CLOSED:
      self->unregister_client(wsi);
      break;

    case LWS_CALLBACK_SERVER_WRITEABLE: {
        if (session == nullptr) {
          break;
        }
        const uint64_t seq = self->sequence_.load();
        if (session->last_sent_sequence >= seq) {
          break;
        }
        const auto payload = self->latest_payload();
        if (payload == nullptr || payload->empty()) {
          break;
        }

        std::vector<unsigned char> buf(LWS_PRE + payload->size());
        std::memcpy(buf.data() + LWS_PRE, payload->data(), payload->size());
        const int written = lws_write(
          wsi, buf.data() + LWS_PRE, payload->size(), LWS_WRITE_BINARY);
        if (written < 0) {
          RCLCPP_WARN(rclcpp::get_logger("webviz_ws"), "lws_write failed: %d", written);
          return -1;
        }
        session->last_sent_sequence = seq;
        if (session->last_sent_sequence < self->sequence_.load()) {
          lws_callback_on_writable(wsi);
        }
        break;
      }

    default:
      break;
  }
  return 0;
}

}  // namespace topsbot_webviz
