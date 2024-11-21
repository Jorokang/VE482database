#include "SubQuery.h"
#include "../../db/Database.h"

constexpr const char *SubQuery::qname;

QueryResult::Ptr SubQuery::execute(){
    using namespace std;
    auto opcount = this->operands.size();
    if (opcount < 2)
        return std::make_unique<ErrorMsgResult>(
            qname, this->targetTable.c_str(),
            "Invalid number of operands (? operands)."_f % operands.size());
    Database &db = Database::getInstance();
    Table::SizeType counter = 0;
    try{
        auto &table = db[this->targetTable];
        this->destFieldId = table.getFieldIndex(this->operands[opcount - 1]);
        this->srcFieldId = table.getFieldIndex(this->operands[0]);
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