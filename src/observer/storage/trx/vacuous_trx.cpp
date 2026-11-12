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
// Created by Wangyunlai on 2023/4/24.
//

#include "storage/trx/vacuous_trx.h"

#include "common/log/log.h"
#include "storage/table/table.h"

RC VacuousTrxKit::init() { return RC::SUCCESS; }

const vector<FieldMeta> *VacuousTrxKit::trx_fields() const { return nullptr; }

Trx *VacuousTrxKit::create_trx(LogHandler &) { return new VacuousTrx; }

Trx *VacuousTrxKit::create_trx(LogHandler &, int32_t /*trx_id*/) { return nullptr; }

void VacuousTrxKit::destroy_trx(Trx *trx) { delete trx; }

void VacuousTrxKit::all_trxes(vector<Trx *> &trxes) { return; }

LogReplayer *VacuousTrxKit::create_log_replayer(Db &, LogHandler &) { return new VacuousTrxLogReplayer; }

////////////////////////////////////////////////////////////////////////////////

RC VacuousTrx::insert_record(Table *table, Record &record) { return table->insert_record(record); }

RC VacuousTrx::delete_record(Table *table, Record &record) { return table->delete_record(record); }

RC VacuousTrx::update_record(Table *table, Record &old_record, Record &new_record)
{
  // VacuousTrx 没有真正的事务能力，这里直接复用 insert/delete：
  // 先删除旧记录（同时清理旧索引项），再插入新记录（同时写入新索引项）。
  RC rc = table->delete_record(old_record);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to delete old record while updating. rc=%s", strrc(rc));
    return rc;
  }

  rc = table->insert_record(new_record);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to insert new record while updating. rc=%s", strrc(rc));
    return rc;
  }
  return RC::SUCCESS;
}

RC VacuousTrx::visit_record(Table *table, Record &record, ReadWriteMode) { return RC::SUCCESS; }

RC VacuousTrx::start_if_need() { return RC::SUCCESS; }

RC VacuousTrx::commit() { return RC::SUCCESS; }

RC VacuousTrx::rollback() { return RC::SUCCESS; }

RC VacuousTrx::redo(Db *, const LogEntry &) { return RC::SUCCESS; }
