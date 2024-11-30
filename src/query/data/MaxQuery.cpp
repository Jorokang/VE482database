#include <limits>
#include <memory>
#include <vector>

#include "../../db/Database.h"
#include "../../db/Table.h"
#include "../../utils/uexception.h"
#include "../QueryResult.h"
#include "MaxQuery.h"

/**********************************************/
/* Define Global Variables */
// constexpr const char *MaxQuery::qname;
/**********************************************/

std::string MaxQuery::toString() {
  return "QUERY = MAX " + this->getTargetTable() + "\"";
}

QueryResult::Ptr MaxQuery::execute() {

  if (this->getOperands().empty()) {
    return std::make_unique<ErrorMsgResult>(
        qname, this->getTargetTable().c_str(),
        "Invalid number of this->getOperands() (? this->getOperands())."_f %
            this->getOperands().size());
  }

  try {
    auto &db = Database::getInstance();
    auto &table = db[this->getTargetTable()];
    auto condition = initCondition(table);
    bool found = false;

    std::vector<int> maxValues(this->getOperands().size(), INT32_MIN);
    if (condition.second) {
      for (auto row = table.begin(); row != table.end(); ++row) {
        if (this->evalCondition(*row)) {
          found = true;
          for (size_t i = 0; i < this->getOperands().size(); ++i) {
            auto value = (*row)[this->getOperands()[i]];
            if (value > maxValues[i]) {
              maxValues[i] = value;
            }
          }
        }
      }
    }

    if (found) {
      return std::make_unique<SuccessMsgResult>(maxValues, true);
    } else {
      return std::make_unique<NullQueryResult>();
    }

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
