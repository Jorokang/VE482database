#include "SwapQuery.h"
#include "../../db/Database.h"
#include "../Multithread.h"

constexpr const char *SwapQuery::qname;
extern Thread_pool pool;

void ThreadTaskSwap(int ThreadInd,
             unsigned int ThreadNum,
             Table &table,
             SwapQuery &query,
             size_t &counter,
             std::pair<std::string, bool> &result,
             unsigned int RegionSize,
             std::mutex &mut){
    auto head = table.begin() + (ThreadInd * (int)RegionSize);
    auto tail = (ThreadInd == (int)ThreadNum - 1) ? table.end() : (head + (int)RegionSize);
    size_t local_counter = 0;
    if (result.second){
        for (auto it = head; it != tail; ++it){
            if (query.evalCondition(*it)){
                auto &field1 = (*it)[query.getFieldId1()];
                auto &field2 = (*it)[query.getFieldId2()];
                std::swap(field1, field2);
                ++local_counter;
            }
        }
    }
    std::lock_guard<std::mutex> const lock(mut);
    counter += local_counter;
}

QueryResult::Ptr SwapQuery::execute(){
    using namespace std;
    auto opcount = this->operands.size();
    if (opcount != 2)
        return std::make_unique<ErrorMsgResult>(
            qname, this->targetTable.c_str(),
            "Invalid number of operands (? operands)."_f % operands.size());
    Database &db = Database::getInstance();
    Table::SizeType counter = 0;
    std::mutex mut;
    unsigned int thread_num = (unsigned int)pool.get_idle_thread_num();
    try {
        auto &table = db[this->targetTable];
        this->fieldId1 = table.getFieldIndex(this->operands[0]);
        this->fieldId2 = table.getFieldIndex(this->operands[1]);
        if (thread_num == 1 || table.size() < 2000) { // signle thread
            auto result = initCondition(table);
            if (result.second) {
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
            thread_num = std::min(thread_num, (unsigned int)(table.size() / MIN_THREAD_REGION_SIZE + 1));
            unsigned int const RegionSize = (unsigned int)(table.size()) / thread_num;
            std::vector<std::future<void>> future_vector((unsigned long)thread_num);
            auto result = initCondition(table);
            for (unsigned long i = 0; i < thread_num; i++)
                    future_vector[i] = pool.add_task(ThreadTaskSwap, i, thread_num, std::ref(table), std::ref(*this),
                                                    std::ref(counter), std::ref(result), RegionSize, std::ref(mut));
            for (unsigned long i = 0; i < thread_num; i++) 
                future_vector[i].get();
        }
        return make_unique<RecordCountResult>(counter);
    } catch (const TableNameNotFound &e){
        return make_unique<ErrorMsgResult>(qname, this->targetTable, "No such table."s);
    } catch (const IllFormedQueryCondition &e){
        return make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
    } catch (const invalid_argument &e){
        return make_unique<ErrorMsgResult>(qname, this->targetTable, "Unknown error '?'"_f % e.what());
    } catch (const exception &e){
        return make_unique<ErrorMsgResult>(qname, this->targetTable, "Unkonwn error '?'."_f % e.what());
    }
}

std::string SwapQuery::toString(){
    return "QUERY = SWAP " + this->targetTable + "\"";
}
