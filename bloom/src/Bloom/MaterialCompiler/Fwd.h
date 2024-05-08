#ifndef BLOOM_GRAPHICS_MATERIAL_FWD_H
#define BLOOM_GRAPHICS_MATERIAL_FWD_H

#include "Bloom/Core/Dyncast.h"

namespace bloom::matgraph {

class MaterialGraph;
class Context;

#define BL_MATGRAPH_TYPE(Name, ...) class Name;
#include "Bloom/MaterialCompiler/Lists.def"

enum class TypeCategory {
#define BL_MATGRAPH_TYPE(Name, ...) Name,
#include "Bloom/MaterialCompiler/Lists.def"
    LAST = AggregateType
};

#define BL_MATGRAPH_VALUE(Name, ...) class Name;
#include "Bloom/MaterialCompiler/Lists.def"

enum class ValueNodeType {
#define BL_MATGRAPH_VALUE(Name, ...) Name,
#include "Bloom/MaterialCompiler/Lists.def"
    LAST = AggregateBuilder
};

using NoParent = void;

} // namespace bloom::matgraph

#define BL_MATGRAPH_TYPE(Name, Parent, Corporeality)                           \
    UTL_DYNCAST_DEFINE(bloom::matgraph::Name,                                  \
                       bloom::matgraph::TypeCategory::Name,                    \
                       bloom::matgraph::Parent, Corporeality)
#include "Bloom/MaterialCompiler/Lists.def"

#define BL_MATGRAPH_VALUE(Name, Parent, Corporeality)                          \
    UTL_DYNCAST_DEFINE(bloom::matgraph::Name,                                  \
                       bloom::matgraph::ValueNodeType::Name,                   \
                       bloom::matgraph::Parent, Corporeality)
#include "Bloom/MaterialCompiler/Lists.def"

#endif // BLOOM_GRAPHICS_MATERIAL_FWD_H
