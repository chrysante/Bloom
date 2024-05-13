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

struct MetalCodeGenerator: CodeGenerator, SourceEmitter {
    MetalCodeGenerator(Context const& ctx, MaterialGraph const& graph,
                       CodeGenOptions const& options):
        SourceEmitter(ScopeType::CompoundStmt),
        ctx(ctx),
        graph(graph),
        options(options) {}

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

    void writeFilePrologue();
    void writeVertexOutput();
    void writeFragmentOutput();
    void writeFragmentFunction();
    void writeVertexFunction();

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
    writeFilePrologue();
    writeVertexOutput();
    writeFragmentOutput();
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
    writeVertexFunction();
    writeFragmentFunction();
    writeInclude(IncludeType::Local, "Shader/PBRShader.metal");
    return { .source = takeSource(),
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
    auto item = makeItem();
    writeRaw("struct ", typeNameGen.get(type));
    auto scope = makeScope(ScopeType::CompoundStmt);
    for (auto [index, member]: type->elements() | enumerate) {
        writeItem(typeNameGen.get(member), " _", index);
    }
}

void MetalCodeGenerator::beginFunction() {
    beginItem();
    writeRaw("static ", typeNameGen.get(returnType), " material");
    beginScope(ScopeType::FuncParams);
    writeItem("FragmentIn in");
    for (auto [tex, index]: textureIndexMap) {
        writeItem("texture2d<half> ", getName(tex));
    }
    endScope();
    beginScope(ScopeType::CompoundStmt);
}

void MetalCodeGenerator::endFunction() {
    std::string valueName = getName(graph.root());
    writeItem([&] {
        writeRaw(valueName, ".albedo = max");
        writeScope(ScopeType::Paran, [&] {
            writeItem("float3()");
            writeItem(valueName, ".albedo");
        });
    });
    writeItem([&] {
        writeRaw(valueName, ".emissive = max");
        writeScope(ScopeType::Paran, [&] {
            writeItem("float3()");
            writeItem(valueName, ".emissive");
        });
    });
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
    writeRaw("struct FragmentIn");
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
    writeRaw("struct MainPassFragmentData");
    writeScope(ScopeType::CompoundStmt, [&] {
        writeItem("float4 color [[color(0)]]");
        writeItem("uint32_t entityID [[color(1)]]");
    });
}

void MetalCodeGenerator::writeVertexFunction() {
    auto fn = makeItem();
    writeRaw("vertex FragmentIn materialVertexShader");
    auto paramScope = makeScope(ScopeType::FuncParams);
    writeItem("CameraInput device const& cameraBuffer [[buffer(0)]]");
    writeItem("Vertex3D device const* vertices [[buffer(1)]]");
    writeItem("float4x4 device const& transform [[buffer(2)]]");
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
    auto fn = makeItem();
    writeRaw("fragment MainPassFragmentData materialFragmentShader");
    auto paramScope = makeScope(ScopeType::FuncParams);
    writeItem("FragmentIn in [[stage_in]]");
    writeItem("CameraInput device const& cameraBuffer [[buffer(0)]]");
    writeItem("LightBufferContents device const& lights [[buffer(1)]]");
    writeItem("float4x4 device const* lightSpaceTransforms [[buffer(2)]]");
    writeItem(
        "ShadowRenderMetadata device const& shadowMetadata [[buffer(3)]]");
    writeItem("uint32_t device const& entityID [[buffer(5)]]");
    writeItem("texture2d_array<float> shadowMaps [[texture(0)]]");
    for (auto [tex, index]: textureIndexMap) {
        writeItem([&] {
            writeRaw("texture2d<half> ", getName(tex), " ");
            auto attr1 = makeScope(ScopeType::Bracket);
            auto attr2 = makeScope(ScopeType::Bracket);
            writeRaw("texture");
            writeScope(ScopeType::Paran, index);
        });
    }
    paramScope.pop();
    auto fnScope = makeScope(ScopeType::CompoundStmt);
    writeItem("in.normalWS.xyz = normalize(in.normalWS.xyz)");
    writeItem("in.tangentWS.xyz = normalize(in.tangentWS.xyz)");
    writeItem("in.binormalWS.xyz = normalize(in.binormalWS.xyz)");
    writeItem([&] {
        writeRaw("PBRData pbr = material");
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

void MetalCodeGenerator::codegenImpl(TextureSampler const& sample) {
    /*
     constexpr sampler textureSampler (mag_filter::linear, min_filter::linear);
     const half4 colorSample = colorTexture.sample(textureSampler,
                                                   in.textureCoordinate);
     */
    std::string samplerName =
        utl::strcat(getName(sample.texture()), "_Sampler");
    std::string sampleTypeName = typeNameGen.get(sample.type());
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
    writeRaw(typeNameGen.get(value.type()), " ", getName(&value));
    auto scope = makeScope(ScopeType::Paran);
    for (auto* operand: value.operands()) {
        writeItem(getName(operand));
    }
}

void MetalCodeGenerator::codegenImpl(AggregateBuilder const& value) {
    auto item = makeItem();
    writeRaw(typeNameGen.get(value.type()), " ", getName(&value));
    auto scope = makeScope(ScopeType::BraceList);
    for (auto* operand: value.operands()) {
        writeItem(getName(operand));
    }
}
