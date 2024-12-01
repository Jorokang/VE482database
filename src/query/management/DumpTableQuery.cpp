//
// Created by liu on 18-10-25.
//

#include <fstream>
#include <memory>
#include <string>

#include "../../db/Database.h"
#include "DumpTableQuery.h"

// constexpr const char *DumpTableQuery::qname;

QueryResult::Ptr DumpTableQuery::execute() {
  using std::exception;
  using std::make_unique;
  using std::ofstream;

  const auto &db = Database::getInstance();
  try {
    ofstream outfile(this->fileName);
    if (!outfile.is_open()) {
      return make_unique<ErrorMsgResult>(qname, "Cannot open file '?'"_f %
                                                    this->fileName);
    }
    outfile << db[this->getTargetTable()];
    outfile.close();
    return make_unique<SuccessMsgResult>(qname, this->getTargetTable());
  } catch (const exception &e) {
    return make_unique<ErrorMsgResult>(qname, e.what());
  }
}

// std::string DumpTableQuery::toString() {
//   return "QUERY = Dump TABLE, FILE = \"" + this->fileName + "\"";
// }
