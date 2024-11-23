#include "DuplicateQuery.h"
#include "../../db/Database.h"

constexpr const char *DuplicateQuery::qname;

std::string DuplicateQuery::toString() {
  return "QUERY = DUPLICATE " + this->targetTable + "\"";
}