#include "Bloom/Graphics/RenderGraph/Fwd.h"

#include <ostream>

using namespace bloom;

std::ostream& bloom::operator<<(std::ostream& ostream,
                                ResourceNodeType nodeType) {
    switch (nodeType) {
#define RG_RESOURCE_DEF(Name, ...)                                             \
    case ResourceNodeType::Name:                                               \
        return ostream << #Name;
#include "Bloom/Graphics/RenderGraph/Resource.def"
    default:
        return ostream << "(ResourceNodeType)" << (int)nodeType;
    }
}
