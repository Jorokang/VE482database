#include <cstdint>
#include <future>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "../../db/Database.h"
#include "../../db/Table.h"
#include "../../utils/uexception.h"
#include "../Multithread.h"
#include "../QueryResult.h"
#include "MaxQuery.h"

extern Thread_pool pool;

// Threaded task to calculate maximum values in a subtable
void ThreadTaskMax(int threadId, unsigned int threadCount, Table *table,
                   ComplexQuery *query,
                   const std::vector<std::string> *operands,
                   std::vector<int> *globalMaxValues,
                   const std::pair<std::string, bool> *condition,
                   unsigned int regionSize, std::mutex *mutex, bool *found) {
  auto head = table->begin() + (threadId * static_cast<int>(regionSize));
  auto tail = (threadId == static_cast<int>(threadCount) - 1)
                  ? table->end()
                  : (head + static_cast<int>(regionSize));

  std::vector<int> localMaxValues(operands->size(),
                                  std::numeric_limits<int>::min());
  bool localFound = false;

  if (condition->second) {
    for (auto it = head; it != tail; ++it) {
      if (query->evalCondition(*it)) {
        localFound = true;
        for (size_t i = 0; i < operands->size(); ++i) {
          int const value = (*it)[(*operands)[i]];
          if (value > localMaxValues[i]) {
            localMaxValues[i] = value;
          }
        }
      }
    }
  }

  // Safely update global results
  std::lock_guard<std::mutex> const lock(*mutex);
  if (localFound) {
    *found = true;
  }
  for (size_t i = 0; i < localMaxValues.size(); ++i) {
    if (localMaxValues[i] > (*globalMaxValues)[i]) {
      (*globalMaxValues)[i] = localMaxValues[i];
    }
  }
}

QueryResult::Ptr MaxQuery::execute() {
  using std::exception;
  using std::make_unique;
  using std::min;

  if (this->getOperands().empty()) {
    return make_unique<ErrorMsgResult>(
        qname, this->getTargetTable(),
        "Invalid number of operands (? operands)."_f %
            this->getOperands().size());
  }

  Database &db = Database::getInstance();
  std::mutex mutex;
  bool found = false;
  std::vector<int> maxValues(this->getOperands().size(),
                             std::numeric_limits<int>::min());

  try {
    auto &table = db[this->getTargetTable()];
    auto condition = initCondition(table);

    unsigned int threadCount =
        static_cast<unsigned int>(pool.get_idle_thread_num());
    if (threadCount <= 1 || table.size() < 2000) {
      // Single-threaded execution
      if (condition.second) {
        for (auto it = table.begin(); it != table.end(); ++it) {
          if (this->evalCondition(*it)) {
            found = true;
            for (size_t i = 0; i < this->getOperands().size(); ++i) {
              int const value = (*it)[this->getOperands()[i]];
              if (value > maxValues[i]) {
                maxValues[i] = value;
              }
            }
          }
        }
      }
      if (found) {
        return make_unique<SuccessMsgResult>(maxValues, true);
      } else {
        return make_unique<NullQueryResult>();
      }
    }

    // Multithreaded execution
    threadCount = min(
        threadCount,
        static_cast<unsigned int>(table.size() / MIN_THREAD_REGION_SIZE + 1));
    unsigned int const regionSize =
        static_cast<unsigned int>(table.size()) / threadCount;

    std::vector<std::future<void>> futures((uint64_t)threadCount);
    // futures.reserve(threadCount);
    for (unsigned int i = 0; i < threadCount; ++i) {
      futures[i] = pool.add_task(ThreadTaskMax, i, threadCount, &table, this,
                                 &this->getOperands(), &maxValues, &condition,
                                 regionSize, &mutex, &found);
    }

    for (auto &future : futures) {
      future.get(); // Ensure all threads complete
    }

    if (found) {
      return make_unique<SuccessMsgResult>(maxValues, true);
    } else {
      return make_unique<NullQueryResult>();
    }
  } catch (const TableNameNotFound &e) {
    return make_unique<ErrorMsgResult>(qname, this->getTargetTable(),
                                       "No such table.");
  } catch (const TableFieldNotFound &e) {
    return make_unique<ErrorMsgResult>(qname, this->getTargetTable(), e.what());
  } catch (const exception &e) {
    return make_unique<ErrorMsgResult>(qname, this->getTargetTable(),
                                       "Unknown error '?'."_f % e.what());
  }
}

// std::string MaxQuery::toString() {
//   return "QUERY = MAX " + this->getTargetTable() + "\"";
// }
