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
// Created by Wangyunlai on 2023/6/13.
//

#include "sql/stmt/drop_table_stmt.h"

#include "common/log/log.h"

RC DropTableStmt::create(Db *db, const DropTableSqlNode &drop_table, Stmt *&stmt)
{
  stmt = nullptr;

  if (nullptr == db || drop_table.relation_name.empty()) {
    LOG_WARN("invalid argument. db=%p, table_name=%s", db, drop_table.relation_name.c_str());
    return RC::INVALID_ARGUMENT;
  }

  // 表是否存在由 Db::drop_table 检查，这里只负责把语句信息带到执行器。
  stmt = new DropTableStmt(drop_table.relation_name);
  return RC::SUCCESS;
}
