#include <future>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../../db/Database.h"
#include "../Multithread.h"
#include "UpdateQuery.h"

extern Thread_pool pool;

void ThreadTaskUpdate(int threadId, unsigned int threadCount, Table *table,
                      UpdateQuery *query, size_t *globalCounter,
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
        // std::lock_guard<std::mutex> const lock(*mutex);
        if (query->getKeyValue().empty()) {
          (*it)[query->getFieldId()] = query->getFieldValue();
        } else {
          it->setKey(query->getKeyValue());
        }
        ++localCounter;
      }
    }
  }

  std::lock_guard<std::mutex> const lock(*mutex);
  *globalCounter += localCounter;
}

QueryResult::Ptr UpdateQuery::execute() {
  if (this->getOperands().size() != 2) {
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

    if (this->getOperands()[0] == "KEY") {
      this->keyValue = this->getOperands()[1];
    } else {
      this->fieldId = table.getFieldIndex(this->getOperands()[0]);
      this->fieldValue = static_cast<Table::ValueType>(
          strtol(this->getOperands()[1].c_str(), nullptr, 10));
    }

    unsigned int threadCount =
        static_cast<unsigned int>(pool.get_idle_thread_num());
    auto condition = initCondition(table);

    if (threadCount <= 1 || table.size() < MIN_THREAD_REGION_SIZE) {
      if (condition.second) {
        for (auto it = table.begin(); it != table.end(); ++it) {
          if (this->evalCondition(*it)) {
            if (this->keyValue.empty()) {
              (*it)[this->fieldId] = this->fieldValue;
            } else {
              it->setKey(this->keyValue);
            }
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
        futures.emplace_back(pool.add_task(ThreadTaskUpdate, i, threadCount,
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

// std::string UpdateQuery::toString() {
//   return "QUERY = UPDATE " + this->getTargetTable() + "\"";
// }
