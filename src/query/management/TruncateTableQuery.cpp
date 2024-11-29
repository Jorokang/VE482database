#include <limits>
#include <memory>
#include <string>

#include "../../db/Database.h"
#include "../../utils/uexception.h"
#include "TruncateTableQuery.h"

constexpr const char *TruncateTableQuery::qname;

QueryResult::Ptr TruncateTableQuery::execute() {
  try {
    Database &db = Database::getInstance();
    auto &table = db[this->targetTable];
    table.clear();
    return std::make_unique<NullQueryResult>();
  } catch (const TableNameNotFound &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable,
                                            "No such table.");
  } catch (const TableFieldNotFound &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
  } catch (const std::exception &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable,
                                            "Unknown error '?'"_f % e.what());
  }
}

std::string TruncateTableQuery::toString() {
  return "QUERY = TRUNCATE table \"" + this->targetTable + "\"";
}