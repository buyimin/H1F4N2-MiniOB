/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by WangYunlai on 2022/6/9.
//

#include "sql/operator/update_physical_operator.h"

#include <string.h>

#include "common/log/log.h"
#include "sql/expr/tuple.h"
#include "storage/field/field_meta.h"
#include "storage/record/record.h"
#include "storage/table/table.h"
#include "storage/trx/trx.h"

RC UpdatePhysicalOperator::open(Trx *trx)
{
  if (children_.empty()) {
    return RC::SUCCESS;
  }

  unique_ptr<PhysicalOperator> &child = children_[0];

  RC rc = child->open(trx);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to open child operator: %s", strrc(rc));
    return rc;
  }

  // 先收集所有满足条件的记录，再统一更新。
  // 如果更新的是索引列，事务层可能会删除旧记录并插入新记录，RID 会发生变化，
  // 边扫描边修改会导致扫描器状态不可预期。
  vector<Record> old_records;
  while (OB_SUCC(rc = child->next())) {
    Tuple *tuple = child->current_tuple();
    if (nullptr == tuple) {
      LOG_WARN("failed to get current record: %s", strrc(rc));
      child->close();
      return RC::INTERNAL;
    }

    RowTuple *row_tuple = static_cast<RowTuple *>(tuple);
    Record    old_record;
    rc = old_record.copy_data(row_tuple->record().data(), row_tuple->record().len());
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to copy record data: %s", strrc(rc));
      child->close();
      return rc;
    }
    old_record.set_rid(row_tuple->record().rid());
    old_records.emplace_back(std::move(old_record));
  }
  child->close();

  if (rc != RC::RECORD_EOF) {
    LOG_WARN("failed to scan records for update: %s", strrc(rc));
    return rc;
  }

  // 逐条复制旧记录、修改目标字段，然后交给事务完成更新。
  for (Record &old_record : old_records) {
    Record new_record;
    rc = new_record.copy_data(old_record.data(), old_record.len());
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to copy record data for update: %s", strrc(rc));
      return rc;
    }
    new_record.set_rid(old_record.rid());

    const int field_offset = field_meta_->offset();
    const int field_len    = field_meta_->len();

    // 先清空目标字段，避免字符串变短时残留旧数据。
    memset(new_record.data() + field_offset, 0, field_len);

    int copy_len = field_len;
    if (field_meta_->type() == AttrType::CHARS) {
      const int value_len = value_.length() + 1;  // include '\0'
      if (copy_len > value_len) {
        copy_len = value_len;
      }
    }
    memcpy(new_record.data() + field_offset, value_.data(), copy_len);

    rc = trx->update_record(table_, old_record, new_record);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to update record: %s", strrc(rc));
      return rc;
    }
  }

  return RC::SUCCESS;
}

RC UpdatePhysicalOperator::next()
{
  return RC::RECORD_EOF;
}

RC UpdatePhysicalOperator::close()
{
  return RC::SUCCESS;
}
