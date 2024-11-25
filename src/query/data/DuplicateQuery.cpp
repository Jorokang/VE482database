#include "DuplicateQuery.h"
#include "../../db/Database.h"
#include <iostream>
#include <vector>

constexpr const char *DuplicateQuery::qname;

QueryResult::Ptr DuplicateQuery::execute() {
  using namespace std;
  Database &db = Database::getInstance();
  Table::SizeType counter = 0;

  try {
    Table &table = db[this->targetTable];
    // cout << endl;
    // cout << "Debug: Table before duplicate: " << endl;
    // table.printData();
    auto result = initCondition(table);
    vector<Table::Iterator> toBeInserted;
    if (result.second) {
      for (auto it = table.begin(); it != table.end(); ++it) {
        if (this->evalCondition(*it)) {
          toBeInserted.push_back(it);
          counter++;
        }
      }
    }
    // for (auto it = toBeInserted.begin(); it != toBeInserted.end(); ++it) {
    // table.duplicateKey(*it, counter);
    // }
    table.duplicateKey(toBeInserted, counter);
    // cout << "Debug: Table after duplicate: " << endl;
    // table.printData();
    return make_unique<RecordCountResult>(counter);
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

std::string DuplicateQuery::toString() {
  return "QUERY = DUPLICATE " + this->targetTable + "\"";
}