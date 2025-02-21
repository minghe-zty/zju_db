#include "buffer/lru_replacer.h"

LRUReplacer::LRUReplacer(size_t num_pages) {}

LRUReplacer::~LRUReplacer() = default;

bool LRUReplacer::Victim(frame_id_t *frame_id) {
  std::scoped_lock<std::mutex> lock(latch_);
  if (lru_list_.empty()) {
    return false;
  }
  *frame_id = lru_list_.back();
  lru_set_.erase(lru_list_.back());
  lru_list_.pop_back();
  return true;
}

void LRUReplacer::Pin(frame_id_t frame_id) {
  std::scoped_lock<std::mutex> lock(latch_);
  auto it = lru_set_.find(frame_id);
  if (it != lru_set_.end()) {
    lru_list_.remove(frame_id);
    lru_set_.erase(it);
  }
}

void LRUReplacer::Unpin(frame_id_t frame_id) {
  std::scoped_lock<std::mutex> lock(latch_);
  if (lru_set_.find(frame_id) == lru_set_.end()) {
    lru_list_.push_front(frame_id);
    lru_set_.insert(frame_id);
  }
}

size_t LRUReplacer::Size() {
  std::scoped_lock<std::mutex> lock(latch_);
  return lru_list_.size();
}
