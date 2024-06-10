#ifndef BLOOM_MATERIALCOMPILER_FWD_H
#define BLOOM_MATERIALCOMPILER_FWD_H

#include <string>
#include <string_view>

#include "Bloom/Core/Dyncast.h"

namespace bloom::matc {

class MaterialGraph;
class Context;

#define BL_MATGRAPH_TYPE(Name, ...) class Name;
#include "Bloom/MaterialCompiler/Lists.def"

enum class TypeCategory {
#define BL_MATGRAPH_TYPE(Name, ...) Name,
#include "Bloom/MaterialCompiler/Lists.def"
    LAST = FunctionType
};

#define BL_MATGRAPH_VALUE(Name, ...) class Name;
#include "Bloom/MaterialCompiler/Lists.def"

enum class ValueNodeType {
#define BL_MATGRAPH_VALUE(Name, ...) Name,
#include "Bloom/MaterialCompiler/Lists.def"
    LAST = ReturnInst
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

enum class ShaderKind { None, Extern, Vertex, Fragment };

struct ParameterAttribute {
    enum Type { None, Position, StageIn, Color, Buffer, Texture };

    Type type;
    int index;

    ParameterAttribute(Type type = None, int index = -1):
        type(type), index(index) {}

    explicit operator bool() const { return type != None; }
};

enum class ParameterQualifier { None, DeviceConstRef, DeviceConstPointer };

struct AggregateElement {
    Type const* type;
    std::string name;
    ParameterAttribute attrib = {};
};

/// Different kinds of functions that can be defined by material nodes
enum class GraphRoot {
    VertexFunction,
    ShadingFunction,
    TransparencyFunction,
};

enum class TextureSubType { Texture2D, TextureCube, Texture2DArray };

enum class RasterizerInputKind {
    ClipPosition,
    TextureCoords,
    WorldNormals,
    WorldTangents,
    WorldBinormals,
    ModelNormals,
    WorldPosition,
    VertexColor,
};

std::string_view toSpelling(RasterizerInputKind kind);

enum class BuiltinFunctionKind {
#define BL_MATGRAPH_BUILTIN(Name, ...) Name,
#include "Bloom/MaterialCompiler/Lists.def"
};

std::string_view toSpelling(BuiltinFunctionKind function);

enum class SampleMode { Color, Normal };

} // namespace bloom::matc

#endif // BLOOM_MATERIALCOMPILER_FWD_H
