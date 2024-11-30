//
// Created by liu on 18-10-25.
//

#include <iostream>
#include <memory>
#include <string>

#include "../../db/Database.h"
#include "PrintTableQuery.h"

// constexpr const char *PrintTableQuery::qname;

QueryResult::Ptr PrintTableQuery::execute() {
  // using namespace std;
  using std::cout;
  using std::endl;
  using std::make_unique;

  Database &db = Database::getInstance();
  try {
    auto &table = db[this->getTargetTable()];
    cout << "================\n";
    cout << "TABLE = ";
    cout << table;
    cout << "================\n" << endl;
    return make_unique<SuccessMsgResult>(qname, this->getTargetTable());
  } catch (const TableNameNotFound &e) {
    return make_unique<ErrorMsgResult>(qname, this->getTargetTable(),
                                       "No such table.");
  }
}

std::string PrintTableQuery::toString() {
  return "QUERY = SHOWTABLE, Table = \"" + this->getTargetTable() + "\"";
}