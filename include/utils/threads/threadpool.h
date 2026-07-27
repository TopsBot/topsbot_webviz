// Copyright 2026 TOPSBOT contributors.
// SPDX-License-Identifier: Apache-2.0

#ifndef UTILS_THREADS_THREADPOOL_H_
#define UTILS_THREADS_THREADPOOL_H_

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace topsbot_webviz
{

class ThreadPool
{
public:
  ThreadPool();
  ~ThreadPool();

  void start(size_t thread_count);
  void post(std::function<void()> task);
  void clear();
  size_t pending() const;

private:
  void worker_loop();

  mutable std::mutex mutex_;
  std::condition_variable cv_;
  std::deque<std::function<void()>> tasks_;
  std::vector<std::thread> workers_;
  bool stop_{false};
};

}  // namespace topsbot_webviz

#endif  // UTILS_THREADS_THREADPOOL_H_
