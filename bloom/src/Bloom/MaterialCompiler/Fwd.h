#ifndef BLOOM_MATERIALCOMPILER_FWD_H
#define BLOOM_MATERIALCOMPILER_FWD_H

#include "Bloom/Core/Dyncast.h"

namespace bloom::matc {

class MaterialGraph;
class Context;

#define BL_MATGRAPH_TYPE(Name, ...) class Name;
#include "Bloom/MaterialCompiler/Lists.def"

enum class TypeCategory {
#define BL_MATGRAPH_TYPE(Name, ...) Name,
#include "Bloom/MaterialCompiler/Lists.def"
    LAST = TextureType
};

#define BL_MATGRAPH_VALUE(Name, ...) class Name;
#include "Bloom/MaterialCompiler/Lists.def"

enum class ValueNodeType {
#define BL_MATGRAPH_VALUE(Name, ...) Name,
#include "Bloom/MaterialCompiler/Lists.def"
    LAST = AggregateElement
};

using NoParent = void;

} // namespace bloom::matc

#define BL_MATGRAPH_TYPE(Name, Parent, Corporeality)                           \
    UTL_DYNCAST_DEFINE(bloom::matc::Name, bloom::matc::TypeCategory::Name,     \
                       bloom::matc::Parent, Corporeality)
#include "Bloom/MaterialCompiler/Lists.def"

#define BL_MATGRAPH_VALUE(Name, Parent, Corporeality)                          \
    UTL_DYNCAST_DEFINE(bloom::matc::Name, bloom::matc::ValueNodeType::Name,    \
                       bloom::matc::Parent, Corporeality)
#include "Bloom/MaterialCompiler/Lists.def"

namespace bloom::matc {

enum class TextureSubType { Texture2D, TextureCube };

enum class RasterizerInputKind {
    TextureCoords,
    VertexNormals,
    VertexTangents,
    VertexBinormals,
    WorldPosition,
};

enum class BuiltinFunctionKind { NormalizeFloat3 };

enum class SampleMode { Color, Normal };

} // namespace bloom::matc

#endif // BLOOM_MATERIALCOMPILER_FWD_H
