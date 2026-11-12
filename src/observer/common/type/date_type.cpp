/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "common/type/date_type.h"

#include <stdio.h>

#include "common/lang/comparator.h"
#include "common/log/log.h"
#include "common/value.h"
#include "storage/common/column.h"

bool DateType::is_leap_year(int year)
{
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int DateType::days_in_month(int year, int month)
{
  static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month == 2 && is_leap_year(year)) {
    return 29;
  }
  return days[month - 1];
}

bool DateType::parse(const char *str, int len, int &value)
{
  // 支持 YYYY-MM-DD，也支持月份和日期不补零的写法，例如 2020-1-1。
  if (str == nullptr || len < 8 || len > 10) {
    return false;
  }

  int pos  = 0;
  int year = 0;
  for (int i = 0; i < 4; i++, pos++) {
    if (str[pos] < '0' || str[pos] > '9') {
      return false;
    }
    year = year * 10 + (str[pos] - '0');
  }

  if (pos >= len || str[pos] != '-') {
    return false;
  }
  pos++;

  int month  = 0;
  int digits = 0;
  while (pos < len && str[pos] != '-' && digits < 2) {
    if (str[pos] < '0' || str[pos] > '9') {
      return false;
    }
    month = month * 10 + (str[pos] - '0');
    pos++;
    digits++;
  }
  if (digits == 0 || pos >= len || str[pos] != '-') {
    return false;
  }
  pos++;

  int day = 0;
  digits  = 0;
  while (pos < len && digits < 2) {
    if (str[pos] < '0' || str[pos] > '9') {
      return false;
    }
    day = day * 10 + (str[pos] - '0');
    pos++;
    digits++;
  }
  if (digits == 0 || pos != len) {
    return false;
  }

  if (year < 1 || year > 9999) {
    return false;
  }
  if (month < 1 || month > 12) {
    return false;
  }
  if (day < 1 || day > days_in_month(year, month)) {
    return false;
  }

  value = year * 10000 + month * 100 + day;
  return true;
}

string DateType::format(int value)
{
  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d", value / 10000, (value / 100) % 100, value % 100);
  return string(buffer);
}

int DateType::compare(const Value &left, const Value &right) const
{
  ASSERT(left.attr_type() == AttrType::DATES, "left type is not date");

  int left_value = left.get_int();
  if (right.attr_type() == AttrType::DATES) {
    int right_value = right.get_int();
    return common::compare_int(&left_value, &right_value);
  }
  if (right.attr_type() == AttrType::CHARS) {
    int right_value = 0;
    if (!parse(right.data(), right.length(), right_value)) {
      return INT32_MAX;
    }
    return common::compare_int(&left_value, &right_value);
  }
  return INT32_MAX;
}

int DateType::compare(const Column &left, const Column &right, int left_idx, int right_idx) const
{
  ASSERT(left.attr_type() == AttrType::DATES, "left type is not date");
  ASSERT(right.attr_type() == AttrType::DATES, "right type is not date");

  int left_value  = reinterpret_cast<const int *>(left.data())[left_idx];
  int right_value = reinterpret_cast<const int *>(right.data())[right_idx];
  return common::compare_int(&left_value, &right_value);
}

RC DateType::cast_to(const Value &val, AttrType type, Value &result) const
{
  if (type == AttrType::DATES) {
    if (val.attr_type() == AttrType::CHARS) {
      int date_value = 0;
      if (!parse(val.data(), val.length(), date_value)) {
        return RC::SCHEMA_FIELD_TYPE_MISMATCH;
      }
      result.set_date(date_value);
    } else {
      result = val;
    }
    return RC::SUCCESS;
  }

  if (type == AttrType::CHARS) {
    result.set_string(format(val.get_int()).c_str());
    return RC::SUCCESS;
  }

  if (type == AttrType::INTS) {
    result.set_int(val.get_int());
    return RC::SUCCESS;
  }

  return RC::SCHEMA_FIELD_TYPE_MISMATCH;
}

RC DateType::to_string(const Value &val, string &result) const
{
  if (val.attr_type() != AttrType::DATES) {
    return RC::SCHEMA_FIELD_TYPE_MISMATCH;
  }
  result = format(val.get_int());
  return RC::SUCCESS;
}

RC DateType::set_value_from_str(Value &val, const string &data) const
{
  int value = 0;
  if (!parse(data.c_str(), static_cast<int>(data.size()), value)) {
    return RC::SCHEMA_FIELD_TYPE_MISMATCH;
  }
  val.set_date(value);
  return RC::SUCCESS;
}

int DateType::cast_cost(AttrType type)
{
  if (type == AttrType::DATES) {
    return 0;
  }
  return INT32_MAX;
}
