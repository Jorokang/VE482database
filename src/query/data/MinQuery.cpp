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
#include "MinQuery.h"

extern Thread_pool pool;

void ThreadTaskMin(int threadId, unsigned int threadCount, Table *table,
                   ComplexQuery *query,
                   const std::vector<std::string> *operands,
                   std::vector<int> *globalMinValues,
                   const std::pair<std::string, bool> *condition,
                   unsigned int regionSize, std::mutex *mutex, bool *found) {
  auto head = table->begin() + (threadId * static_cast<int>(regionSize));
  auto tail = (threadId == static_cast<int>(threadCount) - 1)
                  ? table->end()
                  : (head + static_cast<int>(regionSize));

  std::vector<int> localMinValues(operands->size(),
                                  std::numeric_limits<int>::max());
  bool localFound = false;

  if (condition->second) {
    for (auto it = head; it != tail; ++it) {
      if (query->evalCondition(*it)) {
        localFound = true;
        for (size_t i = 0; i < operands->size(); ++i) {
          auto value = (*it)[(*operands)[i]];
          if (value < localMinValues[i]) {
            localMinValues[i] = value;
          }
        }
      }
    }
  }

  std::lock_guard<std::mutex> const lock(*mutex);
  if (localFound) {
    *found = true;
  }
  for (size_t i = 0; i < localMinValues.size(); ++i) {
    if (localMinValues[i] < (*globalMinValues)[i]) {
      (*globalMinValues)[i] = localMinValues[i];
    }
  }
}

QueryResult::Ptr MinQuery::execute() {
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
  std::vector<int> minValues(this->getOperands().size(),
                             std::numeric_limits<int>::max());

  try {
    auto &table = db[this->getTargetTable()];
    auto condition = initCondition(table);

    unsigned int threadCount =
        static_cast<unsigned int>(pool.get_idle_thread_num());
    if (threadCount <= 1 || table.size() < 2000) {
      if (condition.second) {
        for (auto it = table.begin(); it != table.end(); ++it) {
          if (this->evalCondition(*it)) {
            found = true;
            for (size_t i = 0; i < this->getOperands().size(); ++i) {
              auto value = (*it)[this->getOperands()[i]];
              if (value < minValues[i]) {
                minValues[i] = value;
              }
            }
          }
        }
      }
      if (found) {
        return make_unique<SuccessMsgResult>(minValues, true);
      } else {
        return make_unique<NullQueryResult>();
      }
    }

    threadCount = min(
        threadCount,
        static_cast<unsigned int>(table.size() / MIN_THREAD_REGION_SIZE + 1));
    unsigned int const regionSize =
        static_cast<unsigned int>(table.size()) / threadCount;

    std::vector<std::future<void>> futures;
    futures.reserve(threadCount);
    for (unsigned int i = 0; i < threadCount; ++i) {
      futures.emplace_back(pool.add_task(
          ThreadTaskMin, i, threadCount, &table, this, &this->getOperands(),
          &minValues, &condition, regionSize, &mutex, &found));
    }

    for (auto &future : futures) {
      future.get();
    }

    if (found) {
      return make_unique<SuccessMsgResult>(minValues, true);
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

// std::string MinQuery::toString() {
//   return "QUERY = MIN " + this->getTargetTable() + "\"";
// }
