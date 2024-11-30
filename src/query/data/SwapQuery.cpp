#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "../../db/Database.h"
#include "../Multithread.h"
#include "SwapQuery.h"

extern Thread_pool pool;

void ThreadTaskSwap(int threadId, unsigned int threadCount, Table *table,
                    SwapQuery *query, size_t *globalCounter,
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
        auto &field1 = (*it)[query->getFieldId1()];
        auto &field2 = (*it)[query->getFieldId2()];
        std::swap(field1, field2);
        ++localCounter;
      }
    }
  }

  std::lock_guard<std::mutex> const lock(*mutex);
  *globalCounter += localCounter;
}

QueryResult::Ptr SwapQuery::execute() {
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
    this->fieldId1 = table.getFieldIndex(this->getOperands()[0]);
    this->fieldId2 = table.getFieldIndex(this->getOperands()[1]);
    unsigned int threadCount =
        static_cast<unsigned int>(pool.get_idle_thread_num());
    auto condition = initCondition(table);

    if (threadCount <= 1 || table.size() < MIN_THREAD_REGION_SIZE) {
      if (condition.second) {
        for (auto it = table.begin(); it != table.end(); ++it) {
          if (this->evalCondition(*it)) {
            auto &field1 = (*it)[this->fieldId1];
            auto &field2 = (*it)[this->fieldId2];
            std::swap(field1, field2);
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
        futures.emplace_back(pool.add_task(ThreadTaskSwap, i, threadCount,
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

std::string SwapQuery::toString() {
  return "QUERY = SWAP " + this->getTargetTable() + "\"";
}
