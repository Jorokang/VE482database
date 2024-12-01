#ifndef SWAPQUERY_H
#define SWAPQUERY_H

#include <string>
#include <utility>
#include <vector>

#include "../Query.h"
#include "../QueryResult.h"

class SwapQuery : public ComplexQuery {
  static constexpr const char *qname = "SWAP";
  Table::FieldIndex fieldId1;
  Table::FieldIndex fieldId2;

public:
  using ComplexQuery::ComplexQuery;

  SwapQuery(std::string targetTable, std::vector<std::string> operands,
            std::vector<QueryCondition> condition)
      : ComplexQuery(std::move(targetTable), std::move(operands),
                     std::move(condition)),
        fieldId1(0), fieldId2(0) {}

  QueryResult::Ptr execute() override;

  std::string toString() override;

  Table::FieldIndex getFieldId1() { return fieldId1; }

  Table::FieldIndex getFieldId2() { return fieldId2; }
};

#endif