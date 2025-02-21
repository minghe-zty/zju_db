#include "record/schema.h"

uint32_t Schema::SerializeTo(char *buf) const {
  char *start = buf;

  // Write the magic number
  MACH_WRITE_UINT32(buf, SCHEMA_MAGIC_NUM);
  buf += sizeof(uint32_t);

  // Write the number of columns
  uint32_t column_count = columns_.size();
  MACH_WRITE_UINT32(buf, column_count);
  buf += sizeof(uint32_t);

  // Write each column
  for (const auto &column : columns_) {
    buf += column->SerializeTo(buf);
  }

  return buf - start;
}

uint32_t Schema::GetSerializedSize() const {
  uint32_t size = 0;
  size += sizeof(uint32_t); // MAGIC_NUM
  size += sizeof(uint32_t); // Column count

  // Add size of each column
  for (const auto &column : columns_) {
    size += column->GetSerializedSize();
  }

  return size;
}

uint32_t Schema::DeserializeFrom(char *buf, Schema *&schema) {
  char *start = buf;

  // Read the magic number
  uint32_t magic_num = MACH_READ_UINT32(buf);
  ASSERT(magic_num == SCHEMA_MAGIC_NUM, "Magic number mismatch during deserialize.");
  buf += sizeof(uint32_t);

  // Read the number of columns
  uint32_t column_count = MACH_READ_UINT32(buf);
  buf += sizeof(uint32_t);

  // Read each column
  std::vector<Column *> columns;
  columns.reserve(column_count);
  for (uint32_t i = 0; i < column_count; ++i) {
    Column *column = nullptr;
    buf += Column::DeserializeFrom(buf, column);
    columns.push_back(column);
  }

  schema = new Schema(columns, true);
  return buf - start;
}
