#ifndef SWAPQUERY_H
#define SWAPQUERY_H

#include <string>

#include "../Query.h"
#include "../QueryResult.h"

class SwapQuery : public ComplexQuery {
  static constexpr const char *qname = "SWAP";
  Table::FieldIndex fieldId1;
  Table::FieldIndex fieldId2;

public:
  using ComplexQuery::ComplexQuery;

  QueryResult::Ptr execute() override;

  std::string toString() override;
};

#endif