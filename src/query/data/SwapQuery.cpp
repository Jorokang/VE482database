#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "../../db/Database.h"
#include "SwapQuery.h"

// constexpr const char *SwapQuery::qname;

QueryResult::Ptr SwapQuery::execute() {
  using std::exception;
  using std::invalid_argument;
  using std::make_unique;

  auto opcount = this->getOperands().size();
  if (opcount != 2)
    return std::make_unique<ErrorMsgResult>(
        qname, this->getTargetTable().c_str(),
        "Invalid number of this->getOperands() (? this->getOperands())."_f %
            this->getOperands().size());
  Database &db = Database::getInstance();
  Table::SizeType counter = 0;
  try {
    auto &table = db[this->getTargetTable()];
    this->fieldId1 = table.getFieldIndex(this->getOperands()[0]);
    this->fieldId2 = table.getFieldIndex(this->getOperands()[1]);
    auto result = initCondition(table);
    if (result.second) {
      for (auto it = table.begin(); it != table.end(); ++it) {
        if (this->evalCondition(*it)) {
          auto &field1 = (*it)[this->fieldId1];
          auto &field2 = (*it)[this->fieldId2];
          std::swap(field1, field2);
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
    return make_unique<ErrorMsgResult>(qname, this->getTargetTable(),
                                       "Unknown error '?'"_f % e.what());
  } catch (const exception &e) {
    return make_unique<ErrorMsgResult>(qname, this->getTargetTable(),
                                       "Unkonwn error '?'."_f % e.what());
  }
}

std::string SwapQuery::toString() {
  return "QUERY = SWAP " + this->getTargetTable() + "\"";
}
