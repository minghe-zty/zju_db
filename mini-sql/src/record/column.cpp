#include "record/column.h"
#include "glog/logging.h"

Column::Column(std::string column_name, TypeId type, uint32_t index, bool nullable, bool unique)
    : name_(std::move(column_name)), type_(type), table_ind_(index), nullable_(nullable), unique_(unique) {
  ASSERT(type != TypeId::kTypeChar, "Wrong constructor for CHAR type.");
  switch (type) {
    case TypeId::kTypeInt:
      len_ = sizeof(int32_t);
      break;
    case TypeId::kTypeFloat:
      len_ = sizeof(float_t);
      break;
    default:
      ASSERT(false, "Unsupported column type.");
  }
}

Column::Column(std::string column_name, TypeId type, uint32_t length, uint32_t index, bool nullable, bool unique)
    : name_(std::move(column_name)),
      type_(type),
      len_(length),
      table_ind_(index),
      nullable_(nullable),
      unique_(unique) {
  ASSERT(type == TypeId::kTypeChar, "Wrong constructor for non-VARCHAR type.");
}

Column::Column(const Column *other)
    : name_(other->name_),
      type_(other->type_),
      len_(other->len_),
      table_ind_(other->table_ind_),
      nullable_(other->nullable_),
      unique_(other->unique_) {}

uint32_t Column::SerializeTo(char *buf) const {
  char *start = buf;

  // Write the magic number
  MACH_WRITE_UINT32(buf, COLUMN_MAGIC_NUM);
  buf += sizeof(uint32_t);

  // Write the column name length and name
  uint32_t name_len = name_.length();
  MACH_WRITE_UINT32(buf, name_len);
  buf += sizeof(uint32_t);
  memcpy(buf, name_.c_str(), name_len);
  buf += name_len;

  // Write the type
  MACH_WRITE_UINT32(buf, static_cast<uint32_t>(type_));
  buf += sizeof(uint32_t);

  // Write the length
  MACH_WRITE_UINT32(buf, len_);
  buf += sizeof(uint32_t);

  // Write the table index
  MACH_WRITE_UINT32(buf, table_ind_);
  buf += sizeof(uint32_t);

  // Write the nullable flag
  MACH_WRITE_UINT32(buf, static_cast<uint32_t>(nullable_));
  buf += sizeof(uint32_t);

  // Write the unique flag
  MACH_WRITE_UINT32(buf, static_cast<uint32_t>(unique_));
  buf += sizeof(uint32_t);

  return buf - start;
}

uint32_t Column::DeserializeFrom(char *buf, Column *&column) {
  char *start = buf;

  // Read the magic number
  uint32_t magic_num = MACH_READ_UINT32(buf);
  ASSERT(magic_num == COLUMN_MAGIC_NUM, "Magic number mismatch during deserialize.");
  buf += sizeof(uint32_t);

  // Read the column name length and name
  uint32_t name_len = MACH_READ_UINT32(buf);
  buf += sizeof(uint32_t);
  std::string name(buf, name_len);
  buf += name_len;

  // Read the type
  TypeId type = static_cast<TypeId>(MACH_READ_UINT32(buf));
  buf += sizeof(uint32_t);

  // Read the length
  uint32_t length = MACH_READ_UINT32(buf);
  buf += sizeof(uint32_t);

  // Read the table index
  uint32_t table_ind = MACH_READ_UINT32(buf);
  buf += sizeof(uint32_t);

  // Read the nullable flag
  bool nullable = static_cast<bool>(MACH_READ_UINT32(buf));
  buf += sizeof(uint32_t);

  // Read the unique flag
  bool unique = static_cast<bool>(MACH_READ_UINT32(buf));
  buf += sizeof(uint32_t);

  // Create the Column object
  if (type == TypeId::kTypeChar) {
    column = new Column(name, type, length, table_ind, nullable, unique);
  } else {
    column = new Column(name, type, table_ind, nullable, unique);
  }

  return buf - start;
}

uint32_t Column::GetSerializedSize() const {
  uint32_t size = 0;
  size += sizeof(uint32_t); // MAGIC_NUM
  size += sizeof(uint32_t); // Column name length
  size += name_.length();   // Column name
  size += sizeof(uint32_t); // Type
  size += sizeof(uint32_t); // Length
  size += sizeof(uint32_t); // Table index
  size += sizeof(uint32_t); // Nullable flag
  size += sizeof(uint32_t); // Unique flag
  return size;
}
