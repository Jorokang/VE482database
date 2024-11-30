//
// Created by liu on 18-10-25.
//

#include <memory>
#include <string>

#include "../../db/Database.h"
#include "DropTableQuery.h"

// constexpr const char *DropTableQuery::qname;

QueryResult::Ptr DropTableQuery::execute() {
  using std::exception;
  using std::make_unique;

  Database &db = Database::getInstance();
  try {
    db.dropTable(this->getTargetTable());
    return make_unique<SuccessMsgResult>(qname);
  } catch (const TableNameNotFound &e) {
    return make_unique<ErrorMsgResult>(qname, this->getTargetTable(), "No such table.");
  } catch (const exception &e) {
    return make_unique<ErrorMsgResult>(qname, e.what());
  }
}

std::string DropTableQuery::toString() {
  return "QUERY = DROP, Table = \"" + this->getTargetTable() + "\"";
}