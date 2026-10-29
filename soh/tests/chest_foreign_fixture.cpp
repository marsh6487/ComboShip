#include <cstdint>
#include <string>
#include "soh/Enhancements/item-tables/ItemTableTypes.h"

// Execute the unchanged foreign-category mapping from hook_handlers.cpp.
#include "chest_foreign_category_production.inc"

extern "C" GetItemCategory ChestTest_ForeignCategory(const char* name) {
    return ForeignCategoryFromName(name);
}
