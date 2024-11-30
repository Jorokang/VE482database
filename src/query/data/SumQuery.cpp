#include <iostream>
#include <memory>
#include <string>

#include "../../db/Database.h"
#include "../../db/Table.h"
#include "../../utils/uexception.h"
#include "../QueryResult.h"
#include "SumQuery.h"

/**********************************************/
/* Define Global Variables */
// constexpr const char *SumQuery::qname;
/**********************************************/

std::string SumQuery::toString() {
  return "QUERY = SUM " + this->getTargetTable() + "\"";
}

QueryResult::Ptr SumQuery::execute() {
  if (this->getOperands().empty()) {
    return std::make_unique<ErrorMsgResult>(
        qname, this->getTargetTable().c_str(),
        "Invalid number of this->getOperands() (? this->getOperands())."_f %
            getOperands().size());
  }

  try {
    auto &db = Database::getInstance();
    auto &table = db[this->getTargetTable()];
    auto condition = initCondition(table);

    std::vector<int> sumValues(this->getOperands().size(), 0);
    if (condition.second) {
      for (auto row = table.begin(); row != table.end(); ++row) {
        if (this->evalCondition(*row)) {
          for (size_t i = 0; i < this->getOperands().size(); ++i) {
            auto value = (*row)[this->getOperands()[i]];
            sumValues[i] += value;
          }
        }
      }
    }

    return std::make_unique<SuccessMsgResult>(sumValues, true);

  } catch (const TableNameNotFound &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->getTargetTable(),
                                            "No such table.");
  } catch (const TableFieldNotFound &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->getTargetTable(),
                                            e.what());
  } catch (const std::exception &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->getTargetTable(),
                                            "Unknown error '?'"_f % e.what());
  }
}
