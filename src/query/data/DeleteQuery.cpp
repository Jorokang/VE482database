#include "DeleteQuery.h"
#include "../../db/Database.h"
#include "../../db/Table.h"
#include "../QueryResult.h"
constexpr const char *DeleteQuery::qname;

QueryResult::Ptr DeleteQuery::execute() {
  using namespace std;

  Database &db = Database::getInstance();
  Table::SizeType deletedCount = 0;

  try {
    auto &table = db[this->targetTable];

    // cout << endl;
    // cout << "Debug: Table before delete: " << endl;
    // table.printData();
    auto result = initCondition(table);
    if (result.second) {
      for (auto it = table.begin(); it != table.end();) {
        if (this->evalCondition(*it)) {
          // it = table.erase(it);
          table.deleteByIndex(it->key());
          ++deletedCount;
        } else {
          ++it;
        }
      }
    }
    // cout << endl;
    // cout << "Debug: Table after delete: " << endl;
    // table.printData();

    return make_unique<RecordCountResult>(deletedCount);
  } catch (const TableNameNotFound &e) {
    return make_unique<ErrorMsgResult>(qname, this->targetTable,
                                       "No such table."s);
  } catch (const IllFormedQueryCondition &e) {
    return make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
  } catch (const invalid_argument &e) {
    return make_unique<ErrorMsgResult>(qname, this->targetTable,
                                       "Unknown error '?'"_f % e.what());
  } catch (const exception &e) {
    return make_unique<ErrorMsgResult>(qname, this->targetTable,
                                       "Unknown error '?'."_f % e.what());
  }
}
std::string DeleteQuery::toString() {
  return "QUERY = DELETE " + this->targetTable;
}
