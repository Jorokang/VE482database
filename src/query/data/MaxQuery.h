#ifndef MAX_QUERY_H
#define MAX_QUERY_H

#include "../Query.h"
#include "../QueryResult.h"
#include <string>

class MaxQuery : public ComplexQuery {
public:
    static constexpr const char *qname = "MAX";
    using ComplexQuery::ComplexQuery;
    QueryResult::Ptr execute() override;
    std::string toString() override;
};

#endif // MAX_QUERY_H
