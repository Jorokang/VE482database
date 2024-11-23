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
    auto result = initCondition(table);
    vector<Table::Iterator> toBeInserted;
    if (result.second) {
      for (auto it = table.begin(); it != table.end(); ++it) {
        if (this->evalCondition(*it)) {
          toBeInserted.push_back(it);
          // cout << "Debug: trying to duplicate key " << it->key() << endl;
          // table.duplicateKey(it, counter);
          // cout << "Debug: duplicated key" << it->key() << endl;
          // Table::Keytype key = it->key;
          counter++;
        }
      }
    }
    for (auto it = toBeInserted.begin(); it != toBeInserted.end(); ++it) {
      table.duplicateKey(*it, counter);
    }
    // if (toBeInserted.size() > 0) {
    //   table.duplicateKey(toBeInserted, counter);
    // }
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