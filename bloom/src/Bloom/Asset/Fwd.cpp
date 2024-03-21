#include "Bloom/Asset/Fwd.h"

#include "Bloom/Core/Debug.h"

using namespace bloom;

utl::uuid bloom::toUUID(std::string_view str) {
    char const* const begin = str.data();
    std::size_t const offset = str.size() / 2;
    struct {
        std::size_t first, second;
    } values = { utl::hash_string(std::string_view(begin, offset)),
                 utl::hash_string(
                     std::string_view(begin + offset, str.size() - offset)) };
    return utl::uuid::construct_from_value(
        utl::bit_cast<utl::uuid::value_type>(values));
}

std::string bloom::toString(AssetType type) {
    switch (type) {
#define BLOOM_ASSET_TYPE_DEF(Name, ...)                                        \
    case AssetType::Name:                                                      \
        return #Name;
#include "Bloom/Asset/Assets.def"
    }
    return "InvalidAsset";
}

std::ostream& bloom::operator<<(std::ostream& str, AssetType type) {
    return str << toString(type);
}
