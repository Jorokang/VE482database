#ifndef ADDQUERY_H
#define ADDQUERY_H

#include <string>
#include <vector>

#include "../Query.h"

class AddQuery : public ComplexQuery {
  static constexpr const char *qname = "ADD";
  std::vector<Table::FieldIndex> fieldIds;
  Table::FieldIndex destFieldId;

public:
  using ComplexQuery::ComplexQuery;

  std::vector<Table::FieldIndex> getFieldIds() { return fieldIds; }

  Table::FieldIndex getDestFieldId() { return destFieldId; }

  QueryResult::Ptr execute() override;

  std::string toString() override;
};

#endif