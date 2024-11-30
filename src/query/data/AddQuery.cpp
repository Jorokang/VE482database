#include <algorithm>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "../../db/Database.h"
#include "../Multithread.h"
#include "AddQuery.h"

extern Thread_pool pool;

void ThreadTaskAdd(int threadId, unsigned int threadCount, Table *table,
                   AddQuery *query, size_t *globalCounter,
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
        dest = sum;
        ++localCounter;
      }
    }
  }

  std::lock_guard<std::mutex> const lock(*mutex);
  *globalCounter += localCounter;
}

QueryResult::Ptr AddQuery::execute() {
  if (this->getOperands().size() < 2) {
    return std::make_unique<ErrorMsgResult>(
        qname, this->getTargetTable(),
        "Invalid number of operands (? operands)."_f %
            this->getOperands().size());
  }

  Database &db = Database::getInstance();
  Table::SizeType counter = 0;

  try {
    auto &table = db[this->getTargetTable()];
    this->destFieldId = table.getFieldIndex(this->getOperands().back());
    this->fieldIds.reserve(this->getOperands().size() - 1);

    for (auto it = this->getOperands().begin();
         it != this->getOperands().end() - 1; ++it) {
      this->fieldIds.push_back(table.getFieldIndex(*it));
    }

    std::mutex mutex;
    auto condition = initCondition(table);

    unsigned int threadCount =
        static_cast<unsigned int>(pool.get_idle_thread_num());
    if (threadCount <= 1 || table.size() < MIN_THREAD_REGION_SIZE) {
      // Single-threaded execution
      if (condition.second) {
        for (auto it = table.begin(); it != table.end(); ++it) {
          if (this->evalCondition(*it)) {
            auto &dest = (*it)[this->destFieldId];
            int sum = 0;
            for (const auto &fieldId : this->fieldIds) {
              sum += (*it)[fieldId];
            }
            dest = sum;
            ++counter;
          }
        }
      }
    } else {
      threadCount = std::min(
          threadCount,
          static_cast<unsigned int>(table.size() / MIN_THREAD_REGION_SIZE + 1));
      unsigned int const regionSize =
          static_cast<unsigned int>(table.size()) / threadCount;

      std::vector<std::future<void>> futures;
      futures.reserve(threadCount);

      for (unsigned int i = 0; i < threadCount; ++i) {
        futures.emplace_back(pool.add_task(ThreadTaskAdd, i, threadCount,
                                           &table, this, &counter, &condition,
                                           regionSize, &mutex));
      }

      for (auto &future : futures) {
        future.get();
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

std::string AddQuery::toString() {
  return "QUERY = ADD " + this->getTargetTable() + "\"";
}
