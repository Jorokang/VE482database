#ifndef SUBQUERY_H
#define SUBQUERY_H

#include "../Query.h"

class SubQuery : public ComplexQuery {
  static constexpr const char *qname = "SUB";
  std::vector<Table::FieldIndex> fieldIds;
  Table::FieldIndex srcFieldId;
  Table::FieldIndex destFieldId;

public:
  using ComplexQuery::ComplexQuery;

  QueryResult::Ptr execute() override;

  std::string toString() override;
};

#endif