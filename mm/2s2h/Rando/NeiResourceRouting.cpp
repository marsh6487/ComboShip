#include "NeiResourceRouting.h"
#include "ComboResolve.h"
#include "ComboItemDrawABI.h"
#include <cstring>
#include <string>
#include <unordered_set>

extern "C" int NeiResource_Available(const char* path) {
    if (!path || std::strncmp(path, "__OTR__", 7) != 0)
        return 0;
    static Fn_NeiResourceExists exists = nullptr;
    if (!exists)
        exists = reinterpret_cast<Fn_NeiResourceExists>(Combo_ResolveSym("soh", "OOT_NeiResourceExists"));
    return exists && exists(path);
}

extern "C" const char* NeiResource_Route(const char* path) {
    if (!path || std::strncmp(path, "__OTR__", 7) != 0)
        return nullptr;
    // unordered_set rehash preserves references/pointers to its elements.
    static std::unordered_set<std::string> paths;
    return paths.insert(std::string("__OTR__@oot:") + (path + 7)).first->c_str();
}
