// #include <exception>
// #include <memory>
// #include <stdexcept>

// #include "../../db/Database.h"
// #include "SubQuery.h"

// // constexpr const char *SubQuery::qname;

// QueryResult::Ptr SubQuery::execute() {
//   // using namespace std;
//   using std::exception;
//   using std::invalid_argument;
//   using std::make_unique;

//   auto opcount = this->getOperands().size();
//   if (opcount < 2)
//     return std::make_unique<ErrorMsgResult>(
//         qname, this->getTargetTable().c_str(),
//         "Invalid number of this->getOperands() (? this->getOperands())."_f %
//             getOperands().size());
//   Database &db = Database::getInstance();
//   Table::SizeType counter = 0;
//   try {
//     auto &table = db[this->getTargetTable()];
//     this->destFieldId = table.getFieldIndex(this->getOperands()[opcount -
//     1]); this->srcFieldId = table.getFieldIndex(this->getOperands()[0]); if
//     (opcount == 2) {
//       auto result = initCondition(table);
//       if (result.second) {
//         for (auto it = table.begin(); it != table.end(); ++it) {
//           if (this->evalCondition(*it)) {
//             auto &dest = (*it)[this->destFieldId];
//             dest = (*it)[this->srcFieldId];
//             ++counter;
//           }
//         }
//       }
//     } else {
//       this->fieldIds.reserve(opcount - 2);
//       for (auto it = this->getOperands().begin() + 1;
//            it != this->getOperands().end() - 1; ++it) {
//         this->fieldIds.push_back(table.getFieldIndex(*it));
//       }
//       auto result = initCondition(table);
//       if (result.second) {
//         for (auto it = table.begin(); it != table.end(); ++it) {
//           if (this->evalCondition(*it)) {
//             auto &dest = (*it)[this->destFieldId];
//             auto sum = 0;
//             for (auto &fieldId : this->fieldIds) {
//               sum += (*it)[fieldId];
//             }
//             dest = (*it)[this->srcFieldId] - sum;
//             ++counter;
//           }
//         }
//       }
//     }
//     return make_unique<RecordCountResult>(counter);
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
//                                        "Unkonwn error '?'."_f % e.what());
//   }
// }

// std::string SubQuery::toString() {
//   return "QUERY = SUB " + this->getTargetTable() + "\"";
// }

#include <algorithm>
#include <future>
#include <mutex>
#include <string>
#include <vector>

#include "../../db/Database.h"
#include "../Multithread.h"
#include "SubQuery.h"

extern Thread_pool pool;

// Thread task for subtraction operation
void ThreadTaskSub(int threadId, unsigned int threadCount, Table *table,
                   SubQuery *query, size_t *globalCounter,
                   const std::pair<std::string, bool> *condition,
                   unsigned int regionSize, std::mutex *mutex) {
  auto head = table->begin() + (threadId * static_cast<int>(regionSize));
  auto tail = (threadId == static_cast<int>(threadCount) - 1)
                  ? table->end()
                  : (head + static_cast<int>(regionSize));

  size_t localCounter = 0;

  if (condition->second) {
    for (auto it = head; it != tail; ++it) {
      if (query->evalCondition(*it)) {
        auto &dest = (*it)[query->getDestFieldId()];
        int sum = 0;
        for (const auto &fieldId : query->getFieldIds()) {
          sum += (*it)[fieldId];
        }
        dest = (*it)[query->getSrcFieldId()] - sum;
        ++localCounter;
      }
    }
  }

  std::lock_guard<std::mutex> const lock(*mutex);
  *globalCounter += localCounter;
}

// Thread task for equality operation
void ThreadTaskEqual(int threadId, unsigned int threadCount, Table *table,
                     SubQuery *query, size_t *globalCounter,
                     const std::pair<std::string, bool> *condition,
                     unsigned int regionSize, std::mutex *mutex) {
  auto head = table->begin() + (threadId * static_cast<int>(regionSize));
  auto tail = (threadId == static_cast<int>(threadCount) - 1)
                  ? table->end()
                  : (head + static_cast<int>(regionSize));

  size_t localCounter = 0;

  if (condition->second) {
    for (auto it = head; it != tail; ++it) {
      if (query->evalCondition(*it)) {
        auto &dest = (*it)[query->getDestFieldId()];
        dest = (*it)[query->getSrcFieldId()];
        ++localCounter;
      }
    }
  }

  std::lock_guard<std::mutex> const lock(*mutex);
  *globalCounter += localCounter;
}

QueryResult::Ptr SubQuery::execute() {
  if (this->getOperands().size() < 2) {
    return std::make_unique<ErrorMsgResult>(
        qname, this->getTargetTable(),
        "Invalid number of operands (? operands)."_f %
            this->getOperands().size());
  }

  Database &db = Database::getInstance();
  Table::SizeType counter = 0;
  std::mutex mutex;

  try {
    auto &table = db[this->getTargetTable()];
    this->destFieldId = table.getFieldIndex(this->getOperands().back());
    this->srcFieldId = table.getFieldIndex(this->getOperands().front());

    unsigned int threadCount =
        static_cast<unsigned int>(pool.get_idle_thread_num());
    auto condition = initCondition(table);

    if (threadCount <= 1 || table.size() < MIN_THREAD_REGION_SIZE) {
      // Single-threaded execution
      if (this->getOperands().size() == 2) {
        if (condition.second) {
          for (auto it = table.begin(); it != table.end(); ++it) {
            if (this->evalCondition(*it)) {
              auto &dest = (*it)[this->destFieldId];
              dest = (*it)[this->srcFieldId];
              ++counter;
            }
          }
        }
      } else {
        this->fieldIds.reserve(this->getOperands().size() - 2);
        for (auto it = this->getOperands().begin() + 1;
             it != this->getOperands().end() - 1; ++it) {
          this->fieldIds.push_back(table.getFieldIndex(*it));
        }
        if (condition.second) {
          for (auto it = table.begin(); it != table.end(); ++it) {
            if (this->evalCondition(*it)) {
              auto &dest = (*it)[this->destFieldId];
              int sum = 0;
              for (const auto &fieldId : this->fieldIds) {
                sum += (*it)[fieldId];
              }
              dest = (*it)[this->srcFieldId] - sum;
              ++counter;
            }
          }
        }
      }
    } else {
      // Multithreaded execution
      threadCount = std::min(
          threadCount,
          static_cast<unsigned int>(table.size() / MIN_THREAD_REGION_SIZE + 1));
      unsigned int const regionSize =
          static_cast<unsigned int>(table.size()) / threadCount;

      std::vector<std::future<void>> futures;
      futures.reserve(threadCount);

      if (this->getOperands().size() == 2) {
        for (unsigned int i = 0; i < threadCount; ++i) {
          futures.emplace_back(pool.add_task(ThreadTaskEqual, i, threadCount,
                                             &table, this, &counter, &condition,
                                             regionSize, &mutex));
        }
      } else {
        this->fieldIds.reserve(this->getOperands().size() - 2);
        for (auto it = this->getOperands().begin() + 1;
             it != this->getOperands().end() - 1; ++it) {
          this->fieldIds.push_back(table.getFieldIndex(*it));
        }
        for (unsigned int i = 0; i < threadCount; ++i) {
          futures.emplace_back(pool.add_task(ThreadTaskSub, i, threadCount,
                                             &table, this, &counter, &condition,
                                             regionSize, &mutex));
        }
      }

      for (auto &future : futures) {
        future.get(); // Ensure all threads complete
      }
    }

    return std::make_unique<RecordCountResult>(counter);

  } catch (const TableNameNotFound &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->getTargetTable(),
                                            "No such table.");
  } catch (const IllFormedQueryCondition &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->getTargetTable(),
                                            e.what());
  } catch (const std::invalid_argument &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->getTargetTable(),
                                            "Unknown error '?'"_f % e.what());
  } catch (const std::exception &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->getTargetTable(),
                                            "Unknown error '?'."_f % e.what());
  }
}

std::string SubQuery::toString() {
  return "QUERY = SUB " + this->getTargetTable() + "\"";
}
