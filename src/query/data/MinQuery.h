#ifndef MIN_QUERY_H
#define MIN_QUERY_H

#include <string>

#include "../Query.h"
#include "../QueryResult.h"

class MinQuery : public ComplexQuery {
  static constexpr const char *qname = "MIN";

public:
  using ComplexQuery::ComplexQuery;
  QueryResult::Ptr execute() override;
  // std::string toString() override;
};

#endif // MIN_QUERY_H
