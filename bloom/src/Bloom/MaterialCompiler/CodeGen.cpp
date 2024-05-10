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

    std::string run() override;

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
    void codegenImpl(TextureCoords const&) {}
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
                [&](TextureCoords const&) {
                    return "in.textureCoords";
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

    Type const* returnType;
    NameGenerator<Type const*> typeNameGen{ "Type_" };
    NameGenerator<Value const*> valueNameGen{ "Value_" };
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

std::string MetalCodeGenerator::run() {
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
    float3 positionWS;
    float3 vertexColor;
    float2 textureCoords;
};

)";
    for (auto* type: ctx.types()) {
        declareType(type);
    }
    beginFunction();
    for (auto* value: order) {
        codegen(*value);
    }
    endFunction();
    defineFragmentFunction();
    return std::move(sstr).str();
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
         << "material(FragmentIn in) ";
    pushScope();
}

void MetalCodeGenerator::endFunction() {
    sstr << beginLine() << "return " << getName(graph.root()) << ";\n";
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
    float4 vertexPositionWS = transform * vertexPositionMS;
    return {
        .positionCS = cameraBuffer.camera * vertexPositionWS,
        .normalWS = (transform * vertexNormalMS).xyz,
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
    texture2d_array<float> shadowMaps [[texture(0)]])
{
    PBRData pbr = material(in);
    float3 color =
        PBRForwardShader(pbr, in.positionWS, normalize(in.normalWS.xyz),
                         cameraBuffer.cameraPosition, lightSpaceTransforms,
                         lights, shadowMetadata, shadowMaps);
    return { float4(color, 1), entityID };
}

#include "Shader/PBRShader.metal"

)";
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
    sstr << ");";
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
