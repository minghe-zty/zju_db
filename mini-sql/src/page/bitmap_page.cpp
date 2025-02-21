#include "page/bitmap_page.h"

#include "glog/logging.h"

/**
 * TODO: Student Implement
 */
template <size_t PageSize>
bool BitmapPage<PageSize>::AllocatePage(uint32_t &page_offset) {
  if (page_allocated_ >= GetMaxSupportedSize()) {
    return false;  // No more pages to allocate
  }

  for (size_t byte_index = 0; byte_index < MAX_CHARS; ++byte_index) {
    if (bytes[byte_index] != 0xFF) {  // There is at least one free page in this byte
      for (uint8_t bit_index = 0; bit_index < 8; ++bit_index) {
        if (!(bytes[byte_index] & (1 << bit_index))) {
          bytes[byte_index] |= (1 << bit_index);
          page_allocated_++;
          page_offset = byte_index * 8 + bit_index;
          next_free_page_ = page_offset + 1;
          return true;
        }
      }
    }
  }

  return false;
}

/**
 * TODO: Student Implement
 */
template <size_t PageSize>
bool BitmapPage<PageSize>::DeAllocatePage(uint32_t page_offset) {
  if (page_offset >= GetMaxSupportedSize()) {
    return false;
  }

  size_t byte_index = page_offset / 8;
  uint8_t bit_index = page_offset % 8;

  if (bytes[byte_index] & (1 << bit_index)) {
    bytes[byte_index] &= ~(1 << bit_index);
    page_allocated_--;
    next_free_page_ = page_offset;
    return true;
  }

  return false;
}


/**
 * TODO: Student Implement
 */
template <size_t PageSize>
bool BitmapPage<PageSize>::IsPageFree(uint32_t page_offset) const {
  if (page_offset >= GetMaxSupportedSize()) {
    return false;
  }

  size_t byte_index = page_offset / 8;
  uint8_t bit_index = page_offset % 8;

  return !(bytes[byte_index] & (1 << bit_index));
}

template <size_t PageSize>
bool BitmapPage<PageSize>::IsPageFreeLow(uint32_t byte_index, uint8_t bit_index) const {
  return false;
}
;

template class BitmapPage<64>;

template class BitmapPage<128>;

template class BitmapPage<256>;

template class BitmapPage<512>;

template class BitmapPage<1024>;

template class BitmapPage<2048>;

template class BitmapPage<4096>;

