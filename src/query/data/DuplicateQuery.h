#ifndef DUPLICATEQUERY_H
#define DUPLICATEQUERY_H

#include <string>

#include "../Query.h"
#include "../QueryResult.h"

class DuplicateQuery : public ComplexQuery {
  static constexpr const char *qname = "DUPLICATE";

public:
  using ComplexQuery::ComplexQuery;

  QueryResult::Ptr execute() override;

  std::string toString() override;
};

#endif // DUPLICATEQUERY_H
