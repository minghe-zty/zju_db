#include "storage/table_iterator.h"
#include "common/macros.h"
#include "storage/table_heap.h"

TableIterator::TableIterator(TableHeap *table_heap, RowId rid, Txn *txn)
    : table_heap_(table_heap), rid_(rid), txn_(txn), row_(new Row(rid)) {
  if (rid.Get() != INVALID_ROWID.Get()) {
    table_heap_->GetTuple(row_, txn_);
  }else {
    row_ = new Row(INVALID_ROWID);
  }
}


TableIterator::TableIterator(const TableIterator &other)
    : table_heap_(other.table_heap_), rid_(other.rid_), txn_(other.txn_), row_(new Row(*other.row_)) {}

TableIterator::~TableIterator() {
  delete row_;
}

bool TableIterator::operator==(const TableIterator &itr) const {
  return rid_ == itr.rid_;
}

bool TableIterator::operator!=(const TableIterator &itr) const {
  return !(*this == itr);
}

const Row &TableIterator::operator*() {
  return *row_;
}

Row *TableIterator::operator->() {
  return row_;
}

TableIterator &TableIterator::operator=(const TableIterator &itr) noexcept {
  if (this != &itr) {
    table_heap_ = itr.table_heap_;
    rid_ = itr.rid_;
    txn_ = itr.txn_;
    if (row_ != nullptr) {
      delete row_;
    }
    row_ = new Row(*itr.row_);
  }
  return *this;
}

// ++iter
TableIterator &TableIterator::operator++() {    

  RowId next_rid;
  auto page = reinterpret_cast<TablePage *>(table_heap_->buffer_pool_manager_->FetchPage(rid_.GetPageId()));  

  if (page->GetNextTupleRid(rid_, &next_rid)) {
    rid_ = next_rid;

    // 新修改0612
    if (row_ != nullptr) {
      delete row_;
    }    
    row_ = new Row(rid_);

    table_heap_->GetTuple(row_, txn_);
  } 
  else {

    // 新修改0612
    
    // 当前页面上没有更多的元组，移动到下一页
    page_id_t next_page_id = page->GetNextPageId();
    table_heap_->buffer_pool_manager_->UnpinPage(page->GetPageId(), false);

    if (next_page_id != INVALID_PAGE_ID) {
      page = reinterpret_cast<TablePage *>(table_heap_->buffer_pool_manager_->FetchPage(next_page_id));      
      page->GetFirstTupleRid(&next_rid);
      rid_ = next_rid;
      if (row_ != nullptr) {
        delete row_;
      }
      row_ = new Row(rid_);
      table_heap_->GetTuple(row_, txn_);
    } else {
      rid_ = INVALID_ROWID;
      delete row_;
      row_ = nullptr;
    }
  } 
  table_heap_->buffer_pool_manager_->UnpinPage(rid_.GetPageId(), false);
  return *this;
}

// iter++
TableIterator TableIterator::operator++(int) {
  TableIterator p(table_heap_, row_->GetRowId(),txn_);
  ++(*this);
  return TableIterator{p};
}