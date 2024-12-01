#ifndef SUBQUERY_H
#define SUBQUERY_H

#include <string>
#include <utility>
#include <vector>

#include "../Query.h"

class SubQuery : public ComplexQuery {
  static constexpr const char *qname = "SUB";
  std::vector<Table::FieldIndex> fieldIds;
  Table::FieldIndex srcFieldId;
  Table::FieldIndex destFieldId;

public:
  using ComplexQuery::ComplexQuery;

  SubQuery(std::string targetTable, std::vector<std::string> operands,
           std::vector<QueryCondition> condition)
      : ComplexQuery(std::move(targetTable), std::move(operands),
                     std::move(condition)),
        srcFieldId(0), destFieldId(0) {}

  QueryResult::Ptr execute() override;

  // std::string toString() override;

  std::vector<Table::FieldIndex> getFieldIds() { return fieldIds; }

  Table::FieldIndex getSrcFieldId() { return srcFieldId; }

  Table::FieldIndex getDestFieldId() { return destFieldId; }
};

#endif