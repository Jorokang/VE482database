#ifndef COUNTQUERY_H
#define COUNTQUERY_H

#include "../Query.h"
#include <string>

class CountQuery : public ComplexQuery {
  static constexpr const char *qname = "COUNT";

public:
  using ComplexQuery::ComplexQuery;

  QueryResult::Ptr execute() override;

  std::string toString() override;
};

#endif // COUNTQUERY_H