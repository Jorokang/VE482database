#ifndef SUMQUERY_H
#define SUMQUERY_H

#include <string>
#include <vector>

#include "../Query.h"
#include "../QueryResult.h"

class SumQuery : public ComplexQuery {
  static constexpr const char *qname = "SUM";

public:
  using ComplexQuery::ComplexQuery;
  QueryResult::Ptr execute() override;
  // std::string toString() override;
};

#endif // SUMQUERY_H
