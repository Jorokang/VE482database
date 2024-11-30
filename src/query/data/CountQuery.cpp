#include <iostream>
#include <memory>
#include <string>

#include "../Multithread.h"
#include "../../db/Database.h"
#include "../../db/Table.h"
#include "../../utils/uexception.h"
#include "../QueryResult.h"
#include "CountQuery.h"

extern Thread_pool pool;

void count_subtable(int id,
             unsigned int thread_num,
             Table &table,
             ComplexQuery &query,
             size_t &count,
             std::pair<std::string, bool> &condition,
             unsigned int subtable_size,
             std::mutex &mut) 
{
    auto head = table.begin() + (id * (int)subtable_size);
    auto tail = (id == (int)thread_num - 1) ? table.end() : (head + (int)subtable_size);
    size_t sub_count = 0;
    if (condition.second) {
        for (auto row = head; row != tail; ++row) {
            if (query.evalCondition(*row)) {
                ++sub_count;
            }
        }
    }
    std::lock_guard<std::mutex> const lock(mut);
    count += sub_count;
}

std::string CountQuery::toString() {
  return "QUERY = Count " + this->getTargetTable() + "\"";
}

QueryResult::Ptr CountQuery::execute() {
  try {
    auto &db = Database::getInstance();
    auto &table = db[this->getTargetTable()];
    std::mutex mut;
    std::pair<std::string, bool> condition = initCondition(table);
    size_t count = 0;
    unsigned int thread_num = (unsigned int)pool.get_idle_thread_num();
    if (thread_num <= 1 || table.size() < 2000) {
      if (condition.second) {
        for (auto row = table.begin(); row != table.end(); row++) {
          if (this->evalCondition(*row)) {
            ++count;
          }
        }
      }
      return std::make_unique<SuccessMsgResult>(count, true);
    } else {

      thread_num = std::min(thread_num, (unsigned int)(table.size() / 2000 + 1));
      unsigned int const subtable_size = (unsigned int)(table.size()) / thread_num;
      std::vector<std::future<void>> future_vector((unsigned long)thread_num);
      for (int i = 0; i < (int)thread_num; i++)
        future_vector[i] = pool.add_task(count_subtable, i, thread_num, std::ref(table), std::ref(*this),
                                         std::ref(count), std::ref(condition),
                                         subtable_size, std::ref(mut));
      for (int i = 0; i < (int)thread_num; i++) 
        future_vector[i].get();
      return std::make_unique<SuccessMsgResult>(count, true);

    } 
  } catch (const TableNameNotFound &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->getTargetTable(),
                                            "No such table.");
  } catch (const TableFieldNotFound &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->getTargetTable(),
                                            e.what());
  } catch (const std::exception &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->getTargetTable(),
                                            "Unknown error '?'"_f % e.what());
  }
}
