/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "common/type/vector_type.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <iomanip>
#include <sstream>

#include "common/log/log.h"
#include "common/value.h"

int VectorType::compare(const Value &left, const Value &right) const
{
  ASSERT(left.attr_type() == AttrType::VECTORS, "left type is not vector");
  if (right.attr_type() != AttrType::VECTORS) {
    return INT32_MAX;
  }
  if (left.length() != right.length()) {
    return left.length() < right.length() ? -1 : 1;
  }
  if (left.length() == 0) {
    return 0;
  }
  return memcmp(left.data(), right.data(), left.length()) == 0 ? 0 : -1;
}

RC VectorType::cast_to(const Value &val, AttrType type, Value &result) const
{
  if (type == AttrType::VECTORS) {
    result = val;
    return RC::SUCCESS;
  }
  if (type == AttrType::CHARS) {
    const int dim = val.length() / static_cast<int>(sizeof(float));
    vector<float> values(dim);
    if (dim > 0) {
      memcpy(values.data(), val.data(), val.length());
    }
    result.set_string(format(values).c_str());
    return RC::SUCCESS;
  }
  return RC::SCHEMA_FIELD_TYPE_MISMATCH;
}

RC VectorType::to_string(const Value &val, string &result) const
{
  if (val.attr_type() != AttrType::VECTORS) {
    return RC::SCHEMA_FIELD_TYPE_MISMATCH;
  }
  const int      dim = val.length() / static_cast<int>(sizeof(float));
  vector<float> values(dim);
  if (dim > 0) {
    memcpy(values.data(), val.data(), val.length());
  }
  result = format(values);
  return RC::SUCCESS;
}

RC VectorType::set_value_from_str(Value &val, const string &data) const
{
  vector<float> values;
  RC            rc = parse(data, values);
  if (OB_FAIL(rc)) {
    return rc;
  }
  val.set_vector(values.data(), static_cast<int>(values.size()));
  return RC::SUCCESS;
}

int VectorType::cast_cost(AttrType type)
{
  if (type == AttrType::VECTORS) {
    return 0;
  }
  return INT32_MAX;
}

RC VectorType::parse(const string &text, vector<float> &values)
{
  values.clear();

  size_t pos = 0;
  auto skip_space = [&text, &pos]() {
    while (pos < text.size() && isspace(static_cast<unsigned char>(text[pos]))) {
      pos++;
    }
  };

  skip_space();
  if (pos >= text.size() || text[pos] != '[') {
    return RC::INVALID_ARGUMENT;
  }
  pos++;

  skip_space();
  if (pos < text.size() && text[pos] == ']') {
    return RC::INVALID_ARGUMENT;  // 空向量不允许
  }

  while (pos < text.size()) {
    skip_space();
    if (pos >= text.size()) {
      return RC::INVALID_ARGUMENT;
    }

    const char *start = text.c_str() + pos;
    char       *end   = nullptr;
    float       value = strtof(start, &end);
    if (end == start) {
      return RC::INVALID_ARGUMENT;
    }
    values.push_back(value);
    pos = static_cast<size_t>(end - text.c_str());

    skip_space();
    if (pos >= text.size()) {
      return RC::INVALID_ARGUMENT;
    }
    if (text[pos] == ',') {
      pos++;
      continue;
    }
    if (text[pos] == ']') {
      pos++;
      break;
    }
    return RC::INVALID_ARGUMENT;
  }

  skip_space();
  if (pos != text.size() || values.empty()) {
    return RC::INVALID_ARGUMENT;
  }
  return RC::SUCCESS;
}

string VectorType::format(const vector<float> &values)
{
  std::ostringstream oss;
  oss << '[';
  for (size_t i = 0; i < values.size(); i++) {
    if (i > 0) {
      oss << ',';
    }
    oss << std::scientific << std::setprecision(5) << values[i];
  }
  oss << ']';
  return oss.str();
}
