#include "MaxQuery.h"
#include "../../db/Database.h"
#include "../../db/Table.h"
#include "../../utils/uexception.h"
#include "../QueryResult.h"
#include <limits>

/**********************************************/
/* Define Global Variables */
constexpr const char *MaxQuery::qname;
/**********************************************/

std::string MaxQuery::toString() {
  return "QUERY = MAX " + this->targetTable + "\"";
}

QueryResult::Ptr MaxQuery::execute() {

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

    std::vector<int> maxValues(this->operands.size(), INT32_MIN);
    if (condition.second) {
      for (auto row = table.begin(); row != table.end(); ++row) {
        if (this->evalCondition(*row)) {
          found = true;
          for (size_t i = 0; i < this->operands.size(); ++i) {
            auto value = (*row)[this->operands[i]];
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
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable,
                                            "No such table.");
  } catch (const TableFieldNotFound &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
  } catch (const std::exception &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable,
                                            "Unknown error '?'"_f % e.what());
  }
}
