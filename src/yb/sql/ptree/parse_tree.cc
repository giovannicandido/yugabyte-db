//--------------------------------------------------------------------------------------------------
// Copyright (c) YugaByte, Inc.
//
// Parse Tree Declaration.
//--------------------------------------------------------------------------------------------------

#include "yb/sql/ptree/parse_tree.h"
#include "yb/sql/ptree/tree_node.h"
#include "yb/sql/ptree/sem_context.h"

namespace yb {
namespace sql {

//--------------------------------------------------------------------------------------------------
// Parse Tree
//--------------------------------------------------------------------------------------------------

namespace {

std::shared_ptr<BufferAllocator> MakeAllocator(std::shared_ptr<MemTracker> mem_tracker) {
  if (mem_tracker) {
    return std::make_shared<MemoryTrackingBufferAllocator>(HeapBufferAllocator::Get(),
                                                           std::move(mem_tracker));
  } else {
    return nullptr;
  }
}

} // namespace

ParseTree::ParseTree(std::shared_ptr<MemTracker> mem_tracker)
    : buffer_allocator_(MakeAllocator(std::move(mem_tracker))),
      ptree_mem_(buffer_allocator_ ? buffer_allocator_.get() : HeapBufferAllocator::Get()),
      psem_mem_(buffer_allocator_ ? buffer_allocator_.get() : HeapBufferAllocator::Get()) {
}

ParseTree::~ParseTree() {
  // Make sure we delete the tree first before deleting the memory pools.
  root_ = nullptr;
}

CHECKED_STATUS ParseTree::Analyze(SemContext *sem_context) {
  if (root_ == nullptr) {
    LOG(INFO) << "Parse tree is NULL";
    return Status::OK();
  }

  // Reset and release previous semantic analysis results and free the associated memory.
  root_->Reset();
  psem_mem_.Reset();

  return root_->Analyze(sem_context);
}

void ParseTree::AddAnalyzedTable(const client::YBTableName& table_name) {
  analyzed_tables_.insert(table_name);
}

void ParseTree::ClearAnalyzedTableCache(SqlEnv* sql_env) const {
  for (const auto& table_name : analyzed_tables_) {
    sql_env->RemoveCachedTableDesc(table_name);
  }
}

void ParseTree::AddAnalyzedUDType(const std::string& keyspace_name, const std::string& type_name) {
  analyzed_types_.insert(std::make_pair(keyspace_name, type_name));
}

void ParseTree::ClearAnalyzedUDTypeCache(SqlEnv *sql_env) const {
  for (const auto& type_name : analyzed_types_) {
    sql_env->RemoveCachedUDType(type_name.first, type_name.second);
  }
}

}  // namespace sql
}  // namespace yb
