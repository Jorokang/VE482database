//
// Created by liu on 18-10-25.
//

#ifndef PROJECT_UPDATEQUERY_H
#define PROJECT_UPDATEQUERY_H

#include <string>

#include "../Query.h"

class UpdateQuery : public ComplexQuery {
  static constexpr const char *qname = "UPDATE";
  Table::ValueType fieldValue; // = (this->getOperands()[0]=="KEY")? 0
                               // :std::stoi(this->getOperands()[1]);
  Table::FieldIndex fieldId;
  Table::KeyType keyValue;

public:
  using ComplexQuery::ComplexQuery;

  QueryResult::Ptr execute() override;

  std::string toString() override;

  Table::ValueType getFieldValue() { return fieldValue; }

  Table::FieldIndex getFieldId() { return fieldId; }

  Table::KeyType getKeyValue() { return keyValue; }
};

#endif // PROJECT_UPDATEQUERY_H
