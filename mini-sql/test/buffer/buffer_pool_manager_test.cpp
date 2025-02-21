#include "buffer/buffer_pool_manager.h"

#include <cstdio>
#include <random>
#include <string>

#include "gtest/gtest.h"

TEST(BufferPoolManagerTest, BinaryDataTest) {
  const std::string db_name = "bpm_test.db";
  const size_t buffer_pool_size = 10;

  std::random_device r;
  std::default_random_engine rng(r());
  std::uniform_int_distribution<unsigned> uniform_dist(0, 127);

  remove(db_name.c_str());
  auto *disk_manager = new DiskManager(db_name);
  auto *bpm = new BufferPoolManager(buffer_pool_size, disk_manager);

  page_id_t page_id_temp;
  auto *page0 = bpm->NewPage(page_id_temp);

  // Scenario: The buffer pool is empty. We should be able to create a new page.
  ASSERT_NE(nullptr, page0);
  EXPECT_EQ(0, page_id_temp);

  char random_binary_data[PAGE_SIZE];
  // Generate random binary data
  for (char &i : random_binary_data) {
    i = uniform_dist(rng);
  }

  // Insert terminal characters both in the middle and at end
  random_binary_data[PAGE_SIZE / 2] = '\0';
  random_binary_data[PAGE_SIZE - 1] = '\0';

  // Scenario: Once we have a page, we should be able to read and write content.
  std::memcpy(page0->GetData(), random_binary_data, PAGE_SIZE);
  EXPECT_EQ(0, std::memcmp(page0->GetData(), random_binary_data, PAGE_SIZE));

  // Scenario: We should be able to create new pages until we fill up the buffer pool.
  for (size_t i = 1; i < buffer_pool_size; ++i) {
    EXPECT_NE(nullptr, bpm->NewPage(page_id_temp));
    EXPECT_EQ(i, page_id_temp);
  }

  // Scenario: Once the buffer pool is full, we should not be able to create any new pages.
  for (size_t i = buffer_pool_size; i < buffer_pool_size * 2; ++i) {
    EXPECT_EQ(nullptr, bpm->NewPage(page_id_temp));
  }

  // Scenario: After unpinning pages {0, 1, 2, 3, 4} we should be able to create 5 new pages
  for (int i = 0; i < 5; ++i) {
    EXPECT_EQ(true, bpm->UnpinPage(i, true));
    EXPECT_TRUE(bpm->FlushPage(i));
  }
  for (int i = 0; i < 5; ++i) {
    EXPECT_NE(nullptr, bpm->NewPage(page_id_temp));
    EXPECT_EQ(buffer_pool_size + i, page_id_temp);
    bpm->UnpinPage(page_id_temp, false);
  }
  // Scenario: We should be able to fetch the data we wrote a while ago.
  page0 = bpm->FetchPage(0);
  EXPECT_EQ(0, memcmp(page0->GetData(), random_binary_data, PAGE_SIZE));
  EXPECT_EQ(true, bpm->UnpinPage(0, true));

  // Shutdown the disk manager and remove the temporary file we created.
  disk_manager->Close();
  remove(db_name.c_str());

  delete bpm;
  delete disk_manager;
}


TEST(BufferPoolManagerTest, BasicPageCreation) {
  const std::string db_name = "bpm_test.db";
  const size_t buffer_pool_size = 10;
  remove(db_name.c_str());

  DiskManager* disk_manager = new DiskManager(db_name);
  auto* bpm = new BufferPoolManager(buffer_pool_size, disk_manager);

  page_id_t page_id;
  auto* page = bpm->NewPage(page_id);
  ASSERT_NE(nullptr, page) << "Failed to create a new page in an empty buffer pool.";
  EXPECT_EQ(0, page_id) << "The first page ID should be 0.";

  delete bpm;
  delete disk_manager;
  ASSERT_EQ(0, remove(db_name.c_str())) << "Failed to remove test database file.";
}

TEST(BufferPoolManagerTest, BinaryDataHandling) {
  const std::string db_name = "bpm_test.db";
  const size_t buffer_pool_size = 10;
  std::random_device r;
  std::default_random_engine rng(r());
  std::uniform_int_distribution<unsigned> uniform_dist(0, 127);

  DiskManager* disk_manager = new DiskManager(db_name);
  auto* bpm = new BufferPoolManager(buffer_pool_size, disk_manager);

  page_id_t page_id_temp;
  auto* page = bpm->NewPage(page_id_temp);
  ASSERT_NE(nullptr, page) << "Failed to allocate a new page.";

  char random_binary_data[PAGE_SIZE];
  for (char &c : random_binary_data) {
    c = uniform_dist(rng);
  }
  std::memcpy(page->GetData(), random_binary_data, PAGE_SIZE);
  EXPECT_EQ(0, std::memcmp(page->GetData(), random_binary_data, PAGE_SIZE));

  bpm->UnpinPage(page_id_temp, true);
  bpm->FlushPage(page_id_temp);

  delete bpm;
  delete disk_manager;
  ASSERT_EQ(0, remove(db_name.c_str())) << "Failed to remove test database file.";
}
