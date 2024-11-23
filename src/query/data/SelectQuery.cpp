#include "SelectQuery.h"

#include <iostream>

#include "../../db/Database.h"

constexpr const char *SelectQuery::qname;

// In SelectQuery.cpp (example execution logic)
QueryResult::Ptr SelectQuery::execute() {
    using namespace std;

    auto opcount = this->operands.size();
    if (opcount == 0) {
        return make_unique<ErrorMsgResult>(
            qname, this->targetTable.c_str(),
            "No fields selected."s);
    }

    Database &db = Database::getInstance();
    try {
        auto &table = db[this->targetTable];


        auto result = initCondition(table);
        if (result.second) {
            vector<vector<string>> selectedRecords;
            size_t recordCount = 0;


            for (auto it = table.begin(); it != table.end(); ++it) {
                if (evalCondition(*it)) {
                    vector<string> record;
                    for (const auto &field : this->operands) {
                        try {
                            auto fieldValue = (*it).get(field);
                            record.push_back(to_string(fieldValue));
                        } catch (const TableFieldNotFound &e) {
                            return make_unique<ErrorMsgResult>(
                                qname, this->targetTable, e.what());
                        }
                    }
                    selectedRecords.push_back(record);
                    ++recordCount;
                }
            }


            cout << recordCount << endl;
            return make_unique<SelectQueryResult>(recordCount, selectedRecords);
        } else {
            return make_unique<ErrorMsgResult>(
                qname, this->targetTable.c_str(),
                "Invalid query condition."s);
        }

    } catch (const TableNameNotFound &e) {
        return make_unique<ErrorMsgResult>(qname, this->targetTable, "No such table.");
    } catch (const IllFormedQueryCondition &e) {
        return make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
    } catch (const exception &e) {
      return make_unique<ErrorMsgResult>(
  qname, this->targetTable, "Unknown error: " + std::string(e.what()));

    }
}

std::string join(const std::vector<std::string>& vec, const std::string& delimiter) {
  std::ostringstream oss;
  for (size_t i = 0; i < vec.size(); ++i) {
    if (i != 0) {
      oss << delimiter;
    }
    oss << vec[i];
  }
  return oss.str();
}

std::string SelectQuery::toString() {
    std::string queryStr = "QUERY = SELECT ";
    queryStr += (this->operands.empty()) ? "*" : join(this->operands, ", "); // 如果没有字段，默认查询所有
    queryStr += " FROM " + this->targetTable;

    if (!this->condition.empty()) {
        queryStr += " WHERE ";
        for (const auto &cond : this->condition) {
            queryStr += cond.field + " " + cond.op + " " + cond.value + " AND ";
        }
        queryStr = queryStr.substr(0, queryStr.size() - 4); // 去除最后的 " AND "
    }

    return queryStr;
}
