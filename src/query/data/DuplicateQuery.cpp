#include <algorithm>
#include <cstdint>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>
#include <vector>

#include "../../db/Database.h"
#include "../Multithread.h"
#include "DuplicateQuery.h"

extern Thread_pool pool;

void ThreadTaskDuplicate(const int ThreadInd, const unsigned int ThreadNum,
                         Table *table, DuplicateQuery *query,
                         std::vector<Table::KeyType> *toBeInserted,
                         size_t *counter,
                         const std::pair<std::string, bool> &result,
                         const unsigned int RegionSize, std::mutex *mut) {
  auto head = table->begin() + (ThreadInd * (int)RegionSize);
  auto tail = (ThreadInd == (int)ThreadNum - 1) ? table->end()
                                                : (head + (int)RegionSize);

  std::vector<Table::KeyType> localToBeInserted;
  size_t localCounter = 0;

  if (result.second) {
    for (auto it = head; it != tail; ++it) {
      if (query->evalCondition(*it) && table->checkNotDuplicate(it->key())) {
        localToBeInserted.push_back(it->key());
        localCounter++;
      }
    }
  }

  {
    std::lock_guard<std::mutex> const lock(*mut);
    toBeInserted->insert(toBeInserted->end(), localToBeInserted.begin(),
                         localToBeInserted.end());
    *counter += localCounter;
  }
}

QueryResult::Ptr DuplicateQuery::execute() {
  using std::exception;
  using std::invalid_argument;
  using std::make_unique;
  using std::vector;

  Database &db = Database::getInstance();
  Table::SizeType counter = 0;
  std::mutex mut;

  try {
    Table &table = db[this->getTargetTable()];
    auto result = initCondition(table);

    unsigned int thread_num = (unsigned int)pool.get_idle_thread_num();
    if (thread_num == 1 || table.size() < 2000) { // Single-threaded execution
      vector<Table::KeyType> toBeInserted;
      if (result.second) {
        for (auto it = table.begin(); it != table.end(); ++it) {
          if (this->evalCondition(*it) && table.checkNotDuplicate(it->key())) {
            toBeInserted.push_back(it->key());
            counter++;
          }
        }
      }
      table.duplicateKey(toBeInserted);
    } else {
      thread_num =
          std::min(thread_num, (unsigned int)(table.size() / 2000 + 1));
      unsigned int const RegionSize = (unsigned int)(table.size()) / thread_num;

      std::vector<std::future<void>> future_vector((uint64_t)thread_num);
      vector<Table::KeyType> toBeInserted;

      for (uint64_t i = 0; i < thread_num; i++) {
        future_vector[i] = pool.add_task(ThreadTaskDuplicate, i, thread_num,
                                         &table, this, &toBeInserted, &counter,
                                         std::ref(result), RegionSize, &mut);
      }

      for (auto &fut : future_vector) {
        fut.get();
      }

      table.duplicateKey(toBeInserted);
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
                                       "Unknown error '?'."_f % e.what());
  }
}

std::string DuplicateQuery::toString() {
  return "QUERY = DUPLICATE " + this->getTargetTable();
}
