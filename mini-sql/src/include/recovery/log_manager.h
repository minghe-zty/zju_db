#ifndef MINISQL_LOG_MANAGER_H
#define MINISQL_LOG_MANAGER_H


#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <atomic>
#include <iostream>

/**
 * LogManager maintains a separate thread that is awakened whenever the
 * log buffer is full or whenever a timeout happens.
 * When the thread is awakened, the log buffer's content is written into the disk log file.
 */
class LogManager {
 public:
  LogManager() : shutdown_(false) {
    writer_thread_ = std::thread(&LogManager::ProcessLogQueue, this);
  }

  ~LogManager() {
    shutdown_ = true;
    cv_.notify_one();
    writer_thread_.join();
  }

  void AppendLog(const std::string& log) {
    std::lock_guard<std::mutex> lock(mutex_);
    log_queue_.push_back(log);
    cv_.notify_one();
  }

 private:
  std::vector<std::string> log_queue_;
  std::mutex mutex_;
  std::condition_variable cv_;
  std::thread writer_thread_;
  std::atomic<bool> shutdown_;

  void ProcessLogQueue() {
    while (!shutdown_) {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait(lock, [this] { return !log_queue_.empty() || shutdown_; });

      if (!log_queue_.empty()) {
        // Simulate writing to disk
        for (const auto& log : log_queue_) {
          std::cout << "Writing log to disk: " << log << std::endl;
          // Actual disk writing logic would go here
        }
        log_queue_.clear();
      }
    }
  }
};

#endif  // MINISQL_LOG_MANAGER_H
