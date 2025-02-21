#ifndef MINISQL_TABLE_ITERATOR_H
#define MINISQL_TABLE_ITERATOR_H

#include "common/rowid.h"
#include "concurrency/txn.h"
#include "record/row.h"

class TableHeap;

class TableIterator {
 public:
  // Constructor
  explicit TableIterator(TableHeap *table_heap, RowId rid, Txn *txn);

  // Copy Constructor
  explicit TableIterator(const TableIterator &other);

  // Destructor
  virtual ~TableIterator();

  // Equality Operator
  bool operator==(const TableIterator &itr) const;

  // Inequality Operator
  bool operator!=(const TableIterator &itr) const;

  // Dereference Operator
  const Row &operator*();

  // Arrow Operator
  Row *operator->();

  // Assignment Operator
  TableIterator &operator=(const TableIterator &itr) noexcept;

  // Pre-increment Operator
  TableIterator &operator++();

  // Post-increment Operator
  TableIterator operator++(int);

 private:
  TableHeap *table_heap_;
  RowId rid_;
  Txn *txn_;
  Row *row_;
};

#endif  // MINISQL_TABLE_ITERATOR_H
