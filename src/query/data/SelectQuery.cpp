#include <exception>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../../db/Database.h"
#include "SelectQuery.h"

constexpr const char *SelectQuery::qname;

QueryResult::Ptr SelectQuery::execute() {
  using std::exception;
  using std::invalid_argument;
  using std::make_unique;
  using std::ostringstream;
  using std::pair;
  using std::sort;
  using std::string;
  using std::vector;

  Database &db = Database::getInstance();
  try {
    auto &table = db[this->targetTable];
    auto result = initCondition(table);

    vector<Table::FieldIndex> targetFields;
    targetFields.reserve(this->operands.size());
    for (auto it = this->operands.begin() + 1; it != this->operands.end();
         ++it) {
      targetFields.push_back(table.getFieldIndex(*it));
    }

    vector<pair<string, string>> selected;
    if (result.second) {
      for (auto it = table.begin(); it != table.end(); ++it) {
        if (this->evalCondition(*it)) {
          Table::KeyType const key = it->key();
          string values;
          for (auto &fieldId : targetFields) {
            values += to_string(it->get(fieldId)) + " ";
          }
          selected.push_back(make_pair(key, values));
        }
      }
      sort(selected.begin(), selected.end(),
           [](const auto &a, const auto &b) { return a.first < b.first; });
    }
    // Format the result
    ostringstream os;
    for (auto it = selected.begin(); it != selected.end(); ++it) {
      os << "( " << it->first << " ";
      os << it->second << ")";
      os << "\n";
    }

    return make_unique<SuccessMsgResult>(os.str(), true);
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

std::string SelectQuery::toString() {
  return "QUERY = SELECT " + this->targetTable + "\"";
}