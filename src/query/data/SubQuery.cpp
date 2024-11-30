#include <exception>
#include <memory>
#include <stdexcept>

#include "../../db/Database.h"
#include "SubQuery.h"

// constexpr const char *SubQuery::qname;

QueryResult::Ptr SubQuery::execute() {
  // using namespace std;
  using std::exception;
  using std::invalid_argument;
  using std::make_unique;

  auto opcount = this->getOperands().size();
  if (opcount < 2)
    return std::make_unique<ErrorMsgResult>(
        qname, this->getTargetTable().c_str(),
        "Invalid number of this->getOperands() (? this->getOperands())."_f %
            getOperands().size());
  Database &db = Database::getInstance();
  Table::SizeType counter = 0;
  try {
    auto &table = db[this->getTargetTable()];
    this->destFieldId = table.getFieldIndex(this->getOperands()[opcount - 1]);
    this->srcFieldId = table.getFieldIndex(this->getOperands()[0]);
    if (opcount == 2) {
      auto result = initCondition(table);
      if (result.second) {
        for (auto it = table.begin(); it != table.end(); ++it) {
          if (this->evalCondition(*it)) {
            auto &dest = (*it)[this->destFieldId];
            dest = (*it)[this->srcFieldId];
            ++counter;
          }
        }
      }
    } else {
      this->fieldIds.reserve(opcount - 2);
      for (auto it = this->getOperands().begin() + 1;
           it != this->getOperands().end() - 1; ++it) {
        this->fieldIds.push_back(table.getFieldIndex(*it));
      }
      auto result = initCondition(table);
      if (result.second) {
        for (auto it = table.begin(); it != table.end(); ++it) {
          if (this->evalCondition(*it)) {
            auto &dest = (*it)[this->destFieldId];
            auto sum = 0;
            for (auto &fieldId : this->fieldIds) {
              sum += (*it)[fieldId];
            }
            dest = (*it)[this->srcFieldId] - sum;
            ++counter;
          }
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

std::string SubQuery::toString() {
  return "QUERY = SUB " + this->getTargetTable() + "\"";
}