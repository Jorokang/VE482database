#ifndef SELECTQUERY_H
#define SELECTQUERY_H

#include "../Query.h"

class SelectQuery : public ComplexQuery {
    static constexpr const char *qname = "SELECT";

public:
    using ComplexQuery::ComplexQuery;

    QueryResult::Ptr execute() override;

    std::string toString() override;
};


std::string join(const std::vector<std::string>& vec, const std::string& delimiter);
#endif // SELECTQUERY_H
