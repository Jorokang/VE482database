#ifndef SELECTQUERY_H
#define SELECTQUERY_H

#include <string>

#include "../Query.h"
#include "../QueryResult.h"

class SelectQuery : public ComplexQuery {
  static constexpr const char *qname = "SELECT";

public:
  using ComplexQuery::ComplexQuery;

  QueryResult::Ptr execute() override;

  // std::string toString() override;
};

#endif // SELECTQUERY_H
