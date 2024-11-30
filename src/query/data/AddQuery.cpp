#include <exception>
#include <memory>
#include <stdexcept>
#include <string>

#include "AddQuery.h"
#include "../../db/Database.h"
#include "../Multithread.h"

// constexpr const char *AddQuery::qname;
extern Thread_pool pool;

void ThreadTaskSum(int ThreadInd,
    unsigned int ThreadNum,
    Table& table,
    AddQuery& query,
    size_t& counter,
    std::pair<std::string, bool>& result,
    unsigned int RegionSize,
    std::mutex& mut) {
    auto head = table.begin() + (ThreadInd * (int)RegionSize);
    auto tail = (ThreadInd == (int)ThreadNum - 1) ? table.end() : (head + (int)RegionSize);
    size_t local_counter = 0;
    if (result.second) {
        for (auto it = head; it != tail; ++it) {
            if (query.evalCondition(*it)) {
                auto& dest = (*it)[query.getDestFieldId()];
                auto sum = 0;
                for (auto& fieldId : query.getFieldIds()) {
                    sum += (*it)[fieldId];
                }
                dest = sum;
                ++local_counter;
            }
        }
    }
    std::lock_guard<std::mutex> const lock(mut);
    counter += local_counter;
}

QueryResult::Ptr AddQuery::execute() {
    using std::exception;
    using std::invalid_argument;
    using std::make_unique;
    using std::string;
    auto opcount = this->getOperands().size();
    if (opcount < 2)
        return std::make_unique<ErrorMsgResult>(
            qname, this->getTargetTable().c_str(),
            "Invalid number of this->getOperands() (? this->getOperands())."_f %
            this->getOperands().size());
    Database& db = Database::getInstance();
    Table::SizeType counter = 0;
    try {
        auto& table = db[this->getTargetTable()];
        this->destFieldId = table.getFieldIndex(this->getOperands()[opcount - 1]);
        this->fieldIds.reserve(opcount - 1);
        for (auto it = this->getOperands().begin(); it != this->getOperands().end() - 1; ++it) {
            this->fieldIds.push_back(table.getFieldIndex(*it));
        }
        std::mutex mut;
        auto result = initCondition(table);
        unsigned int thread_num = (unsigned int)pool.get_idle_thread_num();
        if (thread_num == 1 || table.size() < 2000) { // signle thread
            if (result.second) {
                for (auto it = table.begin(); it != table.end(); ++it) {
                    if (this->evalCondition(*it)) {
                        auto& dest = (*it)[this->destFieldId];
                        auto sum = 0;
                        for (auto& fieldId : this->fieldIds) {
                            sum += (*it)[fieldId];
                        }
                        dest = sum;
                        ++counter;
                    }
                }
            }
        }
        else {
            thread_num = std::min(thread_num, (unsigned int)(table.size() / MIN_THREAD_REGION_SIZE + 1));
            unsigned int const RegionSize = (unsigned int)(table.size()) / thread_num;
            std::vector<std::future<void>> future_vector((unsigned long)thread_num);
            for (unsigned long i = 0; i < thread_num; i++)
                future_vector[i] = pool.add_task(ThreadTaskSum, i, thread_num, std::ref(table), std::ref(*this),
                    std::ref(counter), std::ref(result), RegionSize, std::ref(mut));
            for (unsigned long i = 0; i < thread_num; i++)
                future_vector[i].get();
        }
        return make_unique<RecordCountResult>(counter);
    } catch (const TableNameNotFound& e) {
        return make_unique<ErrorMsgResult>(qname, this->getTargetTable(), "No such table."s);
    } catch (const IllFormedQueryCondition& e) {
        return make_unique<ErrorMsgResult>(qname, this->getTargetTable(), e.what());
    } catch (const invalid_argument& e) {
        return make_unique<ErrorMsgResult>(qname, this->getTargetTable(), "Unknown error '?'"_f % e.what());
    } catch (const exception& e) {
        return make_unique<ErrorMsgResult>(qname, this->getTargetTable(), "Unkonwn error '?'."_f % e.what());
    }
}

std::string AddQuery::toString() {
    return "QUERY = ADD " + this->getTargetTable() + "\"";
}
