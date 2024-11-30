//
// Created by liu on 18-10-25.
//

#include <memory>
#include <stdexcept>

#include "../../db/Database.h"
#include "UpdateQuery.h"

// constexpr const char *UpdateQuery::qname;

QueryResult::Ptr UpdateQuery::execute() {
  // using namespace std;
  using std::exception;
  using std::invalid_argument;
  using std::make_unique;

  if (this->getOperands().size() != 2)
    return make_unique<ErrorMsgResult>(
        qname, this->getTargetTable().c_str(),
        "Invalid number of this->getOperands() (? this->getOperands())."_f %
            this->getOperands().size());
  Database &db = Database::getInstance();
  Table::SizeType counter = 0;
  try {
    auto &table = db[this->getTargetTable()];
    if (this->getOperands()[0] == "KEY") {
      this->keyValue = this->getOperands()[1];
    } else {
      this->fieldId = table.getFieldIndex(this->getOperands()[0]);
      this->fieldValue =
          (Table::ValueType)strtol(this->getOperands()[1].c_str(), nullptr, 10);
    }
    auto result = initCondition(table);
    if (result.second) {
      for (auto it = table.begin(); it != table.end(); ++it) {
        if (this->evalCondition(*it)) {
          if (this->keyValue.empty()) {
            (*it)[this->fieldId] = this->fieldValue;
          } else {
            it->setKey(this->keyValue);
          }
          ++counter;
        }
      }
    }
    return make_unique<RecordCountResult>(counter);
  } catch (const TableNameNotFound &e) {
    return make_unique<ErrorMsgResult>(qname, this->getTargetTable(),
                                       "No such table.");
  } catch (const IllFormedQueryCondition &e) {
    return make_unique<ErrorMsgResult>(qname, this->getTargetTable(), e.what());
  } catch (const invalid_argument &e) {
    // Cannot convert operand to string
    return make_unique<ErrorMsgResult>(qname, this->getTargetTable(),
                                       "Unknown error '?'"_f % e.what());
  } catch (const exception &e) {
    return make_unique<ErrorMsgResult>(qname, this->getTargetTable(),
                                       "Unkonwn error '?'."_f % e.what());
  }
}

std::string UpdateQuery::toString() {
  return "QUERY = UPDATE " + this->getTargetTable() + "\"";
}
