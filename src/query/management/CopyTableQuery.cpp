#include "CopyTableQuery.h"
#include "../../db/Database.h"
#include "../../db/Table.h"
#include "../QueryResult.h"
#include "../../utils/uexception.h"
#include <limits>

constexpr const char *CopyTableQuery::qname;

QueryResult::Ptr CopyTableQuery::execute() {
    try {
        Database &db = Database::getInstance();
        auto table = db[this->targetTable];
        db.registerTable(std::make_unique<Table>(newtable, table));
        return std::make_unique<NullQueryResult>();
    } catch (const TableNameNotFound &e) {
        return std::make_unique<ErrorMsgResult>(qname, this->targetTable, "No such table.");
    } catch (const TableFieldNotFound &e) {
        return std::make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
    } catch (const std::exception &e) {
        return std::make_unique<ErrorMsgResult>(qname, this->targetTable,
                                                "Unknown error '?'"_f % e.what());
    }
}

std::string CopyTableQuery::toString() {
    return "QUERY = COPYTABLE from \"" + this->targetTable + "\" to \"" +
         this->newtable + "\"";
}