#ifndef MINISQL_TABLE_HEAP_H
#define MINISQL_TABLE_HEAP_H

#include "buffer/buffer_pool_manager.h"
#include "concurrency/lock_manager.h"
#include "page/header_page.h"
#include "page/table_page.h"
#include "recovery/log_manager.h"
#include "storage/table_iterator.h"
#include <cstdint>
#include <cstdlib>
#include <unordered_set>
#include "common/macros.h"


class TableHeap {
  friend class TableIterator;

 public:
  static TableHeap *Create(BufferPoolManager *buffer_pool_manager, Schema *schema, Txn *txn, LogManager *log_manager,
                           LockManager *lock_manager) {
    return new TableHeap(buffer_pool_manager, schema, txn, log_manager, lock_manager);
  }

  static TableHeap *Create(BufferPoolManager *buffer_pool_manager, page_id_t first_page_id, Schema *schema,
                           LogManager *log_manager, LockManager *lock_manager) {
    return new TableHeap(buffer_pool_manager, first_page_id, schema, log_manager, lock_manager);
  }

  ~TableHeap() {}
  bool InsertTuple(Row &row, Txn *txn);
  bool MarkDelete(const RowId &rid, Txn *txn);
  bool UpdateTuple(Row &row, const RowId &rid, Txn *txn);
  void ApplyDelete(const RowId &rid, Txn *txn);
  void RollbackDelete(const RowId &rid, Txn *txn);
  bool GetTuple(Row *row, Txn *txn);
  void FreeTableHeap();
  void DeleteTable(page_id_t page_id = INVALID_PAGE_ID);
  TableIterator Begin(Txn *txn);
  TableIterator End();
  inline page_id_t GetFirstPageId() const { return first_page_id_; }

 private:
  TableHeap(BufferPoolManager *buffer_pool_manager, Schema *schema, Txn *txn, LogManager *log_manager,
            LockManager *lock_manager);

  TableHeap(BufferPoolManager *buffer_pool_manager, page_id_t first_page_id, Schema *schema, LogManager *log_manager,
            LockManager *lock_manager);

 private:
  BufferPoolManager *buffer_pool_manager_;
  page_id_t first_page_id_;
  Schema *schema_;
  [[maybe_unused]] LogManager *log_manager_;
  [[maybe_unused]] LockManager *lock_manager_;
};


class MemHeap {
 public:
  virtual ~MemHeap() = default;

  /**
   * @brief Allocate a contiguous block of memory of the given size
   * @param size The size (in bytes) of memory to allocate
   * @return A non-null pointer if allocation is successful. A null pointer if
   * allocation fails.
   */
  virtual void *Allocate(size_t size) = 0;

  /**
   * @brief Returns the provided chunk of memory back into the pool
   */
  virtual void Free(void *ptr) = 0;

};

class SimpleMemHeap : public MemHeap {
 public:
  ~SimpleMemHeap() {
    for (auto it: allocated_) {
      free(it);
    }
  }

  void *Allocate(size_t size) {
    void *buf = malloc(size);
    ASSERT(buf != nullptr, "Out of memory exception");
    allocated_.insert(buf);
    return buf;
  }

  void Free(void *ptr) {
    if (ptr == nullptr) {
      return;
    }
    auto iter = allocated_.find(ptr);
    if (iter != allocated_.end()) {
      allocated_.erase(iter);
    }
  }

 private:
  std::unordered_set<void *> allocated_;
};


#endif  // MINISQL_TABLE_HEAP_H
