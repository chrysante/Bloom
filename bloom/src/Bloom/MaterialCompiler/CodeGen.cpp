#include "Bloom/MaterialCompiler/CodeGen.h"

#include <fstream>
#include <span>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <range/v3/algorithm.hpp>
#include <utl/hashtable.hpp>
#include <utl/overload.hpp>
#include <utl/strcat.hpp>
#include <utl/streammanip.hpp>

#include "Bloom/Core/Ranges.h"
#include "Bloom/Graphics/Material/Fwd.h"
#include "Bloom/MaterialCompiler/MaterialGraph.h"
#include "Bloom/MaterialCompiler/SourceEmitter.h"
#include "Bloom/MaterialCompiler/Values.h"

using namespace bloom;
using namespace matc;
using namespace ranges::views;

[[noreturn]] static void unreachable() { __builtin_unreachable(); }

static MaterialDataType translate(matc::TextureType const* type) {
    switch (type->subtype()) {
    case TextureSubType::Texture2D:
        return MaterialDataType::Texture2D;
    case TextureSubType::TextureCube:
        return MaterialDataType::TextureCube;
    }
}

static MaterialDataType translate(VectorType const* type) {
    using enum MaterialDataType;
    return (MaterialDataType)((int)Float + type->numComponents() - 1);
}

static Type const* translate(Context& ctx, MaterialDataType type) {
    switch (type) {
    case MaterialDataType::Float:
        return ctx.floatType();
    case MaterialDataType::Float2:
        return ctx.float2Type();
    case MaterialDataType::Float3:
        return ctx.float3Type();
    case MaterialDataType::Float4:
        return ctx.float4Type();
    case MaterialDataType::Texture2D:
        return ctx.textureType(TextureSubType::Texture2D);
    case MaterialDataType::TextureCube:
        return ctx.textureType(TextureSubType::TextureCube);
    }
}

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

inline constexpr std::string_view MaterialVertexOutName = "MatVertexOut";
inline constexpr std::string_view MaterialFragmentOutName = "MatFragmentOut";
inline constexpr std::string_view ParameterObjectName = "parameters";

struct MetalCodeGenerator: CodeGenerator, SourceEmitter {
    MetalCodeGenerator(Context& ctx, MaterialGraph const& graph,
                       CodeGenOptions const& options):
        SourceEmitter(ScopeType::CompoundStmt),
        ctx(ctx),
        graph(graph),
        options(options) {}

    MaterialSourceData run() override;

    void declareType(Type const* type) {
        visit(*type, [this](auto& type) { declareTypeImpl(&type); });
    }
    void declareTypeImpl(Type const*) {}
    void declareTypeImpl(matc::TextureType const* type);
    void declareTypeImpl(VectorType const* type);
    void declareTypeImpl(AggregateType const* type);

    void defineMaterialFunction(Function const* F);
    void beginFunction(Function const* F);
    void endFunction(Function const* F);

    void writeFilePrologue();
    void defineVertexShaderOutputType();
    void defineFragmentShaderOutputType();

    void defineFragmentShaderFunction();
    void defineVertexShaderFunction();

    void codegen(Value const& value) {
        visit(value, [this](auto& value) { codegenImpl(value); });
    }
    void codegenImpl(Value const&) { assert(false); }
    void codegenImpl(VectorValue const&) {}
    void codegenImpl(TextureValue const&) {}
    void codegenImpl(RasterizerInput const&) {}
    void codegenImpl(FunctionCall const& value);
    void codegenImpl(ArithmeticOperation const& value);
    void codegenImpl(TextureSampler const& value);
    void codegenImpl(VectorBuilder const& value);
    void codegenImpl(ExtractVectorValue const&) {}
    void codegenImpl(ExtractAggregateValue const&) {}
    void codegenImpl(AggregateBuilder const& value);

    /// Called for every value that is part of the program
    /// Derived value types can overload `genNameImpl()` to generate custom
    /// names
    void genName(Value const& value);
    std::string genNameImpl(VectorConstant const& c);
    std::string genNameImpl(TextureValue const&);
    std::string genNameImpl(VectorParameter const& c);
    std::string genNameImpl(RasterizerInput const&);
    std::string genNameImpl(ExtractVectorValue const&);
    std::string genNameImpl(ExtractAggregateValue const&);

    std::string getName(Value const* value) { return valueNameGen.get(value); }

    std::string getName(Type const* type) { return typeNameGen.get(type); }

    void writeIf(auto condition, auto body);
    void callMaterialFunction(GraphRoot kind, std::string_view variableName);
    void clampGreaterZero(Type const* type, auto const&... valueName);
    void normalize(auto const&... valueName);

    static constexpr int ReservedTextureSlots = 8;

    Context& ctx;
    MaterialGraph const& graph;
    CodeGenOptions options;
    int textureIndexCounter = 0;
    int parameterIndexCounter = 0;
    AggregateType const* paramBufferType = nullptr;
    NameGenerator<Type const*> typeNameGen{ "Type_" };
    NameGenerator<Value const*> valueNameGen{ "Value_" };
    utl::hashmap<TextureValue const*, int> textureIndexMap;
    utl::hashmap<int, AssetHandle> textureConstants;
    utl::hashmap<Value const*, int> parameterIndexMap;
    std::vector<MaterialPrimitiveParameter> primitiveParameters;
    std::vector<MaterialTextureParameter> textureParameters;
};

} // namespace

std::unique_ptr<CodeGenerator> matc::makeCodeGenerator(
    Context& ctx, MaterialGraph const& graph, CodeGenOptions const& options) {
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

static std::pair<uint32_t, uint32_t> getSizeAndAlign(MaterialDataType type) {
    using enum MaterialDataType;
    switch (type) {
    case Float:
        return { 4, 4 };
    case Float2:
        return { 8, 8 };
    case Float3:
        return { 16, 16 };
    case Float4:
        return { 16, 16 };
    default:
        unreachable();
    }
}

static size_t computeBufferLayout(
    std::span<MaterialPrimitiveParameter> params,
    std::span<VectorParameter const*> paramValues) {
    auto cmp = [](auto const& a, auto const& b) {
        return (int)a.first.type < (int)b.first.type;
    };
    ranges::sort(zip(params, paramValues), cmp);
    auto itr = params.begin();
    uint32_t offset = 0;
    while (itr < params.end()) {
        auto type = itr->type;
        assert((int)type < (int)MaterialDataType::Texture2D &&
               "This function must not be used with texture parameters");
        auto [size, align] = getSizeAndAlign(type);
        offset += (offset % align == 0 ? 0 : align - offset % align);
        do {
            itr->bufferOffset = offset;
            offset += size;
            ++itr;
        } while (itr < params.end() && itr->type == type);
    }
    return offset;
}

static AggregateType const* makeParameterBufferType(
    Context& ctx, std::span<MaterialPrimitiveParameter const> params) {
    if (params.empty()) {
        return nullptr;
    }
    return ctx.aggregateType("PrimitiveParameters",
                             params |
                                 transform([&, index = 0](auto& param) mutable {
        return AggregateElement{ translate(ctx, param.type),
                                 utl::strcat("_", index++) };
    }) | ranges::to<std::vector>);
}

static std::vector<MaterialPrimitiveParameter> makePrimitiveParameters(
    std::span<VectorParameter const* const> params) {
    return params | transform([](VectorParameter const* param) {
        return MaterialPrimitiveParameter{ .type = translate(param->type()),
                                           .bufferOffset = 0,
                                           .name = param->name(),
                                           .value = param->get() };
    }) | ranges::to<std::vector>;
}

void MetalCodeGenerator::defineMaterialFunction(Function const* F) {
    beginFunction(F);
    for (auto* value: F->body()) {
        if (!value) {
            throw std::runtime_error("Encountered NULL value");
        }
        codegen(*value);
    }
    endFunction(F);
}

MaterialSourceData MetalCodeGenerator::run() {
    for (auto* value: graph.values()) {
        genName(*value);
    }
    writeFilePrologue();
    for (auto* type: ctx.types()) {
        declareType(type);
    }
    for (auto* F: graph.functions()) {
        if (F) {
            defineMaterialFunction(F);
        }
    }
    writeInclude(IncludeType::Local, "Shader/PBRShader.metal");
    return { .source = takeSource(),
             .constantFragmentTextures = std::move(textureConstants),
             .primitiveParameters = std::move(primitiveParameters),
             .textureParameters = std::move(textureParameters),
             .parameterBufferSize = /*parameterBufferSize*/ 0,
             .cullMode = graph.parameters().twoSided ? TriangleCullMode::None :
                                                       TriangleCullMode::Back };

    utl::hashset<Value const*> activeValues;
    utl::hashmap<GraphRoot, std::vector<Value const*>> orders;
    for (auto kind: magic_enum::enum_values<GraphRoot>()) {
        auto* root = graph.getRoot(kind);
        if (!root) {
            continue;
        }
        auto order = topsort(root);
        ranges::copy(order, std::inserter(activeValues, activeValues.begin()));
        orders.insert({ kind, std::move(order) });
    }
    auto vectorParameters = activeValues | filter(isa<VectorParameter>) |
                            transform(cast<VectorParameter const*>) |
                            ToSmallVector<>;
    primitiveParameters = makePrimitiveParameters(vectorParameters);
    size_t parameterBufferSize =
        computeBufferLayout(primitiveParameters, vectorParameters);
    paramBufferType = makeParameterBufferType(ctx, primitiveParameters);
    for (auto [index, param]: vectorParameters | enumerate) {
        parameterIndexMap[param] = (int)index;
    }
    for (auto* value: activeValues) {
        genName(*value);
    }
    writeFilePrologue();
    for (auto* type: ctx.types()) {
        declareType(type);
    }
    if (paramBufferType) {
        writeItem("static_assert(", "sizeof(", getName(paramBufferType),
                  ") == ", parameterBufferSize, ", \"\")");
    }
    for (auto& [kind, order]: orders) {
        //        defineMaterialFunction(kind, order);
    }
    defineVertexShaderFunction();
    defineFragmentShaderFunction();
    writeInclude(IncludeType::Local, "Shader/PBRShader.metal");
    return { .source = takeSource(),
             .constantFragmentTextures = std::move(textureConstants),
             .primitiveParameters = std::move(primitiveParameters),
             .textureParameters = std::move(textureParameters),
             .parameterBufferSize = parameterBufferSize,
             .cullMode = graph.parameters().twoSided ? TriangleCullMode::None :
                                                       TriangleCullMode::Back };
}

void MetalCodeGenerator::declareTypeImpl(matc::TextureType const* type) {
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

static std::string functionName(GraphRoot root) {
    using enum GraphRoot;
    switch (root) {
    case VertexFunction:
        return "VertexFunction";
    case LitFragmentFunction:
        return "LitFragmentFunction";
    case UnlitFragmentFunction:
        return "UnlitFragmentFunction";
    case TransparencyFunction:
        return "TransparencyFunction";
    }
}

void MetalCodeGenerator::declareTypeImpl(AggregateType const* type) {
    // TODO: This does not handle nested structs
    typeNameGen.setName(type, type->name());
    auto item = makeItem();
    writeRaw("struct ", getName(type));
    auto scope = makeScope(ScopeType::CompoundStmt);
    for (auto [index, member]: type->elements() | enumerate) {
        writeItem(getName(member.type), " ", member.name);
    }
}

void MetalCodeGenerator::beginFunction(Function const* F) {
    beginItem();
    writeRaw("static ", getName(F->returnType()), " ", F->name());
    beginScope(ScopeType::FuncParams);
    for (auto param: F->parameters()) {
        auto item = makeItem();
        writeRaw(getName(param.value->type()));
        switch (param.qualifier) {
            using enum FunctionParameter::Qualifier;
        case None:
            break;
        case DeviceConstRef:
            writeRaw(" device const&");
            break;
        case DeviceConstPointer:
            writeRaw(" device const*");
            break;
        }
        writeRaw(" ", getName(param.value));
        switch (param.attribute.type) {
            using enum ParameterAttribute::Type;
        case None:
            break;
        case Buffer:
            writeRaw(" [[buffer(", param.attribute.index, ")]]");
            break;
        case Texture:
            writeRaw(" [[texture(", param.attribute.index, ")]]");
            break;
        }
    }
    endScope();
    beginScope(ScopeType::CompoundStmt);
}

void MetalCodeGenerator::endFunction(Function const* F) {
    writeItem("return ", getName(F->root()));
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
    defineVertexShaderOutputType();
    defineFragmentShaderOutputType();
}

void MetalCodeGenerator::defineVertexShaderOutputType() {
    auto type = makeItem();
    writeRaw("struct ", MaterialVertexOutName);
    writeScope(ScopeType::CompoundStmt, [&] {
        writeItem("float4 positionCS [[position]]");
        writeItem("float3 normalWS");
        writeItem("float3 tangentWS");
        writeItem("float3 binormalWS");
        writeItem("float3 positionWS");
        writeItem("float3 normalMS");
        writeItem("float4 vertexColor");
        writeItem("float2 textureCoords");
    });
}

void MetalCodeGenerator::defineFragmentShaderOutputType() {
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

void MetalCodeGenerator::defineVertexShaderFunction() {
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
        writeItem(".normalMS = vertexNormalMS.xyz");
        writeItem(".vertexColor = v.color");
        writeItem(".textureCoords = v.textureCoordinates[0]");
    });
}

void MetalCodeGenerator::callMaterialFunction(GraphRoot kind,
                                              std::string_view variableName) {
    auto* root = graph.getRoot(kind);
    assert(root);
    auto item = makeItem();
    writeRaw(getName(root->type()), " ", variableName, " = ",
             functionName(kind));
    auto argScope = makeScope(ScopeType::Paran);
    writeItem("in");
    if (paramBufferType) {
        writeItem(ParameterObjectName);
    }
    for (auto [tex, index]: textureIndexMap) {
        writeItem(getName(tex));
    }
}

void MetalCodeGenerator::defineFragmentShaderFunction() {
    using enum MaterialShadingModel;
    using enum GlobalMaterialBindingType;
    using enum MaterialShaderType;
    auto SM = graph.parameters().shadingModel;
    GraphRoot materialFunction = [&] {
        switch (SM) {
        case DefaultLit:
            return GraphRoot::LitFragmentFunction;
        case Unlit:
            return GraphRoot::UnlitFragmentFunction;
        }
    }();
    auto fn = makeItem();

    /// Parameters
    writeRaw("fragment ", MaterialFragmentOutName, " ",
             MaterialFragmentShaderName);
    auto paramScope = makeScope(ScopeType::FuncParams);
    writeItem(MaterialVertexOutName, " in [[stage_in]]");
    writeItem("CameraInput device const& cameraBuffer [[buffer(",
              getBindIndex(Fragment, Camera), ")]]");
    writeItem("uint32_t device const& entityID [[buffer(",
              getBindIndex(Fragment, GeometryIDBuffer), ")]]");
    if (SM == DefaultLit) {
        writeItem("LightBufferContents device const& lights [[buffer(",
                  getBindIndex(Fragment, Lights), ")]]");
        writeItem("float4x4 device const* lightSpaceTransforms [[buffer(",
                  getBindIndex(Fragment, LightSpaceTransforms), ")]]");
        writeItem("texture2d_array<float> shadowMaps [[texture(",
                  getBindIndex(Fragment, ShadowMaps), ")]]");
        writeItem("texturecube<float> specularMap [[texture(",
                  getBindIndex(Fragment, SpecularTexture), ")]]");
        writeItem("texture2d<float> bdrfMap [[texture(",
                  getBindIndex(Fragment, BRDFTexture), ")]]");
    }
    if (paramBufferType) {
        writeItem(getName(paramBufferType), " device const& ",
                  ParameterObjectName, " [[buffer(",
                  getBindIndex(Fragment, MaterialParameterBuffer), ")]]");
    }
    for (auto [tex, index]: textureIndexMap) {
        writeItem(getName(tex->type()), " ", getName(tex), " [[texture(", index,
                  ")]]");
    }
    paramScope.pop();

    /// Function body
    auto fnScope = makeScope(ScopeType::CompoundStmt);
    if (graph.parameters().transparency == MaterialTransparency::Masked) {
        callMaterialFunction(GraphRoot::TransparencyFunction, "transparency");
        writeIf("transparency.value < 0.5",
                [&] { writeItem("discard_fragment()"); });
    }
    normalize("in.normalWS.xyz");
    normalize("in.tangentWS.xyz");
    normalize("in.binormalWS.xyz");
    callMaterialFunction(materialFunction, "materialData");
    switch (SM) {
    case DefaultLit:
        writeItem([&] {
            writeRaw("float3x3 TBN");
            writeScope(ScopeType::Paran, [&] {
                writeItem("in.tangentWS.xyz");
                writeItem("in.binormalWS.xyz");
                writeItem("in.normalWS.xyz");
            });
        });
        writeItem("materialData.normal = TBN * normalize(materialData.normal)");
        writeItem([&] {
            writeRaw("PBRData pbr = ");
            writeScope(ScopeType::BraceList, [&] {
                writeItem("materialData.albedo");
                writeItem("materialData.emissive");
                writeItem("materialData.roughness");
                writeItem("materialData.metallic");
                writeItem("materialData.ao");
                writeItem("materialData.normal");
            });
        });
        writeItem([&] {
            writeRaw("float3 color = PBRForwardShader");
            auto argScope = makeScope(ScopeType::Paran);
            writeItem("pbr");
            writeItem("in.positionWS");
            writeItem("cameraBuffer.position");
            writeItem("lightSpaceTransforms");
            writeItem("lights");
            writeItem("shadowMaps");
            writeItem("specularMap");
            writeItem("bdrfMap");
        });
        writeItem("return { float4(color, 1), entityID }");
        break;
    case Unlit:
        writeItem("return { float4(materialData.emissive, 1), entityID }");
        break;
    }
    fnScope.pop();
}

void MetalCodeGenerator::codegenImpl(FunctionCall const& value) {
    auto item = makeItem();
    writeRaw(getName(value.type()), " ", getName(&value), " = ");
    writeRaw(toSpelling(value.function()));
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

std::string MetalCodeGenerator::genNameImpl(VectorParameter const& param) {
    if (!param.buffer()) {
        throw std::runtime_error("Parameter has no buffer");
    }
    return utl::strcat(getName(param.buffer()), "._", param.elementIndex());
}

static MaterialTextureParameter translate(TextureParameter const& param,
                                          int fragmentIndex) {
    return { .type = translate(param.type()),
             .fragmentBindingPoint = fragmentIndex,
             .vertexBindingPoint = MaterialTextureParameter::UnusedBindingPoint,
             .name = param.name(),
             .value = param.asset() };
}

std::string MetalCodeGenerator::genNameImpl(TextureValue const& tex) {
    int index = ReservedTextureSlots + textureIndexCounter++;
    textureIndexMap[&tex] = index;
    // clang-format off
    visit(tex, utl::overload{
        [&](TextureConstant const& constant) {
            textureConstants[index] = constant.asset();
        },
        [&](TextureParameter const& param) {
            textureParameters.push_back(translate(param, index));
        },
    }); // clang-format on
    return utl::strcat("Texture_", index);
}

std::string MetalCodeGenerator::genNameImpl(RasterizerInput const& input) {
    using enum RasterizerInputKind;
    switch (input.kind()) {
    case TextureCoords:
        return "in.textureCoords";
    case WorldNormals:
        return "in.normalWS";
    case WorldTangents:
        return "in.tangentWS";
    case WorldBinormals:
        return "in.binormalWS";
    case WorldPosition:
        return "in.positionWS";
    case ModelNormals:
        return "in.normalMS";
    case VertexColor:
        return "in.vertexColor";
    }
}

std::string MetalCodeGenerator::genNameImpl(ExtractVectorValue const& e) {
    return utl::strcat(getName(e.operand()), ".", "xyzw"[e.index()]);
}

std::string MetalCodeGenerator::genNameImpl(ExtractAggregateValue const& e) {
    return utl::strcat(getName(e.operand()), "._", e.index());
}

void MetalCodeGenerator::writeIf(auto condition, auto body) {
    auto item = makeItem();
    writeRaw("if (", condition, ")");
    auto scope = makeScope(ScopeType::CompoundStmt);
    body();
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

void MetalCodeGenerator::normalize(auto const&... valueName) {
    writeItem([&] {
        writeRaw(valueName..., " = normalize");
        writeScope(ScopeType::Paran, [&] { writeItem(valueName...); });
    });
}
