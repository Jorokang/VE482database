#include "SumQuery.h"
#include "../Multithread.h"
#include "../../db/Database.h"
#include "../../db/Table.h"
#include "../QueryResult.h"
#include "../../utils/uexception.h"

constexpr const char *SumQuery::qname;
extern Thread_pool pool;

void sum_subtable(int id,
             unsigned int thread_num,
             Table &table,
             ComplexQuery &query,
             std::vector<std::string> &operands,
             std::vector<int> &sum_values,
             std::pair<std::string, bool> &condition,
             unsigned int subtable_size,
             std::mutex &mut) 
{
    auto head = table.begin() + (id * (int)subtable_size);
    auto tail = (id == (int)thread_num - 1) ? table.end() : (head + (int)subtable_size);
    std::vector<int> sub_sum_values(operands.size(), 0);
    if (condition.second) {
        for (auto row = head; row != tail; ++row) {
            if (query.evalCondition(*row)) {
                for (size_t i = 0; i < operands.size(); i++) {
                    sub_sum_values[i] += (*row)[operands[i]];
                }
            }
        }
    }
    std::lock_guard<std::mutex> const lock(mut);
    for (size_t i = 0; i < operands.size(); i++) {
        sum_values[i] += sub_sum_values[i];
    }
}

QueryResult::Ptr SumQuery::execute() {
    if (this->operands.empty()) {
        return std::make_unique<ErrorMsgResult>(
            qname, this->targetTable.c_str(),
            "Invalid number of operands (? operands)."_f % operands.size());
    }

    try {
        auto &db = Database::getInstance();
        auto &table = db[this->targetTable];
        std::mutex mut;
        std::pair<std::string, bool> condition = initCondition(table);
        std::vector<int> sum_values(this->operands.size(), 0);
        unsigned int thread_num = (unsigned int)pool.get_idle_thread_num();
        if (thread_num <= 1 || table.size() < 2000) {
            if (condition.second) {
                for (auto row = table.begin(); row != table.end(); row++) {
                    if (this->evalCondition(*row)) {
                        for (size_t i = 0; i < this->operands.size(); ++i) {
                            auto value = (*row)[this->operands[i]];
                            sum_values[i] += value;
                        }
                    }
                }
            }
            return std::make_unique<SuccessMsgResult>(sum_values, true);
        } else {

            thread_num = std::min(thread_num, (unsigned int)(table.size() / 2000 + 1));
            unsigned int const subtable_size = (unsigned int)(table.size()) / thread_num;
            std::vector<std::future<void>> future_vector((unsigned long)thread_num);
            for (int i = 0; i < (int)thread_num; i++)
                future_vector[i] = pool.add_task(sum_subtable, i, thread_num, std::ref(table), std::ref(*this),
                                                 std::ref(this->operands), std::ref(sum_values), std::ref(condition),
                                                 subtable_size, std::ref(mut));
            for (int i = 0; i < (int)thread_num; i++) 
                future_vector[i].get();
                //actually, we don't need the return value of the get() but this can help us to wait all thread to finish
            return std::make_unique<SuccessMsgResult>(sum_values, true);

        } 
    } catch (const TableNameNotFound &e) {
        return std::make_unique<ErrorMsgResult>(qname, this->targetTable, "No such table.");
    } catch (const TableFieldNotFound &e) {
            return std::make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
    } catch (const std::exception &e) {
        return std::make_unique<ErrorMsgResult>(qname, this->targetTable,
                                                "Unknown error '?'"_f % e.what());
    }

}

std::string SumQuery::toString() {
  return "QUERY = SUM " + this->targetTable + "\"";
}

