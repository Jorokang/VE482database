#include "SelectQuery.h"
#include "../Multithread.h"        // Ensure this includes Thread_pool and related components
#include "../../db/Database.h"
#include "../../db/Table.h"
#include "../QueryResult.h"
#include "../../utils/uexception.h"
#include <limits>
#include <future>
#include <mutex>
#include <sstream>
#include <algorithm>

constexpr const char *SelectQuery::qname;

extern Thread_pool pool;

void select_subtable(int id,
                    unsigned int thread_num,
                    Table &table,
                    SelectQuery &query,
                    std::vector<Table::FieldIndex> &targetFields,
                    std::vector<std::pair<std::string, std::string>> &local_selected,
                    std::pair<std::string, bool> &condition,
                    unsigned int subtable_size)
{
    auto head = table.begin() + (id * static_cast<int>(subtable_size));
    auto tail = (id == static_cast<int>(thread_num) - 1) ? table.end() : (head + static_cast<int>(subtable_size));

    if (condition.second) {
        for (auto row = head; row != tail; ++row) {
            if (query.evalCondition(*row)) {
                Table::KeyType const key = row->key();
                std::string values;
                for (auto &fieldId : targetFields) {
                    values += std::to_string(row->get(fieldId)) + " ";
                }
                local_selected.emplace_back(std::make_pair(key, values));
            }
        }
    }
}

QueryResult::Ptr SelectQuery::execute() {
    using namespace std;

    try {
        auto &db = Database::getInstance();
        auto &table = db[this->targetTable];
        auto condition = initCondition(table);

        vector<Table::FieldIndex> targetFields;
        targetFields.reserve(this->operands.size() - 1);
        for (auto it = this->operands.begin() + 1; it != this->operands.end(); ++it) {
            targetFields.push_back(table.getFieldIndex(*it));
        }

        unsigned int thread_num = static_cast<unsigned int>(pool.get_idle_thread_num());
        unsigned long const table_size = table.size();

        if (thread_num <= 1 || table_size < 2000) {
            // Single-threaded processing
            vector<pair<string, string>> selected;
            if (condition.second) {
                for (auto it = table.begin(); it != table.end(); ++it) {
                    if (this->evalCondition(*it)) {
                        Table::KeyType const key = it->key();
                        string values;
                        for (auto &fieldId : targetFields) {
                            values += to_string(it->get(fieldId)) + " ";
                        }
                        selected.emplace_back(make_pair(key, values));
                    }
                }
                sort(selected.begin(), selected.end(),
                     [](const auto &a, const auto &b) { return a.first < b.first; });
            }

            // Format the result
            ostringstream os;
            for (auto &entry : selected) {
                os << "( " << entry.first << " ";
                os << entry.second << ")";
                os << "\n";
            }

            if (!selected.empty()) {
                return make_unique<SuccessMsgResult>(os.str(), true);
            } else {
                return make_unique<NullQueryResult>();
            }
        } else {
            // Multi-threaded processing
            thread_num = min(thread_num, static_cast<unsigned int>(table_size / 2000 + 1));
            unsigned int const subtable_size = static_cast<unsigned int>(table_size) / thread_num;

            vector<future<void>> future_vector;
            future_vector.reserve(thread_num);

            vector<vector<pair<string, string>>> thread_selected(thread_num);

            for (unsigned long i = 0; i < thread_num; ++i) {
                future_vector.emplace_back(pool.add_task(select_subtable,
                                                         static_cast<int>(i),
                                                         thread_num,
                                                         std::ref(table),
                                                         std::ref(*this),
                                                         std::ref(targetFields),
                                                         std::ref(thread_selected[i]),
                                                         std::ref(condition),
                                                         subtable_size));
            }
            for (auto &fut : future_vector) {
                fut.get();
            }
            vector<pair<string, string>> selected;
            selected.reserve(table_size); // Reserve maximum possible size
            for (auto &thread_data : thread_selected) {
                selected.insert(selected.end(), thread_data.begin(), thread_data.end());
            }

            sort(selected.begin(), selected.end(),
                 [](const auto &a, const auto &b) { return a.first < b.first; });

            ostringstream os;
            for (auto &entry : selected) {
                os << "( " << entry.first << " ";
                os << entry.second << ")";
                os << "\n";
            }
            if (!selected.empty()) {
                return make_unique<SuccessMsgResult>(os.str(), true);
            } else {
                return make_unique<NullQueryResult>();
            }
        }

    } catch (const TableNameNotFound &e) {
        return make_unique<ErrorMsgResult>(qname, this->targetTable, "No such table."s);
    } catch (const IllFormedQueryCondition &e) {
        return make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
    } catch (const invalid_argument &e) {
        return make_unique<ErrorMsgResult>(qname, this->targetTable,
                                           "Unknown error '?'"_f % e.what());
    } catch (const exception &e) {
        return make_unique<ErrorMsgResult>(qname, this->targetTable,
                                           "Unknown error '?'"_f % e.what());
    }
}

std::string SelectQuery::toString() {
    return "QUERY = SELECT " + this->targetTable + "\"";
}
