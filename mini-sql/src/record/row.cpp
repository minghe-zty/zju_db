#include "record/row.h"
#define MAGIC_NUM 1086

uint32_t Row::SerializeTo(char *buf, Schema *schema) const {
  ASSERT(schema != nullptr, "Invalid schema before serialize.");
  ASSERT(schema->GetColumnCount() == fields_.size(), "Fields size do not match schema's column size.");

  char *start = buf;

  // Write the magic number
  MACH_WRITE_UINT32(buf, MAGIC_NUM);
  buf += sizeof(uint32_t);

  // Write the number of fields
  uint32_t field_count = fields_.size();
  MACH_WRITE_UINT32(buf, field_count);
  buf += sizeof(uint32_t);

  // Write the null bitmap
  uint32_t null_bitmap = 0;
  for (size_t i = 0; i < field_count; ++i) {
    if (fields_[i]->IsNull()) {
      null_bitmap |= (1 << i);
    }
  }
  MACH_WRITE_UINT32(buf, null_bitmap);
  buf += sizeof(uint32_t);

  // Write each field
  for (size_t i = 0; i < field_count; ++i) {
    if (!fields_[i]->IsNull()) {
      fields_[i]->SerializeTo(buf);
      buf += fields_[i]->GetSerializedSize();
    }
  }

  return buf - start;
}

uint32_t Row::DeserializeFrom(char *buf, Schema *schema) {
  ASSERT(schema != nullptr, "Invalid schema before serialize.");
  ASSERT(fields_.empty(), "Non empty field in row.");

  char *start = buf;

  // Check the magic number
  uint32_t magic_num = MACH_READ_UINT32(buf);
  ASSERT(magic_num == MAGIC_NUM, "Magic number mismatch during deserialize.");
  buf += sizeof(uint32_t);

  // Read the number of fields
  uint32_t field_count = MACH_READ_UINT32(buf);
  buf += sizeof(uint32_t);

  // Read the null bitmap
  uint32_t null_bitmap = MACH_READ_UINT32(buf);
  buf += sizeof(uint32_t);

  // Deserialize each field
  for (size_t i = 0; i < field_count; ++i) {
    if (null_bitmap & (1 << i)) {
      fields_.push_back(new Field(TypeId::kTypeInvalid)); // Assuming kTypeInvalid denotes a null field
    } else {
      auto column = schema->GetColumn(i);
      Field *field = nullptr;
      Field::DeserializeFrom(buf, column->GetType(), &field, false);
      fields_.push_back(field);
      buf += field->GetSerializedSize();
    }
  }

  return buf - start;
}

uint32_t Row::GetSerializedSize(Schema *schema) const {
  ASSERT(schema != nullptr, "Invalid schema before serialize.");
  ASSERT(schema->GetColumnCount() == fields_.size(), "Fields size do not match schema's column size.");

  uint32_t size = 0;
  size += sizeof(uint32_t); // MAGIC_NUM
  size += sizeof(uint32_t); // Field count
  size += sizeof(uint32_t); // Null bitmap

  // Add size of each field
  for (auto field : fields_) {
    if (!field->IsNull()) {
      size += field->GetSerializedSize();
    }
  }

  return size;
}

void Row::GetKeyFromRow(const Schema *schema, const Schema *key_schema, Row &key_row) {
  auto columns = key_schema->GetColumns();
  std::vector<Field> fields;
  uint32_t idx;
  for (auto column : columns) {
    schema->GetColumnIndex(column->GetName(), idx);
    fields.emplace_back(*this->GetField(idx));
  }
  key_row = Row(fields);
}


