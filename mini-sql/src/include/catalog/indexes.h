#ifndef MINISQL_INDEXES_H
#define MINISQL_INDEXES_H

#include <memory>

#include "catalog/table.h"
#include "common/macros.h"
#include "common/rowid.h"
#include "index/b_plus_tree_index.h"
#include "index/generic_key.h"
#include "record/schema.h"

class IndexMetadata {
  friend class IndexInfo;

 public:
  static IndexMetadata *Create(const index_id_t index_id, const std::string &index_name, const table_id_t table_id,
                               const std::vector<uint32_t> &key_map);

  uint32_t SerializeTo(char *buf) const;

  uint32_t GetSerializedSize() const;

  static uint32_t DeserializeFrom(char *buf, IndexMetadata *&index_meta);

  inline std::string GetIndexName() const { return index_name_; }

  inline table_id_t GetTableId() const { return table_id_; }

  uint32_t GetIndexColumnCount() const { return key_map_.size(); }

  inline const std::vector<uint32_t> &GetKeyMapping() const { return key_map_; }

  inline index_id_t GetIndexId() const { return index_id_; }

 private:
  IndexMetadata() = delete;

  explicit IndexMetadata(const index_id_t index_id, const std::string &index_name, const table_id_t table_id,
                         const std::vector<uint32_t> &key_map);

 private:
  static constexpr uint32_t INDEX_METADATA_MAGIC_NUM = 344528;
  index_id_t index_id_;
  std::string index_name_;
  table_id_t table_id_;
  std::vector<uint32_t> key_map_; /** The mapping of index key to tuple key */
};

/**
 * The IndexInfo class maintains metadata about a index.
 */
class IndexInfo {
 public:
  static IndexInfo *Create() { return new IndexInfo(); }

  ~IndexInfo() {
    delete meta_data_;
    delete index_;
    delete key_schema_;
  }

/**
 * TODO: Student Implement
 */
  void Init(IndexMetadata *meta_data, TableInfo *table_info, BufferPoolManager *buffer_pool_manager) {
    // Step 1: Initialize index metadata and table info
    this->meta_data_ = meta_data;

    // Step 2: Mapping index key to key schema
    std::vector<Column> columns;
    const std::vector<uint32_t>& key_mapping = meta_data_->GetKeyMapping();
    Schema* table_schema = table_info->GetSchema();
    for (uint32_t column_index : key_mapping) {
      // Ensure that the column index is valid for the table schema
      ASSERT(column_index < table_schema->GetColumnCount(), "Column index out of bounds");
      columns.push_back(table_schema->GetColumn(column_index));
    }
    Schema *schema= table_info->GetSchema();
    const std::vector<uint32_t> keymap=meta_data_->GetKeyMapping();
    // Create a new schema for the index keys
    // Assume Schema constructor needs `Column*` (if you use pointers) or simply pass `columns` if copying is acceptable.
    this->key_schema_ = Schema::ShallowCopySchema(schema,keymap);  // Modified as necessary

    // Step 3: Call CreateIndex to create the index
    // Determine index type and create the appropriate index structure
    std::string index_type = "bptree";  // Example type, should be determined based on the system or metadata
    this->index_ = CreateIndex(buffer_pool_manager, index_type);    

    ASSERT(this->index_ != nullptr, "Failed to create index");
  }



  inline Index *GetIndex() { return index_; }

  std::string GetIndexName() { return meta_data_->GetIndexName(); }

  IndexSchema *GetIndexKeySchema() { return key_schema_; }

 private:
  explicit IndexInfo() : meta_data_{nullptr}, index_{nullptr}, key_schema_{nullptr} {}

  Index *CreateIndex(BufferPoolManager *buffer_pool_manager, const string &index_type);

 private:
  IndexMetadata *meta_data_;
  Index *index_;
  IndexSchema *key_schema_;
};

#endif  // MINISQL_INDEXES_H
