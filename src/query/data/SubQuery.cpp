#include "SubQuery.h"
#include "../../db/Database.h"
#include "../Multithread.h"

constexpr const char *SubQuery::qname;
extern Thread_pool pool;

void ThreadTaskSub(int ThreadInd,
             unsigned int ThreadNum,
             Table &table,
             SubQuery &query,
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
                auto &dest = (*it)[query.getDestFieldId()];
                dest = (*it)[query.getSrcFieldId()];
                for (auto &fieldId : query.getFieldIds()){
                    dest -= (*it)[fieldId];
                }
                ++local_counter;
            }
        }
    }
    std::lock_guard<std::mutex> const lock(mut);
    counter += local_counter;
}

void ThreadTaskEqual(int ThreadInd,
             unsigned int ThreadNum,
             Table &table,
             SubQuery &query,
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
                auto &dest = (*it)[query.getDestFieldId()];
                dest = (*it)[query.getSrcFieldId()];
                ++local_counter;
            }
        }
    }
    std::lock_guard<std::mutex> const lock(mut);
    counter += local_counter;
}

QueryResult::Ptr SubQuery::execute(){
    using namespace std;
    auto opcount = this->operands.size();
    if (opcount < 2)
        return std::make_unique<ErrorMsgResult>(
            qname, this->targetTable.c_str(),
            "Invalid number of operands (? operands)."_f % operands.size());
    Database &db = Database::getInstance();
    Table::SizeType counter = 0;
    std::mutex mut;
    unsigned int thread_num = (unsigned int)pool.get_idle_thread_num();
    try{
        auto &table = db[this->targetTable];
        this->destFieldId = table.getFieldIndex(this->operands[opcount - 1]);
        this->srcFieldId = table.getFieldIndex(this->operands[0]);
        if (thread_num == 1 || table.size() < 2000){ // signle thread
            if (opcount == 2){
                auto result = initCondition(table);
                if (result.second){
                    for (auto it = table.begin(); it != table.end(); ++it){
                        if (this->evalCondition(*it)){
                            auto &dest = (*it)[this->destFieldId];
                            dest = (*it)[this->srcFieldId];
                            ++counter;
                        }
                    }
                }
            } else {
                this->fieldIds.reserve(opcount - 2);
                for (auto it = this->operands.begin() + 1; it != this->operands.end() - 1; ++it){
                    this->fieldIds.push_back(table.getFieldIndex(*it));
                }
                auto result = initCondition(table);
                if (result.second){
                    for (auto it = table.begin(); it != table.end(); ++it){
                        if (this->evalCondition(*it)){
                            auto &dest = (*it)[this->destFieldId];
                            dest = (*it)[this->srcFieldId];
                            for (auto &fieldId : this->fieldIds){
                                dest -= (*it)[fieldId];
                            }
                            ++counter;
                        }
                    }
                }
            }
        } else {
            thread_num = std::min(thread_num, (unsigned int)(table.size() / MIN_THREAD_REGION_SIZE + 1));
            unsigned int const RegionSize = (unsigned int)(table.size()) / thread_num;
            std::vector<std::future<void>> future_vector((unsigned long)thread_num);
            auto result = initCondition(table);
            if (opcount == 2)
                for (unsigned long i = 0; i < thread_num; i++)
                    future_vector[i] = pool.add_task(ThreadTaskEqual, i, thread_num, std::ref(table), std::ref(*this),
                                                     std::ref(counter), std::ref(result), RegionSize, std::ref(mut));
            else
                for (unsigned long i = 0; i < thread_num; i++)
                    future_vector[i] = pool.add_task(ThreadTaskSub, i, thread_num, std::ref(table), std::ref(*this),
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

std::string SubQuery::toString(){
    return "QUERY = SUB " + this->targetTable + "\"";
}