//
// Created by liu on 18-10-25.
//

#include "InsertQuery.h"

#include <algorithm>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../../db/Database.h"
#include "../QueryResult.h"

// constexpr const char *InsertQuery::qname;

QueryResult::Ptr InsertQuery::execute() {
  using std::exception;
  using std::invalid_argument;
  using std::make_unique;
  using std::vector;

  if (this->getOperands().empty())
    return make_unique<ErrorMsgResult>(qname, this->getTargetTable().c_str(),
                                       "No operand (? this->getOperands())."_f %
                                           this->getOperands().size());
  Database &db = Database::getInstance();
  try {
    auto &table = db[this->getTargetTable()];
    auto &key = this->getOperands().front();
    vector<Table::ValueType> data;
    data.reserve(this->getOperands().size() - 1);
    for (auto it = ++this->getOperands().begin();
         it != this->getOperands().end(); ++it) {
      data.emplace_back(strtol(it->c_str(), nullptr, 10));
    }
    table.insertByIndex(key, std::move(data));
    return std::make_unique<SuccessMsgResult>(qname, this->getTargetTable());
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

// std::string InsertQuery::toString() {
//   return "QUERY = INSERT " + this->getTargetTable() + "\"";
// }
