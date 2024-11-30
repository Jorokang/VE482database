#include <memory>
#include <string>

#include "../../db/Database.h"
#include "../../db/Table.h"
#include "../QueryResult.h"
#include "DeleteQuery.h"

// constexpr const char *DeleteQuery::qname;

QueryResult::Ptr DeleteQuery::execute() {
  // using namespace std;
  using std::exception;
  using std::invalid_argument;
  using std::make_unique;
  Database &db = Database::getInstance();
  Table::SizeType deletedCount = 0;

  try {
    auto &table = db[this->getTargetTable()];

    auto result = initCondition(table);
    if (result.second) {
      for (auto it = table.begin(); it != table.end();) {
        if (this->evalCondition(*it)) {
          table.deleteByIndex(it->key());
          ++deletedCount;
        } else {
          ++it;
        }
      }
    }
    return make_unique<RecordCountResult>(deletedCount);
  } catch (const TableNameNotFound &e) {
    return make_unique<ErrorMsgResult>(qname, this->getTargetTable(),
                                       "No such table.");
  } catch (const IllFormedQueryCondition &e) {
    return make_unique<ErrorMsgResult>(qname, this->getTargetTable(), e.what());
  } catch (const invalid_argument &e) {
    return make_unique<ErrorMsgResult>(qname, this->getTargetTable(),
                                       "Unknown error '?'"_f % e.what());
  } catch (const exception &e) {
    return make_unique<ErrorMsgResult>(qname, this->getTargetTable(),
                                       "Unknown error '?'."_f % e.what());
  }
}
std::string DeleteQuery::toString() {
  return "QUERY = DELETE " + this->getTargetTable();
}
