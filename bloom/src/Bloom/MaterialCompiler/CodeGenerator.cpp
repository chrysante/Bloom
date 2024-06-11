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
#include "Bloom/Graphics/Material/MaterialSourceData.h"
#include "Bloom/MaterialCompiler/Codegen.h"
#include "Bloom/MaterialCompiler/Context.h"
#include "Bloom/MaterialCompiler/Module.h"
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
    case TextureSubType::Texture2DArray:
        assert(false);
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
class NameMap {
public:
    std::optional<std::string> get(T const& elem) {
        auto itr = map.find(elem);
        if (itr != map.end()) {
            return itr->second;
        }
        return std::nullopt;
    }

    void setName(T const& elem, std::string name) {
        map[elem] = std::move(name);
    }

    bool contains(T const& elem) const { return map.contains(elem); }

private:
    utl::hashmap<T, std::string> map;
};

struct MetalCodeGenerator: CodeGenerator, SourceEmitter {
    MetalCodeGenerator(Context& ctx, Module const& mod):
        SourceEmitter(ScopeType::CompoundStmt), ctx(ctx), mod(mod) {}

    std::string run() override;

    void declareType(Type const* type) {
        if (!typeNameMap.contains(type)) {
            visit(*type, [this](auto& type) { declareTypeImpl(&type); });
        }
    }
    void declareTypeImpl(VoidType const* type);
    void declareTypeImpl(BoolType const* type);
    void declareTypeImpl(IntType const* type);
    void declareTypeImpl(VectorType const* type);
    void declareTypeImpl(MatrixType const* type);
    void declareTypeImpl(FunctionType const* type);
    void declareTypeImpl(matc::TextureType const* type);
    void declareTypeImpl(AggregateType const* type);
    void declareTypeImpl(PointerType const* type);
    void declareTypeImpl(ArrayType const* type);

    void defineMaterialFunction(Function const* F);
    void beginFunction(Function const* F);
    void endFunction();

    void writeFilePrologue();

    void defineFragmentShaderFunction();
    void defineVertexShaderFunction();

    void codegen(Instruction const& inst) {
        visit(inst, [this](auto& inst) { codegenImpl(inst); });
    }
    void codegenImpl(Instruction const&) { assert(false); }
    void codegenImpl(FunctionCall const& value);
    void codegenImpl(ArithmeticInst const& value);
    void codegenImpl(CompareInst const& value);
    void codegenImpl(TextureSampler const& value);
    void codegenImpl(VectorBuilder const& value);
    void codegenImpl(MatrixBuilder const& value);
    void codegenImpl(AggregateBuilder const& value);
    void codegenImpl(ExtractVectorValue const&) {}
    void codegenImpl(ExtractAggregateValue const&) {}
    void codegenImpl(ArrayAccess const& inst);
    void codegenImpl(DiscardFragment const& inst);
    void codegenImpl(ReturnInst const& ret);

    /// Called for every value that is part of the program
    /// Derived value types can overload `genNameImpl()` to generate custom
    /// names
    void genName(Value const& value);
    std::string genNameImpl(IntConstant const& c);
    std::string genNameImpl(VectorConstant const& c);
    std::string genNameImpl(TextureConstant const&);
    std::string genNameImpl(RasterizerInput const&);
    std::string genNameImpl(BuiltinFunction const&);
    std::string genNameImpl(ExtractVectorValue const&);
    std::string genNameImpl(ExtractAggregateValue const&);

    std::string getName(Value const* value) {
        if (auto name = valueNameMap.get(value)) {
            return *std::move(name);
        }
        genName(*value);
        return valueNameMap.get(value).value();
    }

    std::string getName(Type const* type) {
        return typeNameMap.get(type).value();
    }

    void writeIf(auto condition, auto body);
    void clampGreaterZero(Type const* type, auto const&... valueName);
    void normalize(auto const&... valueName);

    Context& ctx;
    Module const& mod;
    int textureIndexCounter = 0;
    int parameterIndexCounter = 0;
    int textureConstantCounter = 0;
    AggregateType const* paramBufferType = nullptr;
    NameMap<Type const*> typeNameMap;
    NameMap<Value const*> valueNameMap;
    utl::hashmap<int, AssetHandle> textureConstants;
    utl::hashmap<Value const*, int> parameterIndexMap;
    std::vector<MaterialPrimitiveParameter> primitiveParameters;
    std::vector<MaterialTextureParameter> textureParameters;
};

} // namespace

std::unique_ptr<CodeGenerator> matc::makeCodeGenerator(Context& ctx,
                                                       Module const& mod) {
    return std::make_unique<MetalCodeGenerator>(ctx, mod);
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

void MetalCodeGenerator::defineMaterialFunction(Function const* F) {
    beginFunction(F);
    for (auto& inst: *F) {
        codegen(inst);
    }
    endFunction();
}

std::string MetalCodeGenerator::run() {
    writeFilePrologue();
    for (auto* type: ctx.types()) {
        declareType(type);
    }
    for (auto* F: mod.functions()) {
        if (F && !F->empty()) {
            defineMaterialFunction(F);
        }
    }
    writeInclude(IncludeType::Local, "Shader/PBRShader.metal");
    return takeSource();
}

void MetalCodeGenerator::declareTypeImpl(VoidType const* type) {
    typeNameMap.setName(type, "void");
}

void MetalCodeGenerator::declareTypeImpl(BoolType const* type) {
    typeNameMap.setName(type, "bool");
}

void MetalCodeGenerator::declareTypeImpl(IntType const* type) {
    typeNameMap.setName(type, [&] {
        using enum IntSubType;
        switch (type->subtype()) {
        case I8:
            return "int8_t";
        case I16:
            return "int16_t";
        case I32:
            return "int32_t";
        case U8:
            return "uint8_t";
        case U16:
            return "uint16_t";
        case U32:
            return "uint32_t";
        }
    }());
}

void MetalCodeGenerator::declareTypeImpl(VectorType const* type) {
    typeNameMap.setName(type, [&]() -> std::string {
        if (type->numComponents() == 1) {
            return "float";
        }
        std::stringstream sstr;
        if (type->packed()) {
            sstr << "packed_";
        }
        sstr << "float" << type->numComponents();
        return std::move(sstr).str();
    }());
}

void MetalCodeGenerator::declareTypeImpl(MatrixType const* type) {
    typeNameMap.setName(type, utl::strcat("float", type->rows(), "x",
                                          type->columns()));
}

void MetalCodeGenerator::declareTypeImpl(FunctionType const*) {}

void MetalCodeGenerator::declareTypeImpl(matc::TextureType const* type) {
    typeNameMap.setName(type, [&] {
        switch (type->subtype()) {
        case TextureSubType::Texture2D:
            return "texture2d<float>";
        case TextureSubType::TextureCube:
            return "texturecube<float>";
        case TextureSubType::Texture2DArray:
            return "texture2d_array<float>";
        }
    }());
}

static std::string toSpelling(ParameterAttribute attrib) {
    using enum ParameterAttribute::Type;
    switch (attrib.type) {
    case None:
        return {};
    case VertexID:
        return "[[vertex_id]]";
    case InstanceID:
        return "[[instance_id]]";
    case Position:
        return "[[position]]";
    case StageIn:
        return "[[stage_in]]";
    case Color:
        return utl::strcat("[[color(", attrib.index, ")]]");
    case Buffer:
        return utl::strcat("[[buffer(", attrib.index, ")]]");
    case Texture:
        return utl::strcat("[[texture(", attrib.index, ")]]");
    }
}

void MetalCodeGenerator::declareTypeImpl(AggregateType const* type) {
    // TODO: This does not handle nested structs
    typeNameMap.setName(type, type->name());
    if (type->isExtern()) {
        return;
    }
    ranges::for_each(type->elements(),
                     [&](auto elem) { declareType(elem.type); });
    auto item = makeItem();
    writeRaw("struct ", getName(type));
    auto scope = makeScope(ScopeType::CompoundStmt);
    for (auto [index, member]: type->elements() | enumerate) {
        auto item = makeItem();
        if (auto* arrayType = dyncast<ArrayType const*>(member.type)) {
            writeRaw(getName(arrayType->elementType()), " ", member.name, "[",
                     arrayType->count()->get(), "]");
        }
        else {
            writeRaw(getName(member.type), " ", member.name);
        }
        if (member.attrib) {
            writeRaw(" ", toSpelling(member.attrib));
        }
    }
}

void MetalCodeGenerator::declareTypeImpl(PointerType const* type) {
    declareType(type->baseType());
    typeNameMap.setName(type,
                        utl::strcat(getName(type->baseType()), " device",
                                    type->isConst() ? " const" : "", "*"));
}

void MetalCodeGenerator::declareTypeImpl(ArrayType const* type) {
    declareType(type->elementType());
    typeNameMap.setName(type, utl::strcat(getName(type->elementType()), "[",
                                          type->count()->get(), "]"));
}

void MetalCodeGenerator::beginFunction(Function const* F) {
    beginItem();
    switch (F->shaderKind()) {
    case ShaderKind::Fragment:
        writeRaw("fragment ");
        break;
    case ShaderKind::Vertex:
        writeRaw("vertex ");
        break;
    default:
        break;
    }
    writeRaw(getName(F->returnType()), " ", F->name());
    beginScope(ScopeType::FuncParams);
    for (auto& param: F->parameters()) {
        auto item = makeItem();
        writeRaw(getName(param.type()));
        switch (param.qualifier()) {
            using enum ParameterQualifier;
        case None:
            break;
        case DeviceConstRef:
            writeRaw(" device const&");
            break;
        case DeviceConstPointer:
            writeRaw(" device const*");
            break;
        }
        writeRaw(" ", getName(&param));
        if (param.attribute()) {
            writeRaw(" ", toSpelling(param.attribute()));
        }
    }
    endScope();
    beginScope(ScopeType::CompoundStmt);
}

void MetalCodeGenerator::endFunction() {
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

static int getBindIndex(MaterialShaderType shaderType,
                        GlobalMaterialBindingType bindingType) {
    return getGlobalMaterialBindingPoint(shaderType, bindingType).index;
}

void MetalCodeGenerator::defineVertexShaderFunction() {
#if 0
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
#endif
}

void MetalCodeGenerator::defineFragmentShaderFunction() {
#if 0
    using enum MaterialShadingModel;
    using enum GlobalMaterialBindingType;
    using enum MaterialShaderType;
    auto SM = graph.codegenParameters().shadingModel;
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
#endif
}

void MetalCodeGenerator::codegenImpl(FunctionCall const& value) {
    auto item = makeItem();
    writeRaw(getName(value.type()), " ", getName(&value), " = ");
    writeRaw(getName(value.function()));
    writeScope(ScopeType::Paran, [&] {
        for (auto* arg: value.arguments()) {
            writeItem(getName(arg));
        }
    });
}

static std::string_view getSourceRepr(ArithmeticInst::Operation operation) {
    using enum ArithmeticInst::Operation;
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

void MetalCodeGenerator::codegenImpl(ArithmeticInst const& value) {
    auto item = makeItem();
    writeRaw(getName(value.type()), " ", getName(&value), " = ",
             getName(value.LHS()), " ", getSourceRepr(value.operation()), " ",
             getName(value.RHS()));
}

static std::string_view getSourceRepr(CompareInst::Operation operation) {
    using enum CompareInst::Operation;
    switch (operation) {
    case Less:
        return "<";
    case LessEq:
        return "<=";
    case Greater:
        return ">";
    case GreaterEq:
        return ">=";
    case Equal:
        return "==";
    case NotEqual:
        return "!=";
    }
}

void MetalCodeGenerator::codegenImpl(CompareInst const& value) {
    auto item = makeItem();
    writeRaw(getName(value.type()), " ", getName(&value), " = ",
             getName(value.LHS()), " ", getSourceRepr(value.operation()), " ",
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

void MetalCodeGenerator::codegenImpl(MatrixBuilder const& value) {
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

void MetalCodeGenerator::codegenImpl(ArrayAccess const& inst) {
    auto item = makeItem();
    writeRaw(getName(inst.type()), " ", getName(&inst), " = ",
             getName(inst.array()), "[", getName(inst.index()), "]");
}

void MetalCodeGenerator::codegenImpl(DiscardFragment const& inst) {
    auto item = makeItem();
    writeRaw("if (", getName(inst.condition()), ") ");
    auto scope = makeScope(ScopeType::CompoundStmt);
    writeItem("discard_fragment()");
}

void MetalCodeGenerator::codegenImpl(ReturnInst const& ret) {
    writeItem("return ", getName(ret.returnValue()));
}

void MetalCodeGenerator::genName(Value const& value) {
    visit(value, [this](auto& value) {
        if constexpr (requires { genNameImpl(value); }) {
            valueNameMap.setName(&value, genNameImpl(value));
        }
        else {
            valueNameMap.setName(&value, value.name());
        }
    });
}

std::string MetalCodeGenerator::genNameImpl(IntConstant const& c) {
    return std::to_string(c.get());
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

std::string MetalCodeGenerator::genNameImpl(TextureConstant const&) {
    assert(false);
}

std::string MetalCodeGenerator::genNameImpl(RasterizerInput const&) {
    assert(false);
}

std::string MetalCodeGenerator::genNameImpl(BuiltinFunction const& F) {
    return std::string(toSpelling(F.kind()));
}

std::string MetalCodeGenerator::genNameImpl(ExtractVectorValue const& e) {
    std::stringstream sstr;
    sstr << getName(e.base()) << ".";
    for (int i = 0; i < e.numComponents(); ++i) {
        sstr << "xyzw"[e.indices()[i]];
    }
    return std::move(sstr).str();
}

std::string MetalCodeGenerator::genNameImpl(ExtractAggregateValue const& e) {
    auto* type = cast<AggregateType const*>(e.base()->type());
    return utl::strcat(getName(e.base()), ".", type->element(e.index()).name);
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
