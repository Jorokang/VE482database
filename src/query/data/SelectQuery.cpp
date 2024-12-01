#include <algorithm>
#include <exception>
#include <future>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../../db/Database.h"
#include "../Multithread.h"
#include "SelectQuery.h"

extern Thread_pool pool;

void ThreadTaskSelect(
    const int ThreadInd, const unsigned int ThreadNum, Table *table,
    SelectQuery *query, const std::vector<Table::FieldIndex> *targetFields,
    std::vector<std::pair<std::string, std::string>> *selected,
    const unsigned int RegionSize, const std::pair<std::string, bool> &result,
    std::mutex *selectedMutex) {
  auto head = table->begin() + (ThreadInd * (int)RegionSize);
  auto tail = (ThreadInd == (int)ThreadNum - 1) ? table->end()
                                                : (head + (int)RegionSize);

  std::vector<std::pair<std::string, std::string>> localSelected;

  if (result.second) {
    for (auto it = head; it != tail; ++it) {
      if (query->evalCondition(*it)) {
        Table::KeyType const key = it->key();
        std::string values;
        for (auto &fieldId : *targetFields) {
          values += to_string(it->get(fieldId)) + " ";
        }
        localSelected.push_back(std::make_pair(key, values));
      }
    }
  }

  {
    std::lock_guard<std::mutex> const lock(*selectedMutex);
    selected->insert(selected->end(), localSelected.begin(),
                     localSelected.end());
  }
}

QueryResult::Ptr SelectQuery::execute() {
  using std::exception;
  using std::invalid_argument;
  using std::make_unique;
  using std::ostringstream;
  using std::sort;
  using std::string;
  using std::vector;

  Database &db = Database::getInstance();
  vector<std::pair<std::string, std::string>> selected;
  std::mutex selectedMutex;

  try {
    auto &table = db[this->getTargetTable()];
    auto result = initCondition(table);

    vector<Table::FieldIndex> targetFields;
    targetFields.reserve(this->getOperands().size());
    for (auto it = this->getOperands().begin() + 1;
         it != this->getOperands().end(); ++it) {
      targetFields.push_back(table.getFieldIndex(*it));
    }

    unsigned int thread_num = (unsigned int)pool.get_idle_thread_num();
    if (thread_num == 1 || table.size() < MIN_THREAD_REGION_SIZE) {
      if (result.second) {
        for (auto it = table.begin(); it != table.end(); ++it) {
          if (this->evalCondition(*it)) {
            Table::KeyType const key = it->key();
            string values;
            for (auto &fieldId : targetFields) {
              values += to_string(it->get(fieldId)) + " ";
            }
            selected.push_back(std::make_pair(key, values));
          }
        }
        sort(selected.begin(), selected.end(),
             [](const auto &a, const auto &b) { return a.first < b.first; });
      }
    } else {
      thread_num =
          std::min(thread_num,
                   (unsigned int)(table.size() / MIN_THREAD_REGION_SIZE + 1));
      unsigned int const RegionSize = (unsigned int)(table.size()) / thread_num;
      std::vector<std::future<void>> future_vector((uint64_t)thread_num);

      for (uint64_t i = 0; i < thread_num; i++) {
        future_vector[i] = pool.add_task(
            ThreadTaskSelect, i, thread_num, &table, this, &targetFields,
            &selected, RegionSize, std::ref(result), &selectedMutex);
      }

      for (auto &fut : future_vector) {
        fut.get();
      }

      sort(selected.begin(), selected.end(),
           [](const auto &a, const auto &b) { return a.first < b.first; });
    }

    ostringstream os;
    for (auto it = selected.begin(); it != selected.end(); ++it) {
      os << "( " << it->first << " ";
      os << it->second << ")";
      os << "\n";
    }

    return make_unique<SuccessMsgResult>(os.str(), true);
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
                                       "Unknown error '?'."_f % e.what());
  }
}

// std::string SelectQuery::toString() {
//   return "QUERY = SELECT " + this->getTargetTable() + "\"";
// }
