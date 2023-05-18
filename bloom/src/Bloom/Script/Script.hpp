#pragma once

#include <string>

#include <utl/vector.hpp>

#include "Bloom/Asset/Asset.hpp"
#include "Bloom/Core/Core.hpp"

namespace bloom {

BLOOM_API utl::vector<std::string> findClassNames(std::string_view script);

class BLOOM_API Script: public Asset {
public:
    using Asset::Asset;

    void setText(std::string);

    std::string text;
    utl::vector<std::string> classes;
};

} // namespace bloom
