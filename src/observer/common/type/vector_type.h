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

#include "common/lang/vector.h"
#include "common/type/data_type.h"

/**
 * @brief 向量类型
 * @ingroup DataType
 * @details 每个向量元素是 4 字节 float，字段长度 = 维度 * sizeof(float)。
 */
class VectorType : public DataType
{
public:
  VectorType() : DataType(AttrType::VECTORS) {}
  virtual ~VectorType() = default;

  int compare(const Value &left, const Value &right) const override;

  RC cast_to(const Value &val, AttrType type, Value &result) const override;
  RC to_string(const Value &val, string &result) const override;
  RC set_value_from_str(Value &val, const string &data) const override;

  int cast_cost(AttrType type) override;

  /// 解析 "[1, 2, 3]" 形式的字符串
  static RC parse(const string &text, vector<float> &values);

  /// 格式化为 "[1.00000e+00,2.00000e+00]" 形式
  static string format(const vector<float> &values);
};
