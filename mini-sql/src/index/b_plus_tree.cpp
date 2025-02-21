#include "index/b_plus_tree.h"

#include <string>

#include "glog/logging.h"
#include "index/basic_comparator.h"
#include "index/generic_key.h"
#include "page/index_roots_page.h"

/**
 * TODO: Student Implement
 */
BPlusTree::BPlusTree(index_id_t index_id, BufferPoolManager *buffer_pool_manager, const KeyManager &KM,
                     int leaf_max_size, int internal_max_size)
    : index_id_(index_id),
      buffer_pool_manager_(buffer_pool_manager),
      processor_(KM),
      leaf_max_size_(leaf_max_size),
      internal_max_size_(internal_max_size) {
  IndexRootsPage *rootpage = reinterpret_cast<IndexRootsPage *>(buffer_pool_manager_->FetchPage(INDEX_ROOTS_PAGE_ID));
  page_id_t root_page_id = INVALID_PAGE_ID;
  if(rootpage->GetRootId(index_id,&root_page_id))
  {
    root_page_id_ = root_page_id;    
  }
  else
    root_page_id_ = INVALID_PAGE_ID;
  
  buffer_pool_manager_->UnpinPage(INDEX_ROOTS_PAGE_ID,false);

  // 新修改
  if (leaf_max_size_ == 0)
    leaf_max_size_ = (PAGE_SIZE - LEAF_PAGE_HEADER_SIZE) / (processor_.GetKeySize() + sizeof(RowId));
  if (internal_max_size_ == 0)
    internal_max_size_ = (PAGE_SIZE - INTERNAL_PAGE_HEADER_SIZE) / (processor_.GetKeySize() + sizeof(page_id_t));
}


void BPlusTree::Destroy(page_id_t current_page_id) {

}

/*
 * Helper function to decide whether current b+tree is empty
 */
bool BPlusTree::IsEmpty() const {
  if(root_page_id_ == INVALID_PAGE_ID) return true;
  else return false;
}

/*****************************************************************************
 * SEARCH
 *****************************************************************************/
/*
 * Return the only value that associated with input key
 * This method is used for point query
 * @return : true means key exists
 */

bool BPlusTree::GetValue(const GenericKey *key, std::vector<RowId> &result, Txn *transaction) {
  // 新修改0612
  if (IsEmpty()) return false;

  Page* key_in_page=FindLeafPage(key,false);

  LeafPage* key_in_leafpage=reinterpret_cast<LeafPage*>(key_in_page->GetData());
  RowId res{};
  bool flag;
  flag=key_in_leafpage->Lookup(key,res,processor_);
  if(flag==true){
    result.push_back(res);
  }
  buffer_pool_manager_->UnpinPage(key_in_page->GetPageId(),false);
  return flag;
}

/*****************************************************************************
 * INSERTION
 *****************************************************************************/
/*
 * Insert constant key & value pair into b+ tree
 * if current tree is empty, start new tree, update root page id and insert
 * entry, otherwise insert into leaf page.
 * @return: since we only support unique key, if user try to insert duplicate
 * keys return false, otherwise return true.
 */
bool BPlusTree::Insert(GenericKey *key, const RowId &value, Txn *transaction) {
  if (IsEmpty() == true) {
    StartNewTree(key, value);
    return true;
  }
  return InsertIntoLeaf(key, value, transaction);
}
/*
 * Insert constant key & value pair into an empty tree
 * User needs to first ask for new page from buffer pool manager(NOTICE: throw
 * an "out of memory" exception if returned value is nullptr), then update b+
 * tree's root page id and insert entry directly into leaf page.
 */
void BPlusTree::StartNewTree(GenericKey *key, const RowId &value) {
  page_id_t new_page_id = INVALID_PAGE_ID;
  Page *root_page = buffer_pool_manager_->NewPage(new_page_id);
  if (nullptr == root_page) {
    throw std::runtime_error("out of memory");
  }
  root_page_id_ = new_page_id;
  auto* root_node = reinterpret_cast<LeafPage *>(root_page->GetData());
  // root_node->Init(root_page_id_, INVALID_PAGE_ID, leaf_max_size_);
  // 新修改
  root_node->Init(root_page_id_, INVALID_PAGE_ID, processor_.GetKeySize(), leaf_max_size_);

  root_node->Insert(key, value, processor_);
  UpdateRootPageId(1);
  buffer_pool_manager_->UnpinPage(root_page->GetPageId(), true);
}

/*
 * Insert constant key & value pair into leaf page
 * User needs to first find the right leaf page as insertion target, then look
 * through leaf page to see whether insert key exist or not. If exist, return
 * immediately, otherwise insert entry. Remember to deal with split if necessary.
 * @return: since we only support unique key, if user try to insert duplicate
 * keys return false, otherwise return true.
 */
bool BPlusTree::InsertIntoLeaf(GenericKey *key, const RowId &value, Txn *transaction) {

  auto *leaf = FindLeafPage(key, false);
  LeafPage* leafpage=reinterpret_cast<LeafPage*>(leaf->GetData());

  int before_insert_size = leafpage->GetSize();
  int after_insert_size=leafpage->Insert(key,value,this->processor_);

  if(before_insert_size==after_insert_size){
    buffer_pool_manager_->UnpinPage(leaf->GetPageId(), false);
    return false;
  }
  // if(after_insert_size > leafpage->GetMaxSize()) {
  // 新修改
  if(after_insert_size >= leafpage->GetMaxSize()) {
    LeafPage *split_page = Split(leafpage,transaction);
    InsertIntoParent(leafpage, split_page->KeyAt(0), split_page, transaction);
    buffer_pool_manager_->UnpinPage(leaf->GetPageId(), true);        // unpin leaf page
    buffer_pool_manager_->UnpinPage(split_page->GetPageId(), true);  // DEBUG: unpin new leaf node
    return true;
  }

  buffer_pool_manager_->UnpinPage(leaf->GetPageId(),true);

  return true;
}

/*
 * Split input page and return newly created page.
 * Using template N to represent either internal page or leaf page.
 * User needs to first ask for new page from buffer pool manager(NOTICE: throw
 * an "out of memory" exception if returned value is nullptr), then move half
 * of key & value pairs from input page to newly created page
 */
BPlusTreeInternalPage *BPlusTree::Split(InternalPage *node, Txn *transaction) {
  page_id_t new_page_id;
  Page *new_page = buffer_pool_manager_->NewPage(new_page_id);
  assert(new_page != nullptr); //这里没有抛出异常，而是改用assert
  InternalPage* new_node = reinterpret_cast<InternalPage *>(new_page->GetData());
  // new_node->Init(new_page_id, node->GetParentPageId());
  // 新修改
  new_node->Init(new_page_id, node->GetParentPageId(), processor_.GetKeySize(), leaf_max_size_);
  node->MoveHalfTo(new_node, buffer_pool_manager_);
  return new_node;
}

BPlusTreeLeafPage *BPlusTree::Split(LeafPage *node, Txn *transaction) {
  page_id_t new_page_id;
  Page *new_page = buffer_pool_manager_->NewPage(new_page_id);
  assert(new_page != nullptr); //这里没有抛出异常，而是改用assert
  LeafPage* new_node = reinterpret_cast<LeafPage *>(new_page->GetData());
  // new_node->Init(new_page_id, node->GetParentPageId());
  // 新修改
  new_node->Init(new_page_id, node->GetParentPageId(), processor_.GetKeySize(), leaf_max_size_);
  node->MoveHalfTo(new_node);
  return new_node;
}

/*
 * Insert key & value pair into internal page after split
 * @param   old_node      input page from split() method
 * @param   key
 * @param   new_node      returned page from split() method
 * User needs to first find the parent page of old_node, parent node must be
 * adjusted to take info of new_node into account. Remember to deal with split
 * recursively if necessary.
 */
void BPlusTree::InsertIntoParent(BPlusTreePage *old_node, GenericKey *key, BPlusTreePage *new_node, Txn *transaction) {
  if (old_node->IsRootPage()) {
    page_id_t new_page_id = INVALID_PAGE_ID;
    Page *new_page = buffer_pool_manager_->NewPage(new_page_id);

    root_page_id_ = new_page_id;

    InternalPage *new_root_node = reinterpret_cast<InternalPage *>(new_page->GetData());
    // new_root_node->Init(new_page_id, INVALID_PAGE_ID, internal_max_size_);
    // 新修改
    new_root_node->Init(new_page_id, INVALID_PAGE_ID, processor_.GetKeySize(), internal_max_size_);

    new_root_node->PopulateNewRoot(old_node->GetPageId(), key, new_node->GetPageId());
    old_node->SetParentPageId(new_page_id);
    new_node->SetParentPageId(new_page_id);
    buffer_pool_manager_->UnpinPage(new_page->GetPageId(), true);
    UpdateRootPageId(0);

    return;
  }

  Page *parent_page = buffer_pool_manager_->FetchPage(old_node->GetParentPageId());
  InternalPage *parent_node = reinterpret_cast<InternalPage *>(parent_page->GetData());
  parent_node->InsertNodeAfter(old_node->GetPageId(), key, new_node->GetPageId());
  if (parent_node->GetSize() < parent_node->GetMaxSize()) {
    buffer_pool_manager_->UnpinPage(parent_page->GetPageId(), true);
    return;
  }
  auto *new_parent_node = Split(parent_node,transaction);
  InsertIntoParent(parent_node, new_parent_node->KeyAt(0), new_parent_node, transaction);
  buffer_pool_manager_->UnpinPage(parent_page->GetPageId(), true);
  buffer_pool_manager_->UnpinPage(new_parent_node->GetPageId(), true);
}

/*****************************************************************************
 * REMOVE
 *****************************************************************************/
/*
 * Delete key & value pair associated with input key
 * If current tree is empty, return immediately.
 * If not, User needs to first find the right leaf page as deletion target, then
 * delete entry from leaf page. Remember to deal with redistribute or merge if
 * necessary.
 */
void BPlusTree::Remove(const GenericKey *key, Txn *transaction) {
  if(this->IsEmpty()){
    return;
  }
  Page* key_in_page=FindLeafPage(key,false);
  LeafPage* key_in_node=reinterpret_cast<LeafPage*>(key_in_page->GetData());
  int before_remove_size;
  before_remove_size=key_in_node->GetSize();
  int after_remove_size;
  after_remove_size=key_in_node->RemoveAndDeleteRecord(key,this->processor_);
  if(before_remove_size != after_remove_size){
    bool flag;
    flag=CoalesceOrRedistribute(key_in_node,transaction);
    buffer_pool_manager_->UnpinPage(key_in_page->GetPageId(),true);
    if(flag==true){
      buffer_pool_manager_->DeletePage(key_in_page->GetPageId());
    }
    return;
  }
  else if(before_remove_size==after_remove_size){
    buffer_pool_manager_->UnpinPage(key_in_page->GetPageId(),false);
    return;
  }
}

/* todo
 * User needs to first find the sibling of input page. If sibling's size + input
 * page's size > page's max size, then redistribute. Otherwise, merge.
 * Using template N to represent either internal page or leaf page.
 * @return: true means target leaf page should be deleted, false means no
 * deletion happens
 */
template <typename N>
bool BPlusTree::CoalesceOrRedistribute(N *&node, Txn *transaction) {
  bool flag;
  if(node->IsRootPage()==true){ //如果根节点，根据情况更新根节点
    flag=AdjustRoot(node);
    return flag;
  }
  else{
    if(node->GetSize()>=node->GetMinSize()){  //不用merge也不用redistribute
      return false;
    }
    Page *parent_page=buffer_pool_manager_->FetchPage(node->GetParentPageId());
    InternalPage *parent_node=reinterpret_cast<InternalPage*>(parent_page->GetData());
    int i;
    i=parent_node->ValueIndex(node->GetPageId());
    Page *sibling_page;
    N *sibling_node;
    if(i==0){
      sibling_page=buffer_pool_manager_->FetchPage(parent_node->ValueAt(1));
      sibling_node=reinterpret_cast<N*>(sibling_page->GetData());
    }
    else{
      sibling_page=buffer_pool_manager_->FetchPage(parent_node->ValueAt(i-1));
      sibling_node=reinterpret_cast<N*>(sibling_page->GetData());
    }
    if(sibling_node->GetSize()+node->GetSize()>node->GetMaxSize()){
      Redistribute(sibling_node,node,i);
      sibling_page->WUnlatch();
      buffer_pool_manager_->UnpinPage(sibling_page->GetPageId(),true);
      buffer_pool_manager_->UnpinPage(parent_page->GetPageId(),true);
      return false;
    }
    else{
      bool flag;
      flag=Coalesce(sibling_node,node,parent_node,i,transaction);
      if(flag==true){
        buffer_pool_manager_->DeletePage(parent_node->GetPageId());
      }
      sibling_page->WUnlatch();
      buffer_pool_manager_->UnpinPage(sibling_page->GetPageId(),true);
      buffer_pool_manager_->UnpinPage(parent_page->GetPageId(),true);
      return true;
    }
  }
  return false;
}

/*
 * Move all the key & value pairs from one page to its sibling page, and notify
 * buffer pool manager to delete this page. Parent page must be adjusted to
 * take info of deletion into account. Remember to deal with coalesce or
 * redistribute recursively if necessary.
 * Using template N to represent either internal page or leaf page.
 * @param   neighbor_node      sibling page of input "node"
 * @param   node               input from method coalesceOrRedistribute()
 * @param   parent             parent page of input "node"
 * @return  true means parent node should be deleted, false means no deletion happened
 */
bool BPlusTree::Coalesce(LeafPage *&neighbor_node, LeafPage *&node, InternalPage *&parent, int index,
                         Txn *transaction) {
  GenericKey* middle_key;
  if(index==0){ //node是parent的第一个孩子
    std::swap(neighbor_node,node);  //确保sibling node在node前面
    middle_key=parent->KeyAt(1);
  }
  middle_key=parent->KeyAt(index);
  if(node->IsLeafPage()==true){
    LeafPage *this_leaf=reinterpret_cast<LeafPage*>(node);
    LeafPage *sibling_leaf=reinterpret_cast<LeafPage*>(neighbor_node);
    sibling_leaf->SetNextPageId(this_leaf->GetNextPageId());
    this_leaf->MoveAllTo(sibling_leaf);
  }
  else if(node->IsLeafPage()==false){
    InternalPage *this_internal=reinterpret_cast<InternalPage*>(node);
    InternalPage *sibling_internal=reinterpret_cast<InternalPage*>(neighbor_node);
    this_internal->MoveAllTo(sibling_internal,middle_key,buffer_pool_manager_);
  }
  if(index==0){
    parent->Remove(1);
  }
  else{
    parent->Remove(index);
  }
  return CoalesceOrRedistribute(parent,transaction);
}

bool BPlusTree::Coalesce(InternalPage *&neighbor_node, InternalPage *&node, InternalPage *&parent, int index,
                         Txn *transaction) {
  GenericKey* middle_key;
  if(index==0){ //node是parent的第一个孩子
    std::swap(neighbor_node,node);  //确保sibling node在node前面
    middle_key=parent->KeyAt(1);
  }
  middle_key=parent->KeyAt(index);
  if(node->IsLeafPage()==true){
    LeafPage *this_leaf=reinterpret_cast<LeafPage*>(node);
    LeafPage *sibling_leaf=reinterpret_cast<LeafPage*>(neighbor_node);
    sibling_leaf->SetNextPageId(this_leaf->GetNextPageId());
    this_leaf->MoveAllTo(sibling_leaf);
  }
  else if(node->IsLeafPage()==false){
    InternalPage *this_internal=reinterpret_cast<InternalPage*>(node);
    InternalPage *sibling_internal=reinterpret_cast<InternalPage*>(neighbor_node);
    this_internal->MoveAllTo(sibling_internal,middle_key,buffer_pool_manager_);
  }
  if(index==0){
    parent->Remove(1);
  }
  else{
    parent->Remove(index);
  }
  return CoalesceOrRedistribute(parent,transaction);
}

/*
 * Redistribute key & value pairs from one page to its sibling page. If index ==
 * 0, move sibling page's first key & value pair into end of input "node",
 * otherwise move sibling page's last key & value pair into head of input
 * "node".
 * Using template N to represent either internal page or leaf page.
 * @param   neighbor_node      sibling page of input "node"
 * @param   node               input from method coalesceOrRedistribute()
 */

void BPlusTree::Redistribute(LeafPage *neighbor_node, LeafPage *node, int index) {
  if(index==0){
    if(node->IsLeafPage()==true){ //如果这个结点是叶子
      LeafPage* this_leaf=reinterpret_cast<LeafPage*>(node);
      LeafPage* sibling_leaf=reinterpret_cast<LeafPage*>(neighbor_node);
      sibling_leaf->MoveFirstToEndOf(this_leaf);//将sibling的第一个key放入这个点的最后

      Page* page_of_parent=buffer_pool_manager_->FetchPage(node->GetParentPageId());
      InternalPage* node_of_parent=reinterpret_cast<InternalPage*>(page_of_parent->GetData());
      node_of_parent->SetKeyAt(1,sibling_leaf->KeyAt(0)); //将这个点的父节点的key更新
      buffer_pool_manager_->UnpinPage(page_of_parent->GetPageId(),true);
    }
    else if(node->IsLeafPage()==false){//如果这个结点是内部结点
      InternalPage* this_internal=reinterpret_cast<InternalPage*>(node);
      InternalPage* sibling_internal=reinterpret_cast<InternalPage*>(neighbor_node);
      Page* page_of_parent=buffer_pool_manager_->FetchPage(node->GetParentPageId());
      InternalPage* node_of_parent=reinterpret_cast<InternalPage*>(page_of_parent->GetData());
      sibling_internal->MoveFirstToEndOf(this_internal,node_of_parent->KeyAt(1),this->buffer_pool_manager_);//将sibling的第一个key放入这个点的最后
      node_of_parent->SetKeyAt(1,sibling_internal->KeyAt(0));
      buffer_pool_manager_->UnpinPage(page_of_parent->GetPageId(),true);
    }
  }
  //move sibling page's last key & value pair into head of input
  else{
    if(node->IsLeafPage()==true){ //如果这个结点是叶子
      LeafPage* this_leaf=reinterpret_cast<LeafPage*>(node);
      LeafPage* sibling_leaf=reinterpret_cast<LeafPage*>(neighbor_node);
      sibling_leaf->MoveLastToFrontOf(this_leaf);

      Page* page_of_parent=buffer_pool_manager_->FetchPage(node->GetParentPageId());
      InternalPage* node_of_parent=reinterpret_cast<InternalPage*>(page_of_parent->GetData());
      node_of_parent->SetKeyAt(index,this_leaf->KeyAt(0)); //将这个点的父节点的key更新
      buffer_pool_manager_->UnpinPage(page_of_parent->GetPageId(),true);
    }
    else if(node->IsLeafPage()==false){//如果这个结点是内部结点
      InternalPage* this_internal=reinterpret_cast<InternalPage*>(node);
      InternalPage* sibling_internal=reinterpret_cast<InternalPage*>(neighbor_node);
      Page* page_of_parent=buffer_pool_manager_->FetchPage(node->GetParentPageId());
      InternalPage* node_of_parent=reinterpret_cast<InternalPage*>(page_of_parent->GetData());
      sibling_internal->MoveLastToFrontOf(this_internal,node_of_parent->KeyAt(index),this->buffer_pool_manager_);
      node_of_parent->SetKeyAt(index,this_internal->KeyAt(0));
      buffer_pool_manager_->UnpinPage(page_of_parent->GetPageId(),true);
    }
  }
}
void BPlusTree::Redistribute(InternalPage *neighbor_node, InternalPage *node, int index) {
  if(index==0){
    if(node->IsLeafPage()==true){ //如果这个结点是叶子
      LeafPage* this_leaf=reinterpret_cast<LeafPage*>(node);
      LeafPage* sibling_leaf=reinterpret_cast<LeafPage*>(neighbor_node);
      sibling_leaf->MoveFirstToEndOf(this_leaf);//将sibling的第一个key放入这个点的最后

      Page* page_of_parent=buffer_pool_manager_->FetchPage(node->GetParentPageId());
      InternalPage* node_of_parent=reinterpret_cast<InternalPage*>(page_of_parent->GetData());
      node_of_parent->SetKeyAt(1,sibling_leaf->KeyAt(0)); //将这个点的父节点的key更新
      buffer_pool_manager_->UnpinPage(page_of_parent->GetPageId(),true);
    }
    else if(node->IsLeafPage()==false){//如果这个结点是内部结点
      InternalPage* this_internal=reinterpret_cast<InternalPage*>(node);
      InternalPage* sibling_internal=reinterpret_cast<InternalPage*>(neighbor_node);
      Page* page_of_parent=buffer_pool_manager_->FetchPage(node->GetParentPageId());
      InternalPage* node_of_parent=reinterpret_cast<InternalPage*>(page_of_parent->GetData());
      sibling_internal->MoveFirstToEndOf(this_internal,node_of_parent->KeyAt(1),this->buffer_pool_manager_);//将sibling的第一个key放入这个点的最后
      node_of_parent->SetKeyAt(1,sibling_internal->KeyAt(0));
      buffer_pool_manager_->UnpinPage(page_of_parent->GetPageId(),true);
    }
  }
  //move sibling page's last key & value pair into head of input
  else{
    if(node->IsLeafPage()==true){ //如果这个结点是叶子
      LeafPage* this_leaf=reinterpret_cast<LeafPage*>(node);
      LeafPage* sibling_leaf=reinterpret_cast<LeafPage*>(neighbor_node);
      sibling_leaf->MoveLastToFrontOf(this_leaf);

      Page* page_of_parent=buffer_pool_manager_->FetchPage(node->GetParentPageId());
      InternalPage* node_of_parent=reinterpret_cast<InternalPage*>(page_of_parent->GetData());
      node_of_parent->SetKeyAt(index,this_leaf->KeyAt(0)); //将这个点的父节点的key更新
      buffer_pool_manager_->UnpinPage(page_of_parent->GetPageId(),true);
    }
    else if(node->IsLeafPage()==false){//如果这个结点是内部结点
      InternalPage* this_internal=reinterpret_cast<InternalPage*>(node);
      InternalPage* sibling_internal=reinterpret_cast<InternalPage*>(neighbor_node);
      Page* page_of_parent=buffer_pool_manager_->FetchPage(node->GetParentPageId());
      InternalPage* node_of_parent=reinterpret_cast<InternalPage*>(page_of_parent->GetData());
      sibling_internal->MoveLastToFrontOf(this_internal,node_of_parent->KeyAt(index),this->buffer_pool_manager_);
      node_of_parent->SetKeyAt(index,this_internal->KeyAt(0));
      buffer_pool_manager_->UnpinPage(page_of_parent->GetPageId(),true);
    }
  }
}
/*
 * Update root page if necessary
 * NOTE: size of root page can be less than min size and this method is only
 * called within coalesceOrRedistribute() method
 * case 1: when you delete the last element in root page, but root page still
 * has one last child
 * case 2: when you delete the last element in whole b+ tree
 * @return : true means root page should be deleted, false means no deletion
 * happened
 */
bool BPlusTree::AdjustRoot(BPlusTreePage *old_root_node) {
  if(old_root_node->IsLeafPage()){
    if(old_root_node->GetSize()==0){ //彻底空了
      this->root_page_id_=INVALID_PAGE_ID;
      UpdateRootPageId(0);
      return true;
    }
    return false;
  }
  //case 1: when you delete the last element in root page, but root page still has one last child
  else if(old_root_node->IsLeafPage()==false&&old_root_node->GetSize()==1){ //空了，（因为是从1开始的）
    InternalPage * this_node=reinterpret_cast<InternalPage*>(old_root_node);
    page_id_t id_of_rootchild;
    id_of_rootchild=this_node->RemoveAndReturnOnlyChild();

    this->root_page_id_=id_of_rootchild;
    Page* root_page_new;
    InternalPage *root_node_new;
    root_page_new=buffer_pool_manager_->FetchPage(this->root_page_id_);
    root_node_new=reinterpret_cast<InternalPage*>(root_page_new->GetData());

    UpdateRootPageId(0);
    root_node_new->SetParentPageId(INVALID_PAGE_ID);
    buffer_pool_manager_->UnpinPage(this->root_page_id_,true);
    return true;
  }
  return false;
}

/*****************************************************************************
 * INDEX ITERATOR
 *****************************************************************************/
/*
 * Input parameter is void, find the left most leaf page first, then construct
 * index iterator
 * @return : index iterator
 */
IndexIterator BPlusTree::Begin() {
  if (IsEmpty()) {
    return End();
  }
  Page *page = FindLeafPage(nullptr, root_page_id_, true);
  LeafPage *leaf = reinterpret_cast<LeafPage *>(page->GetData());  
  return IndexIterator(leaf->GetPageId(), buffer_pool_manager_, 0);  
}

/*
 * Input parameter is low key, find the leaf page that contains the input key
 * first, then construct index iterator
 * @return : index iterator
 */
IndexIterator BPlusTree::Begin(const GenericKey *key) {
  if (IsEmpty()) {
    return End();
  }
  Page *page = FindLeafPage(key, root_page_id_, false);

  LeafPage *leaf = reinterpret_cast<LeafPage *>(page->GetData());
  int index = leaf->KeyIndex(key, processor_);
  return IndexIterator(leaf->GetPageId(), buffer_pool_manager_, index);
}

/*
 * Input parameter is void, construct an index iterator representing the end
 * of the key/value pair in the leaf node
 * @return : index iterator
 */
IndexIterator BPlusTree::End() {
  // return IndexIterator(INVALID_PAGE_ID, nullptr, 0);

  // 新修改
  Page *page = FindLeafPage(nullptr, root_page_id_, true);
  LeafPage *leaf = reinterpret_cast<LeafPage *>(page->GetData());  
  return IndexIterator(leaf->GetPageId(), buffer_pool_manager_, leaf->GetSize());  
}

/*****************************************************************************
 * UTILITIES AND DEBUG
 *****************************************************************************/
/*
 * Find leaf page containing particular key, if leftMost flag == true, find
 * the left most leaf page
 * Note: the leaf page is pinned, you need to unpin it after use.
 */

Page *BPlusTree::FindLeafPage(const GenericKey *key, page_id_t page_id, bool leftMost) {

  auto *cur_page = buffer_pool_manager_->FetchPage(root_page_id_);
  auto *cur_bptree_page = reinterpret_cast<BPlusTreePage *>(cur_page->GetData());

  //不断向下直到叶子
  while(cur_bptree_page->IsLeafPage() != true){
    auto *internal_page = reinterpret_cast<InternalPage *>(cur_bptree_page);
    //确定下一页在哪
    page_id_t next_page_id;
    if(leftMost==true){
      next_page_id = internal_page->ValueAt(0);
      std::cout<<"true"<<endl;
    }
    else{
      next_page_id = internal_page->Lookup(key, processor_);
      if(next_page_id<0){
        next_page_id+=2;
      }
    }
    //迭代前往下一页
    cur_page = buffer_pool_manager_->FetchPage(next_page_id);
    cur_bptree_page = reinterpret_cast<BPlusTreePage *>(cur_page->GetData());
    buffer_pool_manager_->UnpinPage(internal_page->GetPageId(), false);
  }
  //到达叶子页面，退出循环
  return reinterpret_cast<Page *>(cur_bptree_page);
}


/*
 * Update/Insert root page id in header page(where page_id = 0, header_page is
 * defined under include/page/header_page.h)
 * Call this method everytime root page id is changed.
 * @parameter: insert_record      default value is false. When set to true,
 * insert a record <index_name, current_page_id> into header page instead of
 * updating it.
 */
void BPlusTree::UpdateRootPageId(int insert_record) {
  auto *page = buffer_pool_manager_->FetchPage(INDEX_ROOTS_PAGE_ID);
  auto *root_page = reinterpret_cast<IndexRootsPage *>(page->GetData());
  if(insert_record == 1){ //插入(新建)
    root_page->Insert(index_id_, root_page_id_);
  }
  else{ //更新
    root_page->Update(index_id_, root_page_id_);
  }
  buffer_pool_manager_->UnpinPage(INDEX_ROOTS_PAGE_ID, true);
  buffer_pool_manager_->UnpinPage(root_page_id_, true);
}

/**
 * This method is used for debug only, You don't need to modify
 */
void BPlusTree::ToGraph(BPlusTreePage *page, BufferPoolManager *bpm, std::ofstream &out, Schema *schema) const {
  std::string leaf_prefix("LEAF_");
  std::string internal_prefix("INT_");
  if (page->IsLeafPage()) {
    auto *leaf = reinterpret_cast<LeafPage *>(page);
    // Print node name
    out << leaf_prefix << leaf->GetPageId();
    // Print node properties
    out << "[shape=plain color=green ";
    // Print data of the node
    out << "label=<<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"4\">\n";
    // Print data
    out << "<TR><TD COLSPAN=\"" << leaf->GetSize() << "\">P=" << leaf->GetPageId()
        << ",Parent=" << leaf->GetParentPageId() << "</TD></TR>\n";
    out << "<TR><TD COLSPAN=\"" << leaf->GetSize() << "\">"
        << "max_size=" << leaf->GetMaxSize() << ",min_size=" << leaf->GetMinSize() << ",size=" << leaf->GetSize()
        << "</TD></TR>\n";
    out << "<TR>";
    for (int i = 0; i < leaf->GetSize(); i++) {
      Row ans;
      processor_.DeserializeToKey(leaf->KeyAt(i), ans, schema);
      out << "<TD>" << ans.GetField(0)->toString() << "</TD>\n";
    }
    out << "</TR>";
    // Print table end
    out << "</TABLE>>];\n";
    // Print Leaf node link if there is a next page
    if (leaf->GetNextPageId() != INVALID_PAGE_ID) {
      out << leaf_prefix << leaf->GetPageId() << " -> " << leaf_prefix << leaf->GetNextPageId() << ";\n";
      out << "{rank=same " << leaf_prefix << leaf->GetPageId() << " " << leaf_prefix << leaf->GetNextPageId() << "};\n";
    }

    // Print parent links if there is a parent
    if (leaf->GetParentPageId() != INVALID_PAGE_ID) {
      out << internal_prefix << leaf->GetParentPageId() << ":p" << leaf->GetPageId() << " -> " << leaf_prefix
          << leaf->GetPageId() << ";\n";
    }
  } else {
    auto *inner = reinterpret_cast<InternalPage *>(page);
    // Print node name
    out << internal_prefix << inner->GetPageId();
    // Print node properties
    out << "[shape=plain color=pink ";  // why not?
    // Print data of the node
    out << "label=<<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"4\">\n";
    // Print data
    out << "<TR><TD COLSPAN=\"" << inner->GetSize() << "\">P=" << inner->GetPageId()
        << ",Parent=" << inner->GetParentPageId() << "</TD></TR>\n";
    out << "<TR><TD COLSPAN=\"" << inner->GetSize() << "\">"
        << "max_size=" << inner->GetMaxSize() << ",min_size=" << inner->GetMinSize() << ",size=" << inner->GetSize()
        << "</TD></TR>\n";
    out << "<TR>";
    for (int i = 0; i < inner->GetSize(); i++) {
      out << "<TD PORT=\"p" << inner->ValueAt(i) << "\">";
      if (i > 0) {
        Row ans;
        processor_.DeserializeToKey(inner->KeyAt(i), ans, schema);
        out << ans.GetField(0)->toString();
      } else {
        out << " ";
      }
      out << "</TD>\n";
    }
    out << "</TR>";
    // Print table end
    out << "</TABLE>>];\n";
    // Print Parent link
    if (inner->GetParentPageId() != INVALID_PAGE_ID) {
      out << internal_prefix << inner->GetParentPageId() << ":p" << inner->GetPageId() << " -> " << internal_prefix
          << inner->GetPageId() << ";\n";
    }
    // Print leaves
    for (int i = 0; i < inner->GetSize(); i++) {
      auto child_page = reinterpret_cast<BPlusTreePage *>(bpm->FetchPage(inner->ValueAt(i))->GetData());
      ToGraph(child_page, bpm, out, schema);
      if (i > 0) {
        auto sibling_page = reinterpret_cast<BPlusTreePage *>(bpm->FetchPage(inner->ValueAt(i - 1))->GetData());
        if (!sibling_page->IsLeafPage() && !child_page->IsLeafPage()) {
          out << "{rank=same " << internal_prefix << sibling_page->GetPageId() << " " << internal_prefix
              << child_page->GetPageId() << "};\n";
        }
        bpm->UnpinPage(sibling_page->GetPageId(), false);
      }
    }
  }
  bpm->UnpinPage(page->GetPageId(), false);
}

/**
 * This function is for debug only, you don't need to modify
 */
void BPlusTree::ToString(BPlusTreePage *page, BufferPoolManager *bpm) const {
  if (page->IsLeafPage()) {
    auto *leaf = reinterpret_cast<LeafPage *>(page);
    std::cout << "Leaf Page: " << leaf->GetPageId() << " parent: " << leaf->GetParentPageId()
              << " next: " << leaf->GetNextPageId() << std::endl;
    for (int i = 0; i < leaf->GetSize(); i++) {
      std::cout << leaf->KeyAt(i) << ",";
    }
    std::cout << std::endl;
    std::cout << std::endl;
  } else {
    auto *internal = reinterpret_cast<InternalPage *>(page);
    std::cout << "Internal Page: " << internal->GetPageId() << " parent: " << internal->GetParentPageId() << std::endl;
    for (int i = 0; i < internal->GetSize(); i++) {
      std::cout << internal->KeyAt(i) << ": " << internal->ValueAt(i) << ",";
    }
    std::cout << std::endl;
    std::cout << std::endl;
    for (int i = 0; i < internal->GetSize(); i++) {
      ToString(reinterpret_cast<BPlusTreePage *>(bpm->FetchPage(internal->ValueAt(i))->GetData()), bpm);
      bpm->UnpinPage(internal->ValueAt(i), false);
    }
  }
}

bool BPlusTree::Check() {
  bool all_unpinned = buffer_pool_manager_->CheckAllUnpinned();
  if (!all_unpinned) {
    LOG(ERROR) << "problem in page unpin" << endl;
  }
  return all_unpinned;
}