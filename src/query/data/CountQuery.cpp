#include "CountQuery.h"
#include "../../db/Database.h"
#include <memory>

constexpr const char *CountQuery::qname;

QueryResult::Ptr CountQuery::execute() {
  using namespace std;

  Database &db = Database::getInstance();

  try {
    Table &table = db[this->targetTable];
    auto result = initCondition(table);
    size_t count = 0;
    if (result.second) {
      for (auto it = table.begin(); it != table.end(); ++it) {
        if (this->evalCondition(*it)) {
          ++count;
        }
      }
    }
    return make_unique<RecordCountResult>(count);
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

std::string CountQuery::toString() {
  return "QUERY = COUNT " + this->targetTable + "\"";
}