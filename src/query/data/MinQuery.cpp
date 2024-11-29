#include <limits>
#include <memory>
#include <vector>

#include "../../db/Database.h"
#include "../../db/Table.h"
#include "../../utils/uexception.h"
#include "../QueryResult.h"
#include "MinQuery.h"

// constexpr const char *MinQuery::qname;

QueryResult::Ptr MinQuery::execute() {

  if (this->operands.empty()) {
    return std::make_unique<ErrorMsgResult>(
        qname, this->targetTable.c_str(),
        "Invalid number of operands (? operands)."_f % operands.size());
  }

  try {
    auto &db = Database::getInstance();
    auto &table = db[this->targetTable];
    auto condition = initCondition(table);
    bool found = false;

    std::vector<int> minValues(this->operands.size(), INT32_MAX);
    if (condition.second) {
      for (auto row = table.begin(); row != table.end(); ++row) {
        if (this->evalCondition(*row)) {
          found = true;
          for (size_t i = 0; i < this->operands.size(); ++i) {
            auto value = (*row)[this->operands[i]];
            if (value < minValues[i]) {
              minValues[i] = value;
            }
          }
        }
      }
    }

    if (found) {
      return std::make_unique<SuccessMsgResult>(minValues, true);
    } else {
      return std::make_unique<NullQueryResult>();
    }

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

std::string MinQuery::toString() {
  return "QUERY = MIN " + this->targetTable + "\"";
}
