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
// Created by Wangyunlai on 2022/6/6.
//

#include "sql/stmt/select_stmt.h"
#include "common/lang/string.h"
#include "common/log/log.h"
#include "sql/stmt/filter_stmt.h"
#include "storage/db/db.h"
#include "storage/table/table.h"
#include "sql/parser/expression_binder.h"

using namespace std;
using namespace common;

SelectStmt::~SelectStmt()
{
  if (nullptr != filter_stmt_) {
    delete filter_stmt_;
    filter_stmt_ = nullptr;
  }

  for (FilterStmt *filter_stmt : join_filter_stmts_) {
    delete filter_stmt;
  }
  join_filter_stmts_.clear();
}

RC SelectStmt::create(Db *db, SelectSqlNode &select_sql, Stmt *&stmt)
{
  if (nullptr == db) {
    LOG_WARN("invalid argument. db is null");
    return RC::INVALID_ARGUMENT;
  }

  BinderContext binder_context;

  // collect tables in `from` statement
  vector<Table *>                tables;
  unordered_map<string, Table *> table_map;
  for (size_t i = 0; i < select_sql.relations.size(); i++) {
    const RelationSqlNode &relation    = select_sql.relations[i];
    const char            *table_name = relation.relation_name.c_str();
    if (common::is_blank(table_name)) {
      LOG_WARN("invalid argument. relation name is blank. index=%d", i);
      return RC::INVALID_ARGUMENT;
    }

    Table *table = db->find_table(table_name);
    if (nullptr == table) {
      LOG_WARN("no such table. db=%s, table_name=%s", db->name(), table_name);
      return RC::SCHEMA_TABLE_NOT_EXIST;
    }

    binder_context.add_table(table);
    tables.push_back(table);
    table_map.insert({relation.relation_name, table});
  }

  // 为每个 INNER JOIN 创建 ON 条件对应的 FilterStmt。
  // 第 i 个 FilterStmt 只能访问 tables[0..i]，这样可以在 join 时尽早过滤数据。
  vector<FilterStmt *>           join_filter_stmts;
  unordered_map<string, Table *> joined_table_map;
  join_filter_stmts.reserve(tables.size());
  for (size_t i = 0; i < tables.size(); i++) {
    const RelationSqlNode &relation = select_sql.relations[i];
    joined_table_map.insert({relation.relation_name, tables[i]});

    FilterStmt *join_filter_stmt = nullptr;
    if (!relation.conditions.empty()) {
      RC rc = FilterStmt::create(db,
          tables.front(),
          &joined_table_map,
          relation.conditions.data(),
          static_cast<int>(relation.conditions.size()),
          join_filter_stmt);
      if (OB_FAIL(rc)) {
        LOG_WARN("failed to create join filter statement. table=%s, rc=%s", relation.relation_name.c_str(), strrc(rc));
        for (FilterStmt *filter : join_filter_stmts) {
          delete filter;
        }
        return rc;
      }
    }
    join_filter_stmts.push_back(join_filter_stmt);
  }

  auto cleanup_join_filters = [&join_filter_stmts]() {
    for (FilterStmt *filter : join_filter_stmts) {
      delete filter;
    }
    join_filter_stmts.clear();
  };

  // collect query fields in `select` statement
  vector<unique_ptr<Expression>> bound_expressions;
  ExpressionBinder expression_binder(binder_context);
  
  for (unique_ptr<Expression> &expression : select_sql.expressions) {
    RC rc = expression_binder.bind_expression(expression, bound_expressions);
    if (OB_FAIL(rc)) {
      LOG_INFO("bind expression failed. rc=%s", strrc(rc));
      cleanup_join_filters();
      return rc;
    }
  }

  vector<unique_ptr<Expression>> group_by_expressions;
  for (unique_ptr<Expression> &expression : select_sql.group_by) {
    RC rc = expression_binder.bind_expression(expression, group_by_expressions);
    if (OB_FAIL(rc)) {
      LOG_INFO("bind expression failed. rc=%s", strrc(rc));
      cleanup_join_filters();
      return rc;
    }
  }

  Table *default_table = tables.empty() ? nullptr : tables.front();

  // create filter statement in `where` statement
  FilterStmt *filter_stmt = nullptr;
  RC          rc          = FilterStmt::create(db,
      default_table,
      &table_map,
      select_sql.conditions.data(),
      static_cast<int>(select_sql.conditions.size()),
      filter_stmt);
  if (rc != RC::SUCCESS) {
    LOG_WARN("cannot construct filter stmt");
    cleanup_join_filters();
    return rc;
  }

  // everything alright
  SelectStmt *select_stmt = new SelectStmt();

  select_stmt->tables_.swap(tables);
  select_stmt->query_expressions_.swap(bound_expressions);
  select_stmt->filter_stmt_ = filter_stmt;
  select_stmt->join_filter_stmts_.swap(join_filter_stmts);
  select_stmt->group_by_.swap(group_by_expressions);
  stmt                      = select_stmt;
  return RC::SUCCESS;
}
