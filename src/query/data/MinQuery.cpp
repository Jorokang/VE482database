#include "MinQuery.h"
#include "../Multithread.h"
#include "../../db/Database.h"
#include "../../db/Table.h"
#include "../QueryResult.h"
#include "../../utils/uexception.h"
#include <limits>

constexpr const char *MinQuery::qname;
extern Thread_pool pool;

void min_subtable(int id,
             unsigned int thread_num,
             Table &table,
             ComplexQuery &query,
             std::vector<std::string> &operands,
             std::vector<int> &min_values,
             std::pair<std::string, bool> &condition,
             unsigned int subtable_size,
             std::mutex &mut) 
{
    auto head = table.begin() + (id * (int)subtable_size);
    auto tail = (id == (int)thread_num - 1) ? table.end() : (head + (int)subtable_size);
    std::vector<int> sub_min_values(operands.size(), INT32_MAX);
    if (condition.second) {
        for (auto row = head; row != tail; ++row) {
            if (query.evalCondition(*row)) {
                for (size_t i = 0; i < operands.size(); i++) {
                    auto value = (*row)[operands[i]];
                        if (value < sub_min_values[i]) {
                            sub_min_values[i] = value;
                        }
                }
            }
        }
    }
    std::lock_guard<std::mutex> const lock(mut);
    for (size_t i = 0; i < operands.size(); i++) {
        if (sub_min_values[i] < min_values[i]) {
            min_values[i] = sub_min_values[i];
        }
    }
}

QueryResult::Ptr MinQuery::execute() {

    if (this->operands.empty()) {
        return std::make_unique<ErrorMsgResult>(
            qname, this->targetTable.c_str(),
            "Invalid number of operands (? operands)."_f % operands.size());
    }

    try {
        auto &db = Database::getInstance();
        auto &table = db[this->targetTable];
        auto condition = initCondition(table);
        bool found = false;
        std::mutex mut;
        std::vector<int> min_values(this->operands.size(), INT32_MAX);
        unsigned int thread_num = (unsigned int)pool.get_idle_thread_num();
        if (thread_num <= 1 || table.size() < 2000) {
            if (condition.second) {
                for (auto row = table.begin(); row != table.end(); ++row) {
                    if (this->evalCondition(*row)) {
                        found = true;
                        for (size_t i = 0; i < this->operands.size(); ++i) {
                            auto value = (*row)[this->operands[i]];
                            if (value < min_values[i]) {
                                min_values[i] = value;
                            }
                        }
                    }
                }
            }
            if (found) {
                return std::make_unique<SuccessMsgResult>(min_values, true);
            } else {
                return std::make_unique<NullQueryResult>();
            }
        } else {
            thread_num = std::min(thread_num, (unsigned int)(table.size() / 2000 + 1));
            unsigned int const subtable_size = (unsigned int)(table.size()) / thread_num;
            std::vector<std::future<void>> future_vector((unsigned long)thread_num);
            for (unsigned long i = 0; i < thread_num; ++i)
                future_vector[i] = pool.add_task(min_subtable, i, thread_num, std::ref(table), std::ref(*this),
                                                 std::ref(this->operands), std::ref(min_values), std::ref(condition),
                                                 subtable_size, std::ref(mut));
            for (unsigned long i = 0; i < thread_num; i++)
                future_vector[i].get();
            return std::make_unique<SuccessMsgResult>(min_values, true);
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

std::string MinQuery::toString() {
    return "QUERY = MIN " + this->targetTable + "\"";
}
