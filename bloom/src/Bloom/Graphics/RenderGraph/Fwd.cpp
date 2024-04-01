#include "Bloom/Graphics/RenderGraph/Fwd.h"

#include <ostream>

using namespace bloom;
using namespace rg;

std::ostream& rg::operator<<(std::ostream& ostream, ResourceNodeType nodeType) {
    switch (nodeType) {
#define RG_RESOURCE_DEF(Name, ...)                                             \
    case ResourceNodeType::Name:                                               \
        return ostream << #Name;
#include "Bloom/Graphics/RenderGraph/ResourceNodes.def"
    default:
        return ostream << "(ResourceNodeType)" << (int)nodeType;
    }
}
