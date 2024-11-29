#include "SumQuery.h"
#include "../../db/Database.h"
#include "../../db/Table.h"
#include "../../utils/uexception.h"
#include "../QueryResult.h"
#include <iostream>

/**********************************************/
/* Define Global Variables */
constexpr const char *SumQuery::qname;
/**********************************************/

std::string SumQuery::toString() {
  return "QUERY = SUM " + this->targetTable + "\"";
}

QueryResult::Ptr SumQuery::execute() {
  if (this->operands.empty()) {
    return std::make_unique<ErrorMsgResult>(
        qname, this->targetTable.c_str(),
        "Invalid number of operands (? operands)."_f % operands.size());
  }

  try {
    auto &db = Database::getInstance();
    auto &table = db[this->targetTable];
    auto condition = initCondition(table);

    std::vector<int> sumValues(this->operands.size(), 0);
    if (condition.second) {
      for (auto row = table.begin(); row != table.end(); ++row) {
        if (this->evalCondition(*row)) {
          for (size_t i = 0; i < this->operands.size(); ++i) {
            auto value = (*row)[this->operands[i]];
            sumValues[i] += value;
          }
        }
      }
    }

    return std::make_unique<SuccessMsgResult>(sumValues, true);

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
