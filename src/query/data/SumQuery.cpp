#include <algorithm>
#include <future>
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
#include "SumQuery.h"

extern Thread_pool pool;

void ThreadTaskSum(int threadId, unsigned int threadCount, Table *table,
                   ComplexQuery *query,
                   const std::vector<std::string> *operands,
                   std::vector<int> *globalSums,
                   const std::pair<std::string, bool> *condition,
                   unsigned int regionSize, std::mutex *mutex) {
  auto head = table->begin() + (threadId * static_cast<int>(regionSize));
  auto tail = (threadId == static_cast<int>(threadCount) - 1)
                  ? table->end()
                  : (head + static_cast<int>(regionSize));

  std::vector<int> localSums(operands->size(), 0);

  if (condition->second) {
    for (auto it = head; it != tail; ++it) {
      if (query->evalCondition(*it)) {
        for (size_t i = 0; i < operands->size(); ++i) {
          localSums[i] += (*it)[(*operands)[i]];
        }
      }
    }
  }

  std::lock_guard<std::mutex> const lock(*mutex);
  for (size_t i = 0; i < operands->size(); ++i) {
    (*globalSums)[i] += localSums[i];
  }
}

QueryResult::Ptr SumQuery::execute() {
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
  std::vector<int> sums(this->getOperands().size(), 0);

  try {
    auto &table = db[this->getTargetTable()];
    auto condition = initCondition(table);

    unsigned int threadCount =
        static_cast<unsigned int>(pool.get_idle_thread_num());
    if (threadCount <= 1 || table.size() < 2000) {
      if (condition.second) {
        for (auto it = table.begin(); it != table.end(); ++it) {
          if (this->evalCondition(*it)) {
            for (size_t i = 0; i < this->getOperands().size(); ++i) {
              sums[i] += (*it)[this->getOperands()[i]];
            }
          }
        }
      }
      return make_unique<SuccessMsgResult>(sums, true);
    }

    threadCount = min(
        threadCount,
        static_cast<unsigned int>(table.size() / MIN_THREAD_REGION_SIZE + 1));
    unsigned int const regionSize =
        static_cast<unsigned int>(table.size()) / threadCount;

    std::vector<std::future<void>> futures;
    futures.reserve(threadCount);

    for (unsigned int i = 0; i < threadCount; ++i) {
      futures.emplace_back(pool.add_task(ThreadTaskSum, i, threadCount, &table,
                                         this, &this->getOperands(), &sums,
                                         &condition, regionSize, &mutex));
    }

    for (auto &future : futures) {
      future.get();
    }

    return make_unique<SuccessMsgResult>(sums, true);

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

std::string SumQuery::toString() {
  return "QUERY = SUM " + this->getTargetTable() + "\"";
}
