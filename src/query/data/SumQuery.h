#ifndef SUMQUERY_H
#define SUMQUERY_H

#include "../Query.h"
#include "../QueryResult.h"
#include <string>
#include <vector>

class SumQuery : public ComplexQuery {
  static constexpr const char *qname = "SUM";

public:
  using ComplexQuery::ComplexQuery;
  QueryResult::Ptr execute() override;
  std::string toString() override;
};

#endif // SUMQUERY_H
