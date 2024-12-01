#include <algorithm>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "../../db/Database.h"
#include "../../db/Table.h"
#include "../Multithread.h"
#include "../QueryResult.h"
#include "DeleteQuery.h"

extern Thread_pool pool;

// Thread function: Collect keys to delete
void ThreadTaskDelete(const int ThreadInd, const unsigned int ThreadNum,
                      Table *table, DeleteQuery *query,
                      std::vector<Table::KeyType> *keysToDelete,
                      const unsigned int RegionSize,
                      const std::pair<std::string, bool> &result,
                      std::mutex *keysMutex) {
  auto head = table->begin() + (ThreadInd * (int)RegionSize);
  auto tail = (ThreadInd == (int)ThreadNum - 1) ? table->end()
                                                : (head + (int)RegionSize);

  std::vector<Table::KeyType> localKeysToDelete;

  if (result.second) {
    for (auto it = head; it != tail; ++it) {
      if (query->evalCondition(*it)) {
        localKeysToDelete.push_back(it->key());
      }
    }
  }

  {
    std::lock_guard<std::mutex> const lock(*keysMutex);
    keysToDelete->insert(keysToDelete->end(), localKeysToDelete.begin(),
                         localKeysToDelete.end());
  }
}

QueryResult::Ptr DeleteQuery::execute() {
  using std::exception;
  using std::invalid_argument;
  using std::make_unique;

  Database &db = Database::getInstance();

  std::mutex keysMutex;
  std::vector<Table::KeyType> keysToDelete;

  try {
    auto &table = db[this->getTargetTable()];
    auto result = initCondition(table);
    Table::SizeType deletedCount = 0;

    unsigned int thread_num = (unsigned int)pool.get_idle_thread_num();
    if (thread_num == 1 || table.size() < MIN_THREAD_REGION_SIZE) { // Single-thread fallback
      if (result.second) {
        for (auto it = table.begin(); it != table.end();) {
          if (this->evalCondition(*it)) {
            table.deleteByIndex(it->key());
            ++deletedCount;
          } else {
            ++it;
          }
        }
      }
    } else {
      thread_num =
          std::min(thread_num,
                   (unsigned int)(table.size() / MIN_THREAD_REGION_SIZE + 1));
      unsigned int const RegionSize = (unsigned int)(table.size()) / thread_num;
      std::vector<std::future<void>> future_vector((uint64_t)thread_num);

      for (uint64_t i = 0; i < thread_num; i++) {
        future_vector[i] = pool.add_task(
            ThreadTaskDelete, i, thread_num, &table, this, &keysToDelete,
            RegionSize, std::ref(result), &keysMutex);
      }

      for (auto &fut : future_vector) {
        fut.get();
      }

      for (const auto &key : keysToDelete) {
        table.deleteByIndex(key);
        ++deletedCount;
      }
    }

    return make_unique<RecordCountResult>(deletedCount);
  } catch (const TableNameNotFound &e) {
    std::cerr << "[DeleteQuery::execute] Error: Table not found.\n";
    return make_unique<ErrorMsgResult>(qname, this->getTargetTable(),
                                       "No such table.");
  } catch (const IllFormedQueryCondition &e) {
    std::cerr << "[DeleteQuery::execute] Error: " << e.what() << "\n";
    return make_unique<ErrorMsgResult>(qname, this->getTargetTable(), e.what());
  } catch (const invalid_argument &e) {
    std::cerr << "[DeleteQuery::execute] Unknown error: " << e.what() << "\n";
    return make_unique<ErrorMsgResult>(qname, this->getTargetTable(),
                                       "Unknown error '?'"_f % e.what());
  } catch (const exception &e) {
    std::cerr << "[DeleteQuery::execute] Unknown error: " << e.what() << "\n";
    return make_unique<ErrorMsgResult>(qname, this->getTargetTable(),
                                       "Unknown error '?'."_f % e.what());
  }
}

// std::string DeleteQuery::toString() {
//   return "QUERY = DELETE " + this->getTargetTable();
// }
