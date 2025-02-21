#include "catalog/catalog.h"
#include "record/schema.h"
void CatalogMeta::SerializeTo(char *buf) const {
  ASSERT(GetSerializedSize() <= PAGE_SIZE, "Failed to serialize catalog metadata to disk.");
  MACH_WRITE_UINT32(buf, CATALOG_METADATA_MAGIC_NUM);
  buf += 4;
  MACH_WRITE_UINT32(buf, table_meta_pages_.size());
  buf += 4;
  MACH_WRITE_UINT32(buf, index_meta_pages_.size());
  buf += 4;
  for (auto iter : table_meta_pages_) {
    MACH_WRITE_TO(table_id_t, buf, iter.first);
    buf += 4;
    MACH_WRITE_TO(page_id_t, buf, iter.second);
    buf += 4;
  }
  for (auto iter : index_meta_pages_) {
    MACH_WRITE_TO(index_id_t, buf, iter.first);
    buf += 4;
    MACH_WRITE_TO(page_id_t, buf, iter.second);
    buf += 4;
  }
}

CatalogMeta *CatalogMeta::DeserializeFrom(char *buf) {
  // check valid
  uint32_t magic_num = MACH_READ_UINT32(buf);
  buf += 4;
  ASSERT(magic_num == CATALOG_METADATA_MAGIC_NUM, "Failed to deserialize catalog metadata from disk.");
  // get table and index nums
  uint32_t table_nums = MACH_READ_UINT32(buf);
  buf += 4;
  uint32_t index_nums = MACH_READ_UINT32(buf);
  buf += 4;
  // create metadata and read value
  CatalogMeta *meta = new CatalogMeta();
  for (uint32_t i = 0; i < table_nums; i++) {
    auto table_id = MACH_READ_FROM(table_id_t, buf);
    buf += 4;
    auto table_heap_page_id = MACH_READ_FROM(page_id_t, buf);
    buf += 4;
    meta->table_meta_pages_.emplace(table_id, table_heap_page_id);
  }
  for (uint32_t i = 0; i < index_nums; i++) {
    auto index_id = MACH_READ_FROM(index_id_t, buf);
    buf += 4;
    auto index_page_id = MACH_READ_FROM(page_id_t, buf);
    buf += 4;
    meta->index_meta_pages_.emplace(index_id, index_page_id);
  }
  return meta;
}
/**
 * TODO: Student Implement
 */
uint32_t CatalogMeta::GetSerializedSize() const {
  uint32_t size = 4; // Magic number size
  size += 4; // Size of table meta pages count
  size += 4; // Size of index meta pages count
  size += (table_meta_pages_.size() + index_meta_pages_.size()) * (4 + 4); // Each ID and page ID pair
  return size;
}


CatalogMeta::CatalogMeta() {}

/**
 * TODO: Student Implement
 */
CatalogManager::CatalogManager(BufferPoolManager *buffer_pool_manager, LockManager *lock_manager,
                               LogManager *log_manager, bool init)
    : buffer_pool_manager_(buffer_pool_manager), lock_manager_(lock_manager), log_manager_(log_manager) {

  if (init) {
    // 如果是初始化新数据库，创建新的 CatalogMeta 实例
    catalog_meta_ = CatalogMeta::NewInstance();
    if (catalog_meta_ == nullptr) {
      throw std::runtime_error("Failed to create new CatalogMeta instance.");
    }

    // 分配一个页面来持久化新的目录元数据
    page_id_t meta_page_id = INVALID_PAGE_ID;
    Page *meta_page = buffer_pool_manager_->NewPage(meta_page_id);
    if (meta_page == nullptr || meta_page_id == INVALID_PAGE_ID) {
      throw std::runtime_error("Failed to allocate a new page for catalog metadata.");
    }

    // 将 CatalogMeta 实例的数据序列化到分配的页面
    catalog_meta_->SerializeTo(reinterpret_cast<char*>(meta_page->GetData()));

    // 将页面标记为已修改，并解除固定
    buffer_pool_manager_->UnpinPage(meta_page_id, true);
  } else {
    // 如果是加载已存在的数据库，从磁盘中读取目录元数据
    page_id_t meta_page_id = CATALOG_META_PAGE_ID;  // 假设 CATALOG_META_PAGE_ID 已经定义
    Page *meta_page = buffer_pool_manager_->FetchPage(meta_page_id);
    if (meta_page == nullptr) {
      throw std::runtime_error("Failed to fetch catalog metadata page.");
    }

    // 反序列化页面数据以获取 CatalogMeta 实例
    catalog_meta_ = CatalogMeta::DeserializeFrom(reinterpret_cast<char*>(meta_page->GetData()));
    if (catalog_meta_ == nullptr) {
      buffer_pool_manager_->UnpinPage(meta_page_id, false);
      throw std::runtime_error("Failed to deserialize catalog metadata.");
    }

    // 解除页面固定
    buffer_pool_manager_->UnpinPage(meta_page_id, false);
  }

  // 这里可能还需要其他初始化逻辑，例如设置表和索引的映射

  // 刷盘以确保目录的变动持久化
  FlushCatalogMetaPage();
}


CatalogManager::~CatalogManager() {
  FlushCatalogMetaPage();
  delete catalog_meta_;
  for (auto iter : tables_) {
    delete iter.second;
  }
  for (auto iter : indexes_) {
    delete iter.second;
  }
}

/**
 * TODO: Student Implement
 */
/*dberr_t CatalogManager::CreateTable(const string &table_name, TableSchema *schema, Txn *txn, TableInfo *&table_info) {
  if (table_names_.find(table_name) != table_names_.end()) {
    return DB_FAILED;
  }

  table_id_t new_table_id = next_table_id_++;
  page_id_t root_page_id = INVALID_PAGE_ID; // 应该从磁盘管理器请求一个新页


  table_info = TableInfo::Create();
  table_info->Init(table_meta, table_heap);

  tables_[new_table_id] = table_info;
  table_names_[table_name] = new_table_id;

  return DB_SUCCESS;
}*/
dberr_t CatalogManager::CreateTable(const string &table_name, TableSchema *schema,
                                    Txn *txn, TableInfo *&table_info) {
  if (table_names_.find(table_name) != table_names_.end()) {
    return DB_TABLE_ALREADY_EXIST;  // Changed from DB_FAILED to a more specific error
  }

  table_id_t new_table_id = next_table_id_++; // Ensure atomic increment
  page_id_t root_page_id = INVALID_PAGE_ID;

  TableMetadata *table_meta = TableMetadata::Create(new_table_id, table_name, root_page_id, schema);
  if (!table_meta) {
    buffer_pool_manager_->DeletePage(root_page_id); // Clean up if metadata creation fails
    return DB_FAILED;
  }

  TableHeap *table_heap = TableHeap::Create(buffer_pool_manager_, schema, txn, log_manager_, lock_manager_);
  if (!table_heap) {
    delete table_meta;  // Clean up if table heap creation fails
    buffer_pool_manager_->DeletePage(root_page_id);
    return DB_FAILED;
  }

  table_info = TableInfo::Create();
  table_info->Init(table_meta, table_heap);

  tables_[new_table_id] = table_info;
  table_names_[table_name] = new_table_id;

  return DB_SUCCESS;
}


dberr_t CatalogManager::GetTable(const string &table_name, TableInfo *&table_info) {
  auto table_id_it = table_names_.find(table_name);
  if (table_id_it == table_names_.end()) return DB_TABLE_NOT_EXIST;
  // 返回private方法GetTable的数据库状态，并把查到的内容填到table_info里
  return GetTable(table_id_it->second, table_info);
}

/**
 * TODO: Student Implement
 */
dberr_t CatalogManager::GetTables(vector<TableInfo *> &tables) const {
  tables.clear();  // Clear the provided vector to ensure it only contains current tables.
  for (const auto &table_pair : tables_) {
    tables.push_back(table_pair.second);  // Add pointer to each TableInfo in the catalog.
  }
  return DB_SUCCESS;  // Return success if tables are successfully retrieved.
}


/**
 * TODO: Student Implement
 */
//有bug

dberr_t CatalogManager::CreateIndex(const std::string &table_name, const std::string &index_name,
                                    const std::vector<std::string> &index_keys, Txn *txn, IndexInfo *&index_info,
                                    const std::string &index_type) {
  // Check if the table exists
  auto table_iter = table_names_.find(table_name);
  if (table_iter == table_names_.end()) {
    return DB_TABLE_NOT_EXIST;  // Table does not exist
  }

  // Retrieve the table info
  TableInfo *table_info = tables_[table_iter->second];

  // Check if the index already exists
  auto &index_map = index_names_[table_name];
  if (index_map.find(index_name) != index_map.end()) {
    return DB_INDEX_ALREADY_EXIST;  // Index already exists
  }

  // Create key map based on the schema
  std::vector<uint32_t> key_map;
  TableSchema *schema = table_info->GetSchema();
  for (const auto &key : index_keys) {
    uint32_t column_index;
    if (schema->GetColumnIndex(key, column_index) != DB_SUCCESS) {
      return DB_COLUMN_NAME_NOT_EXIST;
    }
    key_map.push_back(column_index);
  }

  // Create and initialize the index metadata
  index_id_t index_id = next_index_id_++;
  IndexMetadata *index_meta = IndexMetadata::Create(index_id, index_name, table_info->GetTableId(), key_map);

  index_info = IndexInfo::Create();
  index_info->Init(index_meta, table_info, buffer_pool_manager_);

  // Register the new index in the maps
  index_map[index_name] = index_id;
  indexes_[index_id] = index_info;

  return DB_SUCCESS;
}



/**
 * TODO: Student Implement
 */
dberr_t CatalogManager::GetIndex(const std::string &table_name, const std::string &index_name,
                                 IndexInfo *&index_info) const {
  // Check if the table exists
  auto table_iter = table_names_.find(table_name);
  if (table_iter == table_names_.end()) {
    return DB_FAILED;  // The table does not exist
  }

  // Check if the index exists for the given table
  auto index_map_iter = index_names_.find(table_name);
  if (index_map_iter == index_names_.end()) {
    return DB_INDEX_NOT_FOUND;  // No indexes for this table
  }

  auto index_iter = index_map_iter->second.find(index_name);
  if (index_iter == index_map_iter->second.end()) {
    return DB_INDEX_NOT_FOUND;  // The specified index does not exist
  }

  // Retrieve the IndexInfo
  index_id_t index_id = index_iter->second;
  //index_info = indexes_[index_id];
  return DB_SUCCESS;
}


/**
 * TODO: Student Implement
 */
dberr_t CatalogManager::GetTableIndexes(const std::string &table_name, std::vector<IndexInfo *> &indexes) const {
  indexes.clear();  // Clear the output vector to ensure it only contains current indexes

  // Check if the table exists
  auto table_iter = table_names_.find(table_name);
  if (table_iter == table_names_.end()) {
    return DB_FAILED;  // The table does not exist
  }

  // Check if there are indexes for this table
  auto index_map_iter = index_names_.find(table_name);
  if (index_map_iter == index_names_.end()) {
    return DB_SUCCESS;  // No indexes to return, but this is not an error
  }

  // Collect all IndexInfo objects for the indexes of the table
  for (const auto &index_pair : index_map_iter->second) {
    index_id_t index_id = index_pair.second;
    indexes.push_back(indexes[index_id]);
  }

  return DB_SUCCESS;
}


/**
 * TODO: Student Implement
 */
dberr_t CatalogManager::DropTable(const string &table_name) {
  auto it = table_names_.find(table_name);
  if (it == table_names_.end()) {
    return DB_TABLE_NOT_EXIST;  // No table with the given name exists
  }

  table_id_t table_id = it->second;
  TableInfo *table_info = tables_[table_id];
  // Optionally, delete associated data pages (not shown here)

  // Remove table from maps and clean up resources
  delete table_info;
  tables_.erase(table_id);
  table_names_.erase(table_name);

  // Flush changes to disk if necessary
  FlushCatalogMetaPage();

  return DB_SUCCESS;
}


/**
 * TODO: Student Implement
 */
dberr_t CatalogManager::DropIndex(const string &table_name, const string &index_name) {
  auto table_it = table_names_.find(table_name);
  if (table_it == table_names_.end()) {
    return DB_TABLE_NOT_EXIST;  // The table does not exist
  }

  auto &index_map = index_names_[table_name];
  auto index_it = index_map.find(index_name);
  if (index_it == index_map.end()) {
    return DB_INDEX_NOT_FOUND;  // The index does not exist
  }

  index_id_t index_id = index_it->second;
  IndexInfo *index_info = indexes_[index_id];
  // Optionally, delete associated index data pages (not shown here)

  // Remove index from maps and clean up resources
  delete index_info;
  indexes_.erase(index_id);
  index_map.erase(index_name);

  // Flush changes to disk if necessary
  FlushCatalogMetaPage();

  return DB_SUCCESS;
}


/**
 * TODO: Student Implement
 */
dberr_t CatalogManager::FlushCatalogMetaPage() const {
  page_id_t catalog_meta_page_id = CATALOG_META_PAGE_ID;  // Assuming a defined constant for the catalog page ID
  Page *page = buffer_pool_manager_->NewPage(catalog_meta_page_id);  // This might be FetchPage depending on design
  if (!page) {
    return DB_FAILED;  // Failed to allocate or fetch the page
  }


  // Serialize the catalog metadata into the page's data area
  catalog_meta_->SerializeTo(page->GetData());
  buffer_pool_manager_->UnpinPage(catalog_meta_page_id, true);  // Mark the page as dirty

  return DB_SUCCESS;
}


/**
 * TODO: Student Implement
 */
dberr_t CatalogManager::LoadTable(const table_id_t table_id, const page_id_t page_id) {
  // Fetch the page from the buffer pool manager
  Page *page = buffer_pool_manager_->FetchPage(page_id);
  if (page == nullptr) {
    return DB_INDEX_NOT_FOUND; // Failed to fetch the page
  }

  // Deserialize table metadata from the page
  TableMetadata *table_meta;
  if (TableMetadata::DeserializeFrom(page->GetData(), table_meta) != true) {
    buffer_pool_manager_->UnpinPage(page_id, false);
    return DB_FAILED; // Deserialization failed
  }

  // Check if the TableInfo already exists, and update it; otherwise, create a new one
  TableInfo *table_info =  tables_[table_id] ;
  table_info->Init(table_meta, TableHeap::Create(buffer_pool_manager_, table_meta->GetFirstPageId(), nullptr,log_manager_, lock_manager_));

  // Store the new or updated TableInfo in the catalog
  tables_[table_id] = table_info;
  table_names_[table_meta->GetTableName()] = table_id; // Update the name-ID mapping

  buffer_pool_manager_->UnpinPage(page_id, false); // Unpin the page after use
  return DB_SUCCESS;
}


/**
 * TODO: Student Implement
 */
dberr_t CatalogManager::LoadIndex(const index_id_t index_id, const page_id_t page_id) {
  Page *page = buffer_pool_manager_->FetchPage(page_id);
  if (!page) {
    return DB_FAILED; // Failed to fetch the page
  }

  // Deserialize index metadata from the page
  IndexMetadata *index_meta;
  if (IndexMetadata::DeserializeFrom(page->GetData(), index_meta) != true) {
    buffer_pool_manager_->UnpinPage(page_id, false);
    return DB_FAILED; // Deserialization failed
  }

  // Retrieve or create the IndexInfo object
  IndexInfo *index_info = indexes_[index_id] ;
  index_info->Init(index_meta, tables_[index_meta->GetTableId()], buffer_pool_manager_);

  // Update the catalog
  indexes_[index_id] = index_info;
  index_names_[tables_[index_meta->GetTableId()]->GetTableName()][index_meta->GetIndexName()] = index_id;

  buffer_pool_manager_->UnpinPage(page_id, false); // Unpin the page after use
  return DB_SUCCESS;
}


/**
 * TODO: Student Implement
 */
/*dberr_t CatalogManager::GetTable(const table_id_t table_id, TableInfo *&table_info) {
  auto it = tables_.find(table_id);
  if (it == tables_.end()) {
    return DB_FAILED; // No table with the given ID exists
  }
  table_info = it->second;
  return DB_SUCCESS;
}*/
dberr_t CatalogManager::GetTable(const table_id_t table_id, TableInfo *&table_info) {
  std::unordered_map<table_id_t, TableInfo *>::const_iterator iter;
  iter=tables_.find(table_id);
  if(iter!=tables_.end())
  {
    table_info=iter->second;
    return DB_SUCCESS;
  }
  else
    return DB_TABLE_NOT_EXIST;
}
