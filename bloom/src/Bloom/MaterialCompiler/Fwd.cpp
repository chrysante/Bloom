#include "Bloom/MaterialCompiler/Fwd.h"

using namespace bloom;
using namespace matc;

std::string_view matc::toSpelling(RasterizerInputKind kind) {
    using enum RasterizerInputKind;
    switch (kind) {
    case ClipPosition:
        return "positionCS";
    case TextureCoords:
        return "UVs";
    case WorldNormals:
        return "normalWS";
    case WorldTangents:
        return "tangentWS";
    case WorldBinormals:
        return "binormalWS";
    case ModelNormals:
        return "normalMS";
    case WorldPosition:
        return "positionWS";
    case VertexColor:
        return "vertexColor";
    }
}

std::string_view matc::toSpelling(BuiltinFunctionKind function) {
    switch (function) {
#define BL_MATGRAPH_BUILTIN(Name, Spelling, ...)                               \
    case BuiltinFunctionKind::Name:                                            \
        return #Spelling;
#include "Bloom/MaterialCompiler/Lists.def"
    }
}
