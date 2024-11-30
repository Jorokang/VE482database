#include <iostream>
#include <memory>
#include <string>

#include "../Multithread.h"
#include "../../db/Database.h"
#include "../../db/Table.h"
#include "../../utils/uexception.h"
#include "../QueryResult.h"
#include "MaxQuery.h"

extern Thread_pool pool;

void max_subtable(int id,
             unsigned int thread_num,
             Table &table,
             ComplexQuery &query,
             const std::vector<std::string> &operands,
             std::vector<int> &max_values,
             std::pair<std::string, bool> &condition,
             unsigned int subtable_size,
             std::mutex &mut,
             bool &found) 
{
    auto head = table.begin() + (id * (int)subtable_size);
    auto tail = (id == (int)thread_num - 1) ? table.end() : (head + (int)subtable_size);
    std::vector<int> sub_max_values(operands.size(), INT32_MIN);
    bool local_found = false;
    if (condition.second) {
        for (auto row = head; row != tail; ++row) {
            if (query.evalCondition(*row)) {
                local_found = true;
                for (size_t i = 0; i < operands.size(); i++) {
                    auto value = (*row)[operands[i]];
                    if (value > max_values[i]) {
                      max_values[i] = value;
                    }
                }
            }
        }
    }
    std::lock_guard<std::mutex> const lock(mut);
    if (local_found) {
        found = true;
    }
    for (size_t i = 0; i < operands.size(); i++) {
        if (sub_max_values[i] > max_values[i]) {
            max_values[i] = sub_max_values[i];
        }
    }
}

std::string MaxQuery::toString() {
  return "QUERY = Max " + this->getTargetTable() + "\"";
}

QueryResult::Ptr MaxQuery::execute() {
  if (this->getOperands().empty()) {
    return std::make_unique<ErrorMsgResult>(
        qname, this->getTargetTable().c_str(),
        "Invalid number of this->getOperands() (? this->getOperands())."_f %
            getOperands().size());
  }

  try {
    auto &db = Database::getInstance();
    auto &table = db[this->getTargetTable()];
    bool found = false;
    std::mutex mut;
    std::pair<std::string, bool> condition = initCondition(table);
    std::vector<int> max_values(this->getOperands().size(), INT32_MIN);
    unsigned int thread_num = (unsigned int)pool.get_idle_thread_num();
    if (thread_num <= 1 || table.size() < 2000) {
      if (condition.second) {
        for (auto row = table.begin(); row != table.end(); row++) {
          if (this->evalCondition(*row)) {
            found = true;
            for (size_t i = 0; i < this->getOperands().size(); ++i) {
              auto value = (*row)[this->getOperands()[i]];
              if (value > max_values[i]) {
                max_values[i] = value;
              }
            }
          }
        }
      }
      if (found) {
        return std::make_unique<SuccessMsgResult>(max_values, true);
      } else {
        return std::make_unique<NullQueryResult>();
      }
    } else {

      thread_num = std::min(thread_num, (unsigned int)(table.size() / 2000 + 1));
      unsigned int const subtable_size = (unsigned int)(table.size()) / thread_num;
      std::vector<std::future<void>> future_vector((unsigned long)thread_num);
      for (int i = 0; i < (int)thread_num; i++)
        future_vector[i] = pool.add_task(max_subtable, i, thread_num, std::ref(table), std::ref(*this),
                                         std::ref(this->getOperands()), std::ref(max_values), std::ref(condition),
                                         subtable_size, std::ref(mut), std::ref(found));
      for (int i = 0; i < (int)thread_num; i++) 
        future_vector[i].get();
        //actually, we don't need the return value of the get() but this can help us to wait all thread to finish
      if (found) {
        return std::make_unique<SuccessMsgResult>(max_values, true);
      } else {
        return std::make_unique<NullQueryResult>();
      }

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
