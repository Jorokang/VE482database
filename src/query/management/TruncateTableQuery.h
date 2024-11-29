#ifndef TRUNCATETABLEQUERY_H
#define TRUNCATETABLEQUERY_H

#include "../Query.h"

class TruncateTableQuery : public Query {
  static constexpr const char *qname = "TRUNCATETABLE";

public:
  using Query::Query;
  QueryResult::Ptr execute() override;
  std::string toString() override;
};

#endif // TRUNCATETABLEQUERY_H