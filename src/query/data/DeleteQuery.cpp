#include "DeleteQuery.h"
#include "../../db/Database.h"
#include "../QueryResult.h"
#include "../../db/Table.h"
constexpr const char *DeleteQuery::qname;

QueryResult::Ptr DeleteQuery::execute() {
  using namespace std;


  if (operands.empty()) {
    return make_unique<ErrorMsgResult>(
        qname, "", "Missing target table."s);
  }

  Database &db = Database::getInstance();
  Table::SizeType deletedCount = 0;

  try {


    auto &table = db[targetTable];

    auto result = initCondition(table);
    if (result.second) {
      for (auto it = table.begin(); it != table.end(); ) {
        if (evalCondition(*it)) {
          it = table.erase(it); // Update 'it' with the iterator returned by 'erase'
          ++deletedCount;
        } else {
          ++it;
        }
      }
    }


    return make_unique<RecordCountResult>(deletedCount);
  } catch (const TableNameNotFound &e) {

    return make_unique<ErrorMsgResult>(qname, targetTable, "No such table."s);
  } catch (const IllFormedQueryCondition &e) {

    return make_unique<ErrorMsgResult>(qname, targetTable, e.what());
  } catch (const invalid_argument &e) {

    return make_unique<ErrorMsgResult>(qname, targetTable, "Unknown error '?'"_f % e.what());
  } catch (const exception &e) {

    return make_unique<ErrorMsgResult>(qname, targetTable, "Unknown error '?'."_f % e.what());
  }
}


std::string DeleteQuery::toString() {
  return "QUERY = DELETE " + targetTable;
}
