// Copyright 2026 TOPSBOT contributors.
// SPDX-License-Identifier: Apache-2.0

#ifndef TOPSBOT_WEBVIZ__TRANSPORT_WS_SERVER_HPP_
#define TOPSBOT_WEBVIZ__TRANSPORT_WS_SERVER_HPP_

#include <atomic>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include <libwebsockets.h>

struct lws;
struct lws_context;

namespace topsbot_webviz
{

/** Thread-safe WebSocket server (libwebsockets) for binary protobuf broadcast. */
class WsServer
{
public:
  explicit WsServer(int port);
  ~WsServer();

  WsServer(const WsServer &) = delete;
  WsServer & operator=(const WsServer &) = delete;

  void start();
  void stop();

  /** Enqueue latest frame; safe from ROS / pipeline threads. */
  void broadcast(const std::string & payload);

  size_t client_count() const;

  static int lws_callback(
    struct lws * wsi, enum lws_callback_reasons reason, void * user, void * in, size_t len);

private:
  struct PerSessionData
  {
    uint64_t last_sent_sequence{0};
  };

  std::shared_ptr<const std::vector<uint8_t>> latest_payload() const;
  uint64_t bump_sequence();
  void register_client(struct lws * wsi);
  void unregister_client(struct lws * wsi);
  void schedule_writable_all();
  void service_loop();

  int port_;
  struct lws_context * context_{nullptr};
  std::thread worker_;
  std::atomic<bool> running_{false};

  mutable std::mutex payload_mutex_;
  std::shared_ptr<const std::vector<uint8_t>> latest_;
  std::atomic<uint64_t> sequence_{0};

  mutable std::mutex clients_mutex_;
  std::set<struct lws *> clients_;
};

}  // namespace topsbot_webviz

#endif  // TOPSBOT_WEBVIZ__TRANSPORT_WS_SERVER_HPP_
