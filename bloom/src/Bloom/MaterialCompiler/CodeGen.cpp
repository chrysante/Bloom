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

#include "Bloom/Graphics/Material/Fwd.h"
#include "Bloom/MaterialCompiler/MaterialGraph.h"
#include "Bloom/MaterialCompiler/SourceEmitter.h"
#include "Bloom/MaterialCompiler/Values.h"

using namespace bloom;
using namespace matc;
using namespace ranges::views;

namespace {

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

static constexpr std::string_view MaterialVertexFunctionName =
    "materialVertexFunction";
static constexpr std::string_view MaterialFragmentFunctionName =
    "materialFragmentFunction";
inline constexpr std::string_view MaterialVertexOutName = "MatVertexOut";
inline constexpr std::string_view MaterialFragmentOutName = "MatFragmentOut";

struct MetalCodeGenerator: CodeGenerator, SourceEmitter {
    MetalCodeGenerator(Context const& ctx, MaterialGraph const& graph,
                       CodeGenOptions const& options):
        SourceEmitter(ScopeType::CompoundStmt),
        ctx(ctx),
        graph(graph),
        options(options) {}

    CodegenResult run() override;

    void declareType(Type const* type) {
        visit(*type, [this](auto& type) { declareTypeImpl(&type); });
    }
    void declareTypeImpl(Type const*) {}
    void declareTypeImpl(TextureType const* type);
    void declareTypeImpl(VectorType const* type);
    void declareTypeImpl(AggregateType const* type);

    void beginFunction();
    void endFunction();

    void writeFilePrologue();
    void writeVertexOutput();
    void writeFragmentOutput();
    void writeFragmentFunction();
    void writeVertexFunction();
    void writeUnlitFragmentFunction();
    void writeDefaultLitFragmentFunction();

    void codegen(Value const& value) {
        visit(value, [this](auto& value) { codegenImpl(value); });
    }
    void codegenImpl(Value const&) { assert(false); }
    void codegenImpl(VectorConstant const&) {}
    void codegenImpl(TextureConstant const&) {}
    void codegenImpl(RasterizerInput const&) {}
    void codegenImpl(BuiltinFunction const& value);
    void codegenImpl(ArithmeticOperation const& value);
    void codegenImpl(TextureSampler const& value);
    void codegenImpl(VectorBuilder const& value);
    void codegenImpl(VectorElement const&) {}
    void codegenImpl(AggregateElement const&) {}
    void codegenImpl(AggregateBuilder const& value);

    /// Called for every value that is part of the program
    /// Derived value types can overload `genNameImpl()` to generate custom
    /// names
    void genName(Value const& value);
    std::string genNameImpl(VectorConstant const& c);
    std::string genNameImpl(TextureConstant const&);
    std::string genNameImpl(RasterizerInput const&);
    std::string genNameImpl(VectorElement const&);
    std::string genNameImpl(AggregateElement const&);

    std::string getName(Value const* value) { return valueNameGen.get(value); }

    std::string getName(Type const* type) { return typeNameGen.get(type); }

    void clampGreaterZero(Type const* type, auto const&... valueName);

    Context const& ctx;
    MaterialGraph const& graph;
    CodeGenOptions options;

    static constexpr int ReservedTextureSlots = 8;
    int textureIndexCounter = 0;

    Type const* returnType;
    NameGenerator<Type const*> typeNameGen{ "Type_" };
    NameGenerator<Value const*> valueNameGen{ "Value_" };
    utl::hashmap<TextureConstant const*, int> textureIndexMap;
};

} // namespace

std::unique_ptr<CodeGenerator> matc::makeCodeGenerator(
    Context const& ctx, MaterialGraph const& graph,
    CodeGenOptions const& options) {
    return std::make_unique<MetalCodeGenerator>(ctx, graph, options);
}

static std::vector<Value const*> topsort(Value const* root) {
    std::vector<Value const*> sorted;
    utl::hashset<Value const*> perm;
    utl::hashset<Value const*> tmp;
    auto visit = [&](auto& visit, Value const* value) {
        if (!value) {
            throw std::runtime_error("Encountered NULL value");
        }
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
    visit(visit, root);
    return sorted;
}

CodegenResult MetalCodeGenerator::run() {
    if (!graph.root()) {
        throw std::runtime_error("Material graph has no root");
    }
    returnType = graph.root()->type();
    auto order = topsort(graph.root());
    for (auto* value: order) {
        genName(*value);
    }
    writeFilePrologue();
    writeVertexOutput();
    writeFragmentOutput();
    for (auto* type: ctx.types()) {
        declareType(type);
    }
    beginFunction();
    for (auto* value: order) {
        if (!value) {
            throw std::runtime_error("Encountered NULL value");
        }
        codegen(*value);
    }
    endFunction();
    writeVertexFunction();
    writeFragmentFunction();
    writeInclude(IncludeType::Local, "Shader/PBRShader.metal");
    return { .source = takeSource(),
             .fragmentTextures = textureIndexMap | transform([](auto& p) {
        auto [tex, index] = p;
        return std::pair{ index, tex->asset() };
    }) | ranges::to<utl::hashmap<int, AssetHandle>> };
}

void MetalCodeGenerator::declareTypeImpl(TextureType const* type) {
    typeNameGen.setName(type, [&] {
        switch (type->subtype()) {
        case TextureSubType::Texture2D:
            return "texture2d<float>";
        case TextureSubType::TextureCube:
            return "texturecube<float>";
        }
    }());
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
    auto item = makeItem();
    writeRaw("struct ", getName(type));
    auto scope = makeScope(ScopeType::CompoundStmt);
    for (auto [index, memberType]: type->elements() | enumerate) {
        writeItem(getName(memberType), " _", index);
    }
}

void MetalCodeGenerator::beginFunction() {
    beginItem();
    writeRaw("static ", getName(returnType), " ", MaterialFragmentFunctionName);
    beginScope(ScopeType::FuncParams);
    writeItem(MaterialVertexOutName, " in");
    for (auto [tex, index]: textureIndexMap) {
        writeItem(getName(tex->type()), " ", getName(tex));
    }
    endScope();
    beginScope(ScopeType::CompoundStmt);
}

void MetalCodeGenerator::endFunction() {
    std::string valueName = getName(graph.root());
    switch (graph.shadingModel()) {
    case MaterialShadingModel::Unlit:
        clampGreaterZero(returnType, valueName);
        break;
    case MaterialShadingModel::DefaultLit:
        clampGreaterZero(ctx.float3Type(), valueName, ".albedo");
        clampGreaterZero(ctx.float3Type(), valueName, ".emissive");
        break;
    }
    writeItem("return ", valueName);
    endScope();
    endItem();
}

void MetalCodeGenerator::writeFilePrologue() {
    writeInclude(IncludeType::System, "metal_stdlib");
    writeInclude(IncludeType::Local, "Shader/PBRShader.h");
    writeInclude(IncludeType::Local, "Shader/SharedTypes.h");
    writeInclude(IncludeType::Local, "Shader/Vertex.h");
    writeItem("using namespace metal");
    writeItem("using namespace bloom");
}

void MetalCodeGenerator::writeVertexOutput() {
    auto type = makeItem();
    writeRaw("struct ", MaterialVertexOutName);
    writeScope(ScopeType::CompoundStmt, [&] {
        writeItem("float4 positionCS [[position]]");
        writeItem("float3 normalWS");
        writeItem("float3 tangentWS");
        writeItem("float3 binormalWS");
        writeItem("float3 positionWS");
        writeItem("float3 vertexColor");
        writeItem("float2 textureCoords");
    });
}

void MetalCodeGenerator::writeFragmentOutput() {
    auto type = makeItem();
    writeRaw("struct ", MaterialFragmentOutName);
    writeScope(ScopeType::CompoundStmt, [&] {
        writeItem("float4 color [[color(0)]]");
        writeItem("uint32_t entityID [[color(1)]]");
    });
}

static int getBindIndex(MaterialShaderType shaderType,
                        GlobalMaterialBindingType bindingType) {
    return getGlobalMaterialBindingPoint(shaderType, bindingType).index;
}

void MetalCodeGenerator::writeVertexFunction() {
    using enum GlobalMaterialBindingType;
    using enum MaterialShaderType;
    auto fn = makeItem();
    writeRaw("vertex ", MaterialVertexOutName, " ", MaterialVertexShaderName);
    auto paramScope = makeScope(ScopeType::FuncParams);
    writeItem("CameraInput device const& cameraBuffer [[buffer(",
              getBindIndex(Vertex, Camera), ")]]");
    writeItem("Vertex3D device const* vertices [[buffer(",
              getBindIndex(Vertex, Geometry), ")]]");
    writeItem("float4x4 device const& transform [[buffer(",
              getBindIndex(Vertex, Transform), ")]]");
    writeItem("unsigned vertexID [[vertex_id]]");
    paramScope.pop();
    auto fnScope = makeScope(ScopeType::CompoundStmt);
    writeItem("Vertex3D v = vertices[vertexID]");
    writeItem("float4 vertexPositionMS = float4(v.position, 1)");
    writeItem("float4 vertexNormalMS = float4(v.normal, 0)");
    writeItem("float4 vertexTangentMS = float4(v.tangent, 0)");
    writeItem("float4 vertexBitangentMS = float4(v.binormal, 0)");
    writeItem("float4 vertexPositionWS = transform * vertexPositionMS");
    writeItem([&] {
        writeRaw("return ");
        auto scope = makeScope(ScopeType::BraceList);
        writeItem(".positionCS = cameraBuffer.camera * vertexPositionWS");
        writeItem(".normalWS = (transform * vertexNormalMS).xyz");
        writeItem(".tangentWS = (transform * vertexTangentMS).xyz");
        writeItem(".binormalWS = (transform * vertexBitangentMS).xyz");
        writeItem(".positionWS = vertexPositionWS.xyz");
        writeItem(".vertexColor = v.color.rgb");
        writeItem(".textureCoords = v.textureCoordinates[0]");
    });
}

void MetalCodeGenerator::writeFragmentFunction() {
    switch (graph.shadingModel()) {
    case MaterialShadingModel::Unlit:
        writeUnlitFragmentFunction();
        break;
    case MaterialShadingModel::DefaultLit:
        writeDefaultLitFragmentFunction();
        break;
    }
}

void MetalCodeGenerator::writeUnlitFragmentFunction() {
    using enum GlobalMaterialBindingType;
    using enum MaterialShaderType;
    auto fn = makeItem();
    writeRaw("fragment ", MaterialFragmentOutName, " ",
             MaterialFragmentShaderName);
    auto paramScope = makeScope(ScopeType::FuncParams);
    writeItem(MaterialVertexOutName, " in [[stage_in]]");
    writeItem("CameraInput device const& cameraBuffer [[buffer(",
              getBindIndex(Fragment, Camera), ")]]");
    writeItem("uint32_t device const& entityID [[buffer(",
              getBindIndex(Fragment, GeometryIDBuffer), ")]]");
    for (auto [tex, index]: textureIndexMap) {
        writeItem(getName(tex->type()), " ", getName(tex), " [[texture(", index,
                  ")]]");
    }
    paramScope.pop();
    auto fnScope = makeScope(ScopeType::CompoundStmt);
    writeItem("in.normalWS.xyz = normalize(in.normalWS.xyz)");
    writeItem("in.tangentWS.xyz = normalize(in.tangentWS.xyz)");
    writeItem("in.binormalWS.xyz = normalize(in.binormalWS.xyz)");
    writeItem([&] {
        writeRaw("float3 color = ", MaterialFragmentFunctionName);
        auto argScope = makeScope(ScopeType::Paran);
        writeItem("in");
        for (auto [tex, index]: textureIndexMap) {
            writeItem(getName(tex));
        }
    });
    writeItem("return { float4(color, 1), entityID }");
    fnScope.pop();
}

void MetalCodeGenerator::writeDefaultLitFragmentFunction() {
    using enum GlobalMaterialBindingType;
    using enum MaterialShaderType;
    auto fn = makeItem();
    writeRaw("fragment ", MaterialFragmentOutName, " ",
             MaterialFragmentShaderName);
    auto paramScope = makeScope(ScopeType::FuncParams);
    writeItem(MaterialVertexOutName, " in [[stage_in]]");
    writeItem("CameraInput device const& cameraBuffer [[buffer(",
              getBindIndex(Fragment, Camera), ")]]");
    writeItem("LightBufferContents device const& lights [[buffer(",
              getBindIndex(Fragment, Lights), ")]]");
    writeItem("float4x4 device const* lightSpaceTransforms [[buffer(",
              getBindIndex(Fragment, LightSpaceTransforms), ")]]");
    writeItem("ShadowRenderMetadata device const& shadowMetadata [[buffer(",
              getBindIndex(Fragment, ShadowMetadata), ")]]");
    writeItem("uint32_t device const& entityID [[buffer(",
              getBindIndex(Fragment, GeometryIDBuffer), ")]]");
    writeItem("texture2d_array<float> shadowMaps [[texture(",
              getBindIndex(Fragment, ShadowMaps), ")]]");
    for (auto [tex, index]: textureIndexMap) {
        writeItem(getName(tex->type()), " ", getName(tex), " [[texture(", index,
                  ")]]");
    }
    paramScope.pop();
    auto fnScope = makeScope(ScopeType::CompoundStmt);
    writeItem("in.normalWS.xyz = normalize(in.normalWS.xyz)");
    writeItem("in.tangentWS.xyz = normalize(in.tangentWS.xyz)");
    writeItem("in.binormalWS.xyz = normalize(in.binormalWS.xyz)");
    writeItem([&] {
        writeRaw("PBRData pbr = ", MaterialFragmentFunctionName);
        auto argScope = makeScope(ScopeType::Paran);
        writeItem("in");
        for (auto [tex, index]: textureIndexMap) {
            writeItem(getName(tex));
        }
    });
    writeItem([&] {
        writeRaw("float3x3 TBN");
        writeScope(ScopeType::Paran, [&] {
            writeItem("in.tangentWS.xyz");
            writeItem("in.binormalWS.xyz");
            writeItem("in.normalWS.xyz");
        });
    });
    writeItem("pbr.normal = TBN * normalize(pbr.normal)");
    writeItem([&] {
        writeRaw("float3 color = PBRForwardShader");
        auto argScope = makeScope(ScopeType::Paran);
        writeItem("pbr");
        writeItem("in.positionWS");
        writeItem("cameraBuffer.cameraPosition");
        writeItem("lightSpaceTransforms");
        writeItem("lights");
        writeItem("shadowMetadata");
        writeItem("shadowMaps");
    });
    writeItem("return { float4(color, 1), entityID }");
    fnScope.pop();
}

void MetalCodeGenerator::codegenImpl(BuiltinFunction const& value) {
    auto item = makeItem();
    writeRaw(getName(value.type()), " ", getName(&value), " = ");
    using enum BuiltinFunctionKind;
    switch (value.kind()) {
    case Normalize:
        writeRaw("normalize");
        break;
    case Sqrt:
        writeRaw("sqrt");
        break;
    }
    writeScope(ScopeType::Paran, [&] {
        for (auto& arg: value.operands()) {
            writeItem(getName(arg));
        }
    });
}

static std::string_view getSourceRepr(ArithmeticOperation::Kind operation) {
    using enum ArithmeticOperation::Kind;
    switch (operation) {
    case Add:
        return "+";
    case Sub:
        return "-";
    case Mul:
        return "*";
    case Div:
        return "/";
    }
}

void MetalCodeGenerator::codegenImpl(ArithmeticOperation const& value) {
    auto item = makeItem();
    writeRaw(getName(value.type()), " ", getName(&value), " = ",
             getName(value.LHS()), " ", getSourceRepr(value.kind()), " ",
             getName(value.RHS()));
}

void MetalCodeGenerator::codegenImpl(TextureSampler const& sample) {
    /*
     constexpr sampler textureSampler (mag_filter::linear, min_filter::linear);
     const half4 colorSample = colorTexture.sample(textureSampler,
                                                   in.textureCoordinate);
     */
    std::string samplerName = utl::strcat(getName(&sample), "_Sampler");
    std::string sampleTypeName = getName(sample.type());
    std::string sampleName = getName(&sample);
    writeItem([&] {
        writeRaw("constexpr sampler ", samplerName);
        writeScope(ScopeType::Paran, [&] {
            writeItem("min_filter::linear");
            writeItem("mag_filter::linear");
        });
    });
    writeItem([&] {
        writeRaw(sampleTypeName, " ", sampleName, " = ", sampleTypeName);
        auto scope = makeScope(ScopeType::Paran);
        auto item = makeItem();
        writeRaw(getName(sample.texture()), ".sample");
        writeScope(ScopeType::Paran, [&] {
            writeItem(samplerName);
            writeItem(getName(sample.coords()));
        });
    });
    if (sample.mode() == SampleMode::Normal) {
        auto item = makeItem();
        writeRaw(sampleName, ".xy = ");
        writeScope(ScopeType::Paran,
                   [&] { writeItem(sampleName, ".xy - 0.5f"); });
        writeRaw(" * 2.0f");
    }
}

void MetalCodeGenerator::codegenImpl(VectorBuilder const& value) {
    auto item = makeItem();
    writeRaw(getName(value.type()), " ", getName(&value));
    auto scope = makeScope(ScopeType::Paran);
    for (auto* operand: value.operands()) {
        writeItem(getName(operand));
    }
}

void MetalCodeGenerator::codegenImpl(AggregateBuilder const& value) {
    auto item = makeItem();
    writeRaw(getName(value.type()), " ", getName(&value));
    auto scope = makeScope(ScopeType::BraceList);
    for (auto* operand: value.operands()) {
        writeItem(getName(operand));
    }
}

void MetalCodeGenerator::genName(Value const& value) {
    visit(value, [this](auto& value) {
        if constexpr (requires { genNameImpl(value); }) {
            valueNameGen.setName(&value, genNameImpl(value));
        }
    });
}

std::string MetalCodeGenerator::genNameImpl(VectorConstant const& c) {
    if (c.numComponents() == 1) {
        return utl::strcat(c.get().x);
    }
    SourceEmitter e(ScopeType::Paran);
    e.writeRaw("float", c.numComponents());
    e.writeScope(ScopeType::Paran, [&] {
        for (int i = 0; i < c.numComponents(); ++i) {
            e.writeItem(c.get()[i]);
        }
    });
    return e.takeSource();
}

std::string MetalCodeGenerator::genNameImpl(TextureConstant const& tex) {
    int index = ReservedTextureSlots + textureIndexCounter++;
    textureIndexMap[&tex] = index;
    return utl::strcat("Texture_", index);
}

std::string MetalCodeGenerator::genNameImpl(RasterizerInput const& input) {
    using enum RasterizerInputKind;
    switch (input.kind()) {
    case TextureCoords:
        return "in.textureCoords";
    case VertexNormals:
        return "in.normalWS";
    case VertexTangents:
        return "in.tangentWS";
    case VertexBinormals:
        return "in.binormalWS";
    case WorldPosition:
        return "in.positionWS";
    }
}

std::string MetalCodeGenerator::genNameImpl(VectorElement const& e) {
    return utl::strcat(getName(e.operand()), ".", "xyzw"[e.index()]);
}

std::string MetalCodeGenerator::genNameImpl(AggregateElement const& e) {
    return utl::strcat(getName(e.operand()), "._", e.index());
}

void MetalCodeGenerator::clampGreaterZero(Type const* type,
                                          auto const&... valueName) {
    writeItem([&] {
        writeRaw(valueName..., " = max");
        writeScope(ScopeType::Paran, [&] {
            writeItem(getName(type), "()");
            writeItem(valueName...);
        });
    });
}
