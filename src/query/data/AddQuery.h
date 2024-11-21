#ifndef ADDQUERY_H
#define ADDQUERY_H

#include "../Query.h"

class AddQuery : public ComplexQuery {
    static constexpr const char *qname = "ADD";
    Table::ValueType fieldValue;
    std::vector<Table::FieldIndex> fieldIds;
    Table::FieldIndex destFieldId;
    
public:
    using ComplexQuery::ComplexQuery;
    
    QueryResult::Ptr execute() override;
    
    std::string toString() override;
};

#endif