#include "Bloom/MaterialCompiler/CodeGen.h"

#include <fstream>
#include <span>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <utl/hashtable.hpp>
#include <utl/overload.hpp>
#include <utl/strcat.hpp>
#include <utl/streammanip.hpp>

#include "Bloom/MaterialCompiler/MaterialGraph.h"
#include "Bloom/MaterialCompiler/Values.h"

using namespace bloom;
using namespace matc;
using namespace ranges::views;

namespace {

struct Indenter {
public:
    explicit Indenter(std::ostream& str): str(str) {}

    auto beginLine() const {
        return utl::streammanip(
            [n = scopeNesting](std::ostream& str) { indent(str, n); });
    }

    void pushScope() {
        ++scopeNesting;
        str << " {\n";
    }

    void popScope(std::string_view postfix = {}) {
        --scopeNesting;
        str << beginLine();
        str << "}" << postfix << "\n";
    }

private:
    static void indent(std::ostream& str, int n) {
        for (int i = 0; i < n; ++i) {
            str << "    ";
        }
    }

    int scopeNesting = 0;
    std::ostream& str;
};

template <typename T>
class NameGenerator {
public:
    explicit NameGenerator(std::string prefix): prefix(prefix) {}

    std::string get(T const& elem) {
        auto itr = map.find(elem);
        if (itr != map.end()) {
            return itr->second;
        }
        return map.insert({ elem, generateNewName() }).first->second;
    }

    void setName(T const& elem, std::string name) {
        map[elem] = std::move(name);
    }

    bool contains(T const& elem) const { return map.contains(elem); }

private:
    std::string generateNewName() { return utl::strcat(prefix, index++); }

    std::string prefix;
    int index = 0;
    utl::hashmap<T, std::string> map;
};

struct Data {};

static std::string makeConstantName(VectorConstant const& c) {
    switch (c.numComponents()) {
    case 1:
        return utl::strcat(c.get().x);
    case 2:
        return utl::strcat("float2", c.get().xy);
    case 3:
        return utl::strcat("float3", c.get().xyz);
    case 4:
        return utl::strcat("float4", c.get());
    default:
        assert(false);
    }
}

struct MetalCodeGenerator: CodeGenerator, Data, Indenter {
    MetalCodeGenerator(Context const& ctx, MaterialGraph const& graph,
                       CodeGenOptions const& options):
        Indenter(sstr), ctx(ctx), graph(graph), options(options) {}

    CodegenResult run() override;

    void declareTexture(TextureConstant const* tex) {
        textureIndexMap[tex] = ReservedTextureSlots + textureIndexCounter++;
    }

    void declareType(Type const* type) {
        visit(*type, [this](auto& type) { declareTypeImpl(&type); });
    }
    void declareTypeImpl(Type const*) {}
    void declareTypeImpl(VectorType const* type);
    void declareTypeImpl(AggregateType const* type);

    void codegen(Value const& value) {
        visit(value, [this](auto& value) { codegenImpl(value); });
    }

    void beginFunction();
    void endFunction();

    void defineFragmentFunction();

    void codegenImpl(Value const&) { assert(false); }
    void codegenImpl(VectorConstant const&) {}
    void codegenImpl(TextureConstant const&) {}
    void codegenImpl(RasterizerInput const&) {}
    void codegenImpl(TextureSampler const& value);
    void codegenImpl(VectorBuilder const& value);
    void codegenImpl(VectorElement const&) {}
    void codegenImpl(AggregateBuilder const& value);

    std::string getName(Value const* value) {
        if (!valueNameGen.contains(value)) {
            // clang-format off
            auto name = visit<std::optional<std::string>>(*value, utl::overload{
                [&](VectorConstant const& c) {
                    return makeConstantName(c);
                },
                [&](TextureConstant const& c) {
                    return utl::strcat("Texture_", textureIndexMap[&c]);
                },
                [&](RasterizerInput const& input) {
                    using enum RasterizerInputKind;
                    switch (input.kind()) {
                    case TextureCoords:
                        return "in.textureCoords";
                    case VertexNormals:
                        return "in.normalWS.xyz";
                    case VertexTangents:
                        return "in.tangentWS.xyz";
                    case VertexBinormals:
                        return "in.binormalWS.xyz";
                    }
                },
                [&](VectorElement const& e) {
                    return utl::strcat(getName(e.operand()), ".",
                                       "xyzw"[e.index()]);
                },
                [](Value const&) { return std::nullopt; }
            }); // clang-format on
            if (name) {
                valueNameGen.setName(value, *name);
            }
        }
        return valueNameGen.get(value);
    }

    void setName(Value const* value, std::string name) {
        return valueNameGen.setName(value, std::move(name));
    }

    Context const& ctx;
    MaterialGraph const& graph;
    CodeGenOptions options;

    static constexpr int ReservedTextureSlots = 8;
    int textureIndexCounter = 0;

    Type const* returnType;
    NameGenerator<Type const*> typeNameGen{ "Type_" };
    NameGenerator<Value const*> valueNameGen{ "Value_" };
    utl::hashmap<TextureConstant const*, int> textureIndexMap;
    std::stringstream sstr;
};

} // namespace

std::unique_ptr<CodeGenerator> matc::makeCodeGenerator(
    Context const& ctx, MaterialGraph const& graph,
    CodeGenOptions const& options) {
    return std::make_unique<MetalCodeGenerator>(ctx, graph, options);
}

static std::vector<Value const*> topsort(std::span<Value const* const> values) {
    std::vector<Value const*> sorted;
    utl::hashset<Value const*> perm;
    utl::hashset<Value const*> tmp;
    auto visit = [&](auto& visit, Value const* value) {
        if (perm.contains(value)) {
            return;
        }
        if (tmp.contains(value)) {
            throw std::runtime_error("Material graph is not a DAG");
        }
        tmp.insert(value);
        if (auto* user = dyncast<User const*>(value)) {
            for (auto* operand: user->operands()) {
                visit(visit, operand);
            }
        }
        tmp.erase(value);
        perm.insert(value);
        sorted.push_back(value);
    };
    for (auto* value: values) {
        visit(visit, value);
    }
    return sorted;
}

CodegenResult MetalCodeGenerator::run() {
    if (!graph.root()) {
        throw std::runtime_error("Material graph has no root");
    }
    returnType = graph.root()->type();
    auto order = topsort(graph.values() | ranges::to<std::vector>);
    sstr << R"(
#include "Shader/PBRShader.h"
#include "Shader/SharedTypes.h"
#include "Shader/Vertex.h"

using namespace metal;
using namespace bloom;

struct FragmentIn {
    float4 positionCS [[position]];
    float3 normalWS;
    float3 tangentWS;
    float3 binormalWS;
    float3 positionWS;
    float3 vertexColor;
    float2 textureCoords;
};

)";
    for (auto* type: ctx.types()) {
        declareType(type);
    }
    for (auto tex: ctx.textureConstants()) {
        declareTexture(tex);
    }
    beginFunction();
    for (auto* value: order) {
        if (!value) {
            throw std::runtime_error("Encountered NULL value");
        }
        codegen(*value);
    }
    endFunction();
    defineFragmentFunction();
    return { .source = std::move(sstr).str(),
             .fragmentTextures = textureIndexMap | transform([](auto& p) {
        auto [tex, index] = p;
        return std::pair{ index, tex->asset() };
    }) | ranges::to<utl::hashmap<int, AssetHandle>> };
}

void MetalCodeGenerator::declareTypeImpl(VectorType const* type) {
    typeNameGen.setName(type, [&] {
        if (type->numComponents() == 1) {
            return std::string("float");
        }
        return utl::strcat("float", type->numComponents());
    }());
}

void MetalCodeGenerator::declareTypeImpl(AggregateType const* type) {
    if (type == graph.root()->type()) {
        typeNameGen.setName(type, "PBRData");
        return;
    }
    // TODO: This does not handle nested structs
    sstr << "struct " << typeNameGen.get(type);
    pushScope();
    for (auto [index, member]: type->elements() | enumerate) {
        sstr << beginLine() << typeNameGen.get(member) << " _" << index
             << ";\n";
    }
    popScope(";");
    sstr << "\n";
}

void MetalCodeGenerator::beginFunction() {
    sstr << "static " << typeNameGen.get(returnType) << " "
         << "material(FragmentIn in";
    for (auto [tex, index]: textureIndexMap) {
        sstr << ",\n    texture2d<half>" << getName(tex) << "\n";
    }
    sstr << "    ) ";
    pushScope();
}

void MetalCodeGenerator::endFunction() {
    std::string valueName = getName(graph.root());
    sstr << beginLine() << valueName << ".albedo = max(float3(), " << valueName
         << ".albedo);\n";
    sstr << beginLine() << valueName << ".emissive = max(float3(), "
         << valueName << ".emissive);\n";
    sstr << beginLine() << "return " << valueName << ";\n";
    popScope();
}

void MetalCodeGenerator::defineFragmentFunction() {
    sstr << R"(
vertex FragmentIn materialVertexShader(
    CameraInput device const& cameraBuffer [[buffer(0)]],
    Vertex3D device const* vertices [[buffer(1)]],
    float4x4 device const& transform [[buffer(2)]],
    unsigned vertexID [[vertex_id]])
{
    Vertex3D v = vertices[vertexID];
    float4 vertexPositionMS = float4(v.position, 1);
    float4 vertexNormalMS = float4(v.normal, 0);
    float4 vertexTangentMS = float4(v.tangent, 0);
    float4 vertexBitangentMS = float4(v.binormal, 0);
    float4 vertexPositionWS = transform * vertexPositionMS;
    return {
        .positionCS = cameraBuffer.camera * vertexPositionWS,
        .normalWS = (transform * vertexNormalMS).xyz,
        .tangentWS = (transform * vertexTangentMS).xyz,
        .binormalWS = (transform * vertexBitangentMS).xyz,
        .positionWS = vertexPositionWS.xyz,
        .vertexColor = v.color.rgb,
        .textureCoords = v.textureCoordinates[0]
    };
}

struct MainPassFragmentData {
    float4 color [[color(0)]];
    uint32_t entityID [[color(1)]];
};

fragment MainPassFragmentData materialFragmentShader(
    FragmentIn in [[stage_in]],
    CameraInput device const& cameraBuffer [[buffer(0)]],
    LightBufferContents device const& lights [[buffer(1)]],
    float4x4 device const* lightSpaceTransforms [[buffer(2)]],
    ShadowRenderMetadata device const& shadowMetadata [[buffer(3)]],
    uint32_t device const& entityID [[buffer(5)]],
    texture2d_array<float> shadowMaps [[texture(0)]])";
    for (auto [tex, index]: textureIndexMap) {
        sstr << ",\n    texture2d<half> " << getName(tex) << " [[texture("
             << index << ")]]";
    }
    sstr << R"()
{
    in.normalWS.xyz = normalize(in.normalWS.xyz);
    in.tangentWS.xyz = normalize(in.tangentWS.xyz);
    in.binormalWS.xyz = normalize(in.binormalWS.xyz);
    PBRData pbr = material(in)";
    for (auto [tex, index]: textureIndexMap) {
        sstr << ",\n        " << getName(tex);
    }
    sstr << R"(
    );
    float3x3 TBN(in.tangentWS.xyz,
                 in.binormalWS.xyz,
                 in.normalWS.xyz);
    pbr.normal = TBN * normalize(pbr.normal);
    float3 color =
        PBRForwardShader(pbr, in.positionWS,
                         cameraBuffer.cameraPosition, lightSpaceTransforms,
                         lights, shadowMetadata, shadowMaps);
    return { float4(color, 1), entityID };
}

#include "Shader/PBRShader.metal"

)";
}

void MetalCodeGenerator::codegenImpl(TextureSampler const& sample) {
    /*
     constexpr sampler textureSampler (mag_filter::linear, min_filter::linear);
     const half4 colorSample = colorTexture.sample(textureSampler,
     in.textureCoordinate);
     */
    std::string samplerName =
        utl::strcat(getName(sample.texture()), "_Sampler");
    std::string sampleTypeName = typeNameGen.get(sample.type());
    sstr << beginLine() << "constexpr sampler " << samplerName
         << "(mag_filter::linear, min_filter::linear);\n";
    sstr << beginLine() << sampleTypeName << " " << getName(&sample) << " = "
         << sampleTypeName << "(" << getName(sample.texture()) << ".sample("
         << samplerName << ", " << getName(sample.coords()) << "));\n";
}

void MetalCodeGenerator::codegenImpl(VectorBuilder const& value) {
    sstr << beginLine() << typeNameGen.get(value.type()) << " "
         << getName(&value) << "(";
    bool first = true;
    for (auto* operand: value.operands()) {
        if (!first) {
            sstr << ", ";
        }
        first = false;
        sstr << getName(operand);
    }
    sstr << ");\n";
}

void MetalCodeGenerator::codegenImpl(AggregateBuilder const& value) {
    sstr << beginLine() << typeNameGen.get(value.type()) << " "
         << getName(&value);
    pushScope();
    for (auto* operand: value.operands()) {
        sstr << beginLine() << getName(operand) << ",\n";
    }
    popScope(";");
}
