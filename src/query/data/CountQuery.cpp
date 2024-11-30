// #include <exception>
// #include <memory>
// #include <stdexcept>

// #include "../../db/Database.h"
// #include "CountQuery.h"

// // constexpr const char *CountQuery::qname;

// QueryResult::Ptr CountQuery::execute() {
//   // using namespace std;
//   using std::exception;
//   using std::invalid_argument;
//   using std::make_unique;
//   using std::string;

//   Database &db = Database::getInstance();

//   try {
//     Table &table = db[this->getTargetTable()];
//     auto result = initCondition(table);
//     size_t count = 0;
//     if (result.second) {
//       for (auto it = table.begin(); it != table.end(); ++it) {
//         if (this->evalCondition(*it)) {
//           ++count;
//         }
//       }
//     }
//     return make_unique<SuccessMsgResult>(count, true);
//   } catch (const TableNameNotFound &e) {
//     return make_unique<ErrorMsgResult>(qname, this->getTargetTable(),
//                                        "No such table.");
//   } catch (const IllFormedQueryCondition &e) {
//     return make_unique<ErrorMsgResult>(qname, this->getTargetTable(),
//     e.what());
//   } catch (const invalid_argument &e) {
//     return make_unique<ErrorMsgResult>(qname, this->getTargetTable(),
//                                        "Unknown error '?'"_f % e.what());
//   } catch (const exception &e) {
//     return make_unique<ErrorMsgResult>(qname, this->getTargetTable(),
//                                        "Unknown error '?'."_f % e.what());
//   }
// }

// std::string CountQuery::toString() {
//   return "QUERY = COUNT " + this->getTargetTable() + "\"";
// }

#include <cstdint>
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
#include "CountQuery.h"

extern Thread_pool pool;

void ThreadTaskCount(int threadId, unsigned int threadCount, Table *table,
                     ComplexQuery *query, size_t *globalCount,
                     const std::pair<std::string, bool> &condition,
                     unsigned int regionSize, std::mutex *countMutex) {
  auto head = table->begin() + (threadId * (int)regionSize);
  auto tail = (threadId == (int)threadCount - 1) ? table->end()
                                                 : (head + (int)regionSize);

  size_t localCount = 0;

  if (condition.second) {
    for (auto it = head; it != tail; ++it) {
      if (query->evalCondition(*it)) {
        ++localCount;
      }
    }
  }

  {
    std::lock_guard<std::mutex> const lock(*countMutex);
    *globalCount += localCount;
  }
}

QueryResult::Ptr CountQuery::execute() {
  using std::exception;
  using std::make_unique;
  using std::min;

  Database &db = Database::getInstance();
  size_t count = 0;
  std::mutex countMutex;

  try {
    auto &table = db[this->getTargetTable()];
    auto condition = initCondition(table);

    unsigned int threadCount = (unsigned int)pool.get_idle_thread_num();
    if (threadCount <= 1 || table.size() < 2000) {
      if (condition.second) {
        for (auto it = table.begin(); it != table.end(); ++it) {
          if (this->evalCondition(*it)) {
            ++count;
          }
        }
      }
      return make_unique<SuccessMsgResult>(count, true);
    }

    threadCount = min(
        threadCount, (unsigned int)(table.size() / MIN_THREAD_REGION_SIZE + 1));
    unsigned int const regionSize = (unsigned int)(table.size()) / threadCount;

    std::vector<std::future<void>> futureVector((uint64_t)threadCount);
    for (unsigned int i = 0; i < threadCount; ++i) {
      futureVector[i] =
          pool.add_task(ThreadTaskCount, i, threadCount, &table, this, &count,
                        std::ref(condition), regionSize, &countMutex);
    }

    for (auto &future : futureVector) {
      future.get();
    }

    return make_unique<SuccessMsgResult>(count, true);
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

std::string CountQuery::toString() {
  return "QUERY = COUNT " + this->getTargetTable() + "\"";
}