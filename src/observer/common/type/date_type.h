/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#pragma once

#include "common/type/data_type.h"

/**
 * @brief 日期类型
 * @ingroup DataType
 * @details 日期以 YYYYMMDD 的 4 字节整数存储，例如 2022-10-10 存储为 20221010。
 *          这种表示不依赖 time_t，因此不受 2038 年限制，也支持 1970 年以前的日期。
 */
class DateType : public DataType
{
public:
  DateType() : DataType(AttrType::DATES) {}
  virtual ~DateType() = default;

  int compare(const Value &left, const Value &right) const override;
  int compare(const Column &left, const Column &right, int left_idx, int right_idx) const override;

  RC cast_to(const Value &val, AttrType type, Value &result) const override;
  RC to_string(const Value &val, string &result) const override;
  RC set_value_from_str(Value &val, const string &data) const override;

  int cast_cost(AttrType type) override;

  /**
   * @brief 解析 YYYY-MM-DD 格式的日期
   * @param str 输入字符串
   * @param len 输入长度
   * @param value 输出 YYYYMMDD
   * @return 合法返回 true，非法返回 false
   */
  static bool parse(const char *str, int len, int &value);

  /// 将 YYYYMMDD 格式化为 YYYY-MM-DD
  static string format(int value);

private:
  static bool is_leap_year(int year);
  static int  days_in_month(int year, int month);
};
