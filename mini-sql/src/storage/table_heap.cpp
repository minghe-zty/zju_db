#include "storage/table_heap.h"
#include "common/macros.h"
#include "page/table_page.h"


TableHeap::TableHeap(BufferPoolManager *buffer_pool_manager, Schema *schema, Txn *txn, LogManager *log_manager,
                     LockManager *lock_manager)
    : buffer_pool_manager_(buffer_pool_manager), schema_(schema), log_manager_(log_manager), lock_manager_(lock_manager) {
  Page *page = buffer_pool_manager_->NewPage(first_page_id_);
  if (page == nullptr) {
    throw std::runtime_error("Failed to allocate new page");
  }
  auto table_page = reinterpret_cast<TablePage *>(page);
  table_page->Init(first_page_id_, INVALID_PAGE_ID, log_manager, txn);
  buffer_pool_manager_->UnpinPage(first_page_id_, true);
}

TableHeap::TableHeap(BufferPoolManager *buffer_pool_manager, page_id_t first_page_id, Schema *schema, LogManager *log_manager,
                     LockManager *lock_manager)
    : buffer_pool_manager_(buffer_pool_manager), first_page_id_(first_page_id), schema_(schema), log_manager_(log_manager),
      lock_manager_(lock_manager) {}

bool TableHeap::InsertTuple(Row &row, Txn *txn) {
  if (row.GetSerializedSize(schema_) > PAGE_SIZE - 32) {
    return false;
  }
  auto page = reinterpret_cast<TablePage *>(buffer_pool_manager_->FetchPage(GetFirstPageId()));
  while (true) {
    if (page == nullptr) {
      return false;
    }
    if (page->InsertTuple(row, schema_, txn, lock_manager_, log_manager_)) {
      buffer_pool_manager_->UnpinPage(page->GetTablePageId(), true);
      return true;
    }
    buffer_pool_manager_->UnpinPage(page->GetTablePageId(), false);
    page_id_t next = page->GetNextPageId();
    if (next != INVALID_PAGE_ID) {
      page = reinterpret_cast<TablePage *>(buffer_pool_manager_->FetchPage(next));
    } else {
      auto new_page = reinterpret_cast<TablePage *>(buffer_pool_manager_->NewPage(next));
      page->SetNextPageId(next);
      new_page->Init(next, page->GetPageId(), log_manager_, txn);
      new_page->InsertTuple(row, schema_, txn, lock_manager_, log_manager_);
      buffer_pool_manager_->UnpinPage(next, true);
      return true;
    }
  }
}

bool TableHeap::MarkDelete(const RowId &rid, Txn *txn) {
  auto page = reinterpret_cast<TablePage *>(buffer_pool_manager_->FetchPage(rid.GetPageId()));
  if (page == nullptr) {
    return false;
  }
  page->WLatch();
  page->MarkDelete(rid, txn, lock_manager_, log_manager_);
  page->WUnlatch();
  buffer_pool_manager_->UnpinPage(page->GetTablePageId(), true);
  return true;
}

bool TableHeap::UpdateTuple(Row &row, const RowId &rid, Txn *txn) {
  auto page = reinterpret_cast<TablePage *>(buffer_pool_manager_->FetchPage(rid.GetPageId()));
  if (page == nullptr) return false;
  Row old(rid);

  // 新修改0612
  // if (!GetTuple(&old, txn)) {
  //   return false;
  // }

  page->WLatch();    
  int type = page->UpdateTuple(row, &old, schema_, txn, lock_manager_, log_manager_);  
  page->WUnlatch();
  if (type == 0) {
    buffer_pool_manager_->UnpinPage(page->GetTablePageId(), true);    
    return true;
  } else {
    buffer_pool_manager_->UnpinPage(page->GetTablePageId(), false);    
    return false;
  }
}

void TableHeap::ApplyDelete(const RowId &rid, Txn *txn) {
  auto page = reinterpret_cast<TablePage *>(buffer_pool_manager_->FetchPage(rid.GetPageId()));
  assert(page != nullptr);
  page->WLatch();
  page->ApplyDelete(rid, txn, log_manager_);
  page->WUnlatch();
  buffer_pool_manager_->UnpinPage(page->GetTablePageId(), true);
}

void TableHeap::RollbackDelete(const RowId &rid, Txn *txn) {
  auto page = reinterpret_cast<TablePage *>(buffer_pool_manager_->FetchPage(rid.GetPageId()));
  assert(page != nullptr);
  page->WLatch();
  page->RollbackDelete(rid, txn, log_manager_);
  page->WUnlatch();
  buffer_pool_manager_->UnpinPage(page->GetTablePageId(), true);
}

bool TableHeap::GetTuple(Row *row, Txn *txn) {
  auto page = reinterpret_cast<TablePage *>(buffer_pool_manager_->FetchPage(row->GetRowId().GetPageId()));
  if (page == nullptr) return false;
  page->RLatch();  
  if (!page->GetTuple(row, schema_, txn, lock_manager_)) {    
    page->RUnlatch();
    buffer_pool_manager_->UnpinPage(page->GetTablePageId(), false);
    return false;
  }  
  page->RUnlatch();
  buffer_pool_manager_->UnpinPage(page->GetTablePageId(), false);  
  return true;
}

TableIterator TableHeap::Begin(Txn *txn) {
  auto page = reinterpret_cast<TablePage *>(buffer_pool_manager_->FetchPage(first_page_id_));
  RowId rid;
  page->RLatch();
  page->GetFirstTupleRid(&rid);
  page->RUnlatch();
  buffer_pool_manager_->UnpinPage(first_page_id_, false);
  return TableIterator(this, rid, txn);
}

TableIterator TableHeap::End() {
  return TableIterator(this, INVALID_ROWID, nullptr);
}
