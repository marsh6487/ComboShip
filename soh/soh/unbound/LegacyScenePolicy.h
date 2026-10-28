#pragma once

namespace SOH::Unbound {
// A complete legacy scene graph remains effective when its winning structured
// resource is above the JSON scene layers. At equal priority an archive that
// deliberately supplies both formats selects its Unbound documents. Bulk mesh
// and texture replacements do not contribute a structured legacy priority.
constexpr bool UseLegacyScene(bool hasUnboundBase, int legacyPriority, int jsonPriority) {
    return !hasUnboundBase || legacyPriority > jsonPriority;
}
} // namespace SOH::Unbound
