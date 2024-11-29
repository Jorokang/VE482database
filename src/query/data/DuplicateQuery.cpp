#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

#include "../../db/Database.h"
#include "DuplicateQuery.h"

// constexpr const char *DuplicateQuery::qname;

QueryResult::Ptr DuplicateQuery::execute() {
  using std::exception;
  using std::invalid_argument;
  using std::make_unique;
  using std::vector;

  Database &db = Database::getInstance();
  Table::SizeType counter = 0;

  try {
    Table &table = db[this->targetTable];
    auto result = initCondition(table);
    vector<Table::KeyType> toBeInserted;
    if (result.second) {
      for (auto it = table.begin(); it != table.end(); ++it) {
        if (this->evalCondition(*it) && table.checkNotDuplicate(it->key())) {
          toBeInserted.push_back(it->key());
          counter++;
        }
      }
    }
    table.duplicateKey(toBeInserted);
    return make_unique<RecordCountResult>(counter);
  } catch (const TableNameNotFound &e) {
    return make_unique<ErrorMsgResult>(qname, this->targetTable,
                                       "No such table.");
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