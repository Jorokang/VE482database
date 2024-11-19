#ifndef MIN_QUERY_H
#define MIN_QUERY_H

#include "../Query.h"
#include "../QueryResult.h"
#include <string>

class MinQuery : public ComplexQuery {
    static constexpr const char *qname = "MIN";
public:
    using ComplexQuery::ComplexQuery;
    QueryResult::Ptr execute() override;
    std::string toString() override;
};

#endif // MIN_QUERY_H
