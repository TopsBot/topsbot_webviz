// Copyright 2026 TOPSBOT contributors.
// SPDX-License-Identifier: Apache-2.0

#include "utils/threads/threadpool.h"

namespace topsbot_webviz
{

ThreadPool::ThreadPool() = default;

ThreadPool::~ThreadPool()
{
  {
    std::lock_guard<std::mutex> lock(mutex_);
    stop_ = true;
  }
  cv_.notify_all();
  for (auto & worker : workers_) {
    if (worker.joinable()) {
      worker.join();
    }
  }
}

void ThreadPool::start(const size_t thread_count)
{
  if (!workers_.empty()) {
    return;
  }
  workers_.reserve(thread_count);
  for (size_t i = 0; i < thread_count; ++i) {
    workers_.emplace_back(&ThreadPool::worker_loop, this);
  }
}

void ThreadPool::post(std::function<void()> task)
{
  {
    std::lock_guard<std::mutex> lock(mutex_);
    tasks_.push_back(std::move(task));
  }
  cv_.notify_one();
}

void ThreadPool::clear()
{
  std::lock_guard<std::mutex> lock(mutex_);
  tasks_.clear();
}

size_t ThreadPool::pending() const
{
  std::lock_guard<std::mutex> lock(mutex_);
  return tasks_.size();
}

void ThreadPool::worker_loop()
{
  while (true) {
    std::function<void()> task;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait(lock, [this]() { return stop_ || !tasks_.empty(); });
      if (stop_ && tasks_.empty()) {
        return;
      }
      task = std::move(tasks_.front());
      tasks_.pop_front();
    }
    if (task) {
      task();
    }
  }
}

}  // namespace topsbot_webviz
