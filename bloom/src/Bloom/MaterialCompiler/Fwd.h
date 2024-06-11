#ifndef BLOOM_MATERIALCOMPILER_FWD_H
#define BLOOM_MATERIALCOMPILER_FWD_H

#include <string>
#include <string_view>

#include <utl/common.hpp>

#include "Bloom/Core/Dyncast.h"
#include "Bloom/Graphics/Material/Fwd.h"

namespace bloom::matc {

class Context;
class MaterialGraph;
class Module;

#define BL_MATGRAPH_TYPE(Name, ...) class Name;
#include "Bloom/MaterialCompiler/Lists.def"

enum class TypeCategory {
#define BL_MATGRAPH_TYPE(Name, ...) Name,
#include "Bloom/MaterialCompiler/Lists.def"
    LAST = ArrayType
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

/// Symbolic types used internally
enum class SymbolicType {
    Float,  /// Single floating point value
    Float2, /// Vector of two floating point values
    Float3, /// Vector of three floating point values
    Float4, /// Vector of four floating point values
    FloatN, /// Single floating point value or vector of 2-4 floating point
    /// values
    VectorN /// Vector of 2-4 floating point values
};

/// \Returns the actual type corresponding to \p symType or null if no type can
/// be mapped
Type const* toConcreteType(Context& ctx, SymbolicType symType);

/// List of values in a default lit material
enum class DefaultLitMaterialValue {
#define BL_MATGRAPH_DEFLITVALUE(Name, Type) Name,
#include "Bloom/MaterialCompiler/Lists.def"
};

/// \Returns the type of the material value \p value
Type const* getType(Context& ctx, DefaultLitMaterialValue value);

///
Type const* makeShadingFunctionReturnType(Context& ctx,
                                          MaterialShadingModel model);

struct CodegenParameters {
    MaterialShadingModel shadingModel = {};
    MaterialTransparency transparency = {};
    bool twoSided = false;
};

enum class Mutability { Mutable, Const };

enum class IntSubType { I8, I16, I32, U8, U16, U32 };

enum class ShaderKind { None, Extern, Vertex, Fragment };

struct ParameterAttribute {
    enum Type {
        None,
        VertexID,
        InstanceID,
        Position,
        StageIn,
        Color,
        Buffer,
        Texture
    };

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
#define BL_MATGRAPH_VERTEXATTRIB(Name, Type) Name,
#include "Bloom/MaterialCompiler/Lists.def"
};

std::string_view toSpelling(RasterizerInputKind kind);

enum class SceneDataKind {
#define BL_MATGRAPH_SCENEDATA(Name, Type) Name,
#include "Bloom/MaterialCompiler/Lists.def"
};

enum class BuiltinFunctionKind {
#define BL_MATGRAPH_BUILTIN(Name, ...) Name,
#include "Bloom/MaterialCompiler/Lists.def"
};

std::string_view toSpelling(BuiltinFunctionKind function);

enum class SampleMode { Color, Normal };

enum class VectorTypeFlags { None = 0, Packed = 1 << 0 };

UTL_ENUM_OPERATORS(VectorTypeFlags)

} // namespace bloom::matc

#endif // BLOOM_MATERIALCOMPILER_FWD_H
