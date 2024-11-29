#ifndef COPYTABLEQUERY_H
#define COPYTABLEQUERY_H

#include "../Query.h"

class CopyTableQuery : public Query {
  static constexpr const char *qname = "COPYTABLE";
  const std::string newtable;

public:
  explicit CopyTableQuery(std::string table, std::string newtable)
      : Query(std::move(table)), newtable(std::move(newtable)) {}

  QueryResult::Ptr execute() override;
  std::string toString() override;
};

#endif // COPYTABLEQUERY_H