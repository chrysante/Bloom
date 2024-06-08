#include "Bloom/MaterialCompiler//Values.h"

#include <magic_enum.hpp>
#include <range/v3/algorithm.hpp>
#include <range/v3/numeric.hpp>
#include <range/v3/view.hpp>
#include <utl/strcat.hpp>

#include "Bloom/Core/Ranges.h"
#include "Bloom/MaterialCompiler/Context.h"
#include "Bloom/MaterialCompiler/Types.h"
#include "Bloom/MaterialCompiler/Values.h"

using namespace bloom;
using namespace matc;
using namespace vml::short_types;
using namespace ranges::views;

Value::~Value() { detachAllUsers(); }

void Value::detachAllUsers() {
    for (auto [user, count]: _users) {
        user->killOperand(this);
    }
    _users.clear();
}

void Value::addUser(User* user) { ++_users[user]; }

void Value::removeUser(User* user) {
    auto itr = _users.find(user);
    assert(itr != _users.end());
    if (--itr->second == 0) {
        _users.erase(itr);
    }
}

User::~User() {
    for (auto* value: _operands) {
        value->removeUser(this);
    }
}

User::User(ValueNodeType nodeType, std::string name, Type const* type,
           std::vector<Value*> operands):
    Value(nodeType, std::move(name), type), _operands(std::move(operands)) {
    for (auto* value: _operands) {
        value->addUser(this);
    }
}

User::User(User const& rhs): Value(rhs), _operands(rhs._operands) {
    for (auto* value: _operands) {
        value->addUser(this);
    }
}

void User::setOperand(size_t index, Value* value) {
    assert(index < _operands.size());
    if (auto* old = _operands[index]) {
        old->removeUser(this);
    }
    value->addUser(this);
    _operands[index] = value;
}

void User::killOperand(Value* operand) {
    auto itr = ranges::remove(_operands, operand);
    _operands.erase(itr, _operands.end());
}

VectorConstant::VectorConstant(Context& ctx, std::string name,
                               int numComponents, float4 value):
    Constant(ValueNodeType::VectorConstant, std::move(name),
             ctx.vectorType(numComponents)),
    _value(value),
    numc(numComponents) {}

TextureConstant::TextureConstant(std::string name, TextureType const* type,
                                 AssetHandle asset):
    Constant(ValueNodeType::TextureConstant, std::move(name), type),
    _asset(asset) {}

static Type const* selectType(Context& ctx, RasterizerInputKind kind) {
    using enum RasterizerInputKind;
    switch (kind) {
    case TextureCoords:
        return ctx.vectorType(2);
    case WorldNormals:
        [[fallthrough]];
    case WorldTangents:
        [[fallthrough]];
    case WorldBinormals:
        [[fallthrough]];
    case WorldPosition:
        [[fallthrough]];
    case ModelNormals:
        return ctx.vectorType(3);
    case VertexColor:
        return ctx.vectorType(4);
    }
}

RasterizerInput::RasterizerInput(Context& ctx, std::string name,
                                 RasterizerInputKind kind):
    Constant(ValueNodeType::RasterizerInput, std::move(name),
             selectType(ctx, kind)),
    _kind(kind) {}

DynUniquePtr<Instruction> Instruction::clone() const {
    return visit(*this, [](auto& inst) { return inst.doClone(); });
}

DynUniquePtr<Instruction> Instruction::doClone() const {
    return visit(*this,
                 []<typename T>(T const& inst) -> DynUniquePtr<Instruction> {
        return allocate<T>(inst);
    });
}

Callable::Callable(ValueNodeType nodeType, std::string name,
                   FunctionType const* type,
                   List<FunctionParameter> parameters):
    Constant(nodeType, std::move(name), type),
    _parameters(std::move(parameters)) {}

Type const* Callable::returnType() const { return type()->returnType(); }

Function::Function(std::string name, FunctionType const* type,
                   List<FunctionParameter> parameters, List<Instruction> body):
    Callable(ValueNodeType::Function, std::move(name), type,
             std::move(parameters)),
    _body(std::move(body)) {}

namespace {

enum class SymType {
    Float,  /// Single floating point value
    Float3, /// Vector of three floating point values
    FloatN, /// Single floating point value or vector of 2-4 floating point
    /// values
    VectorN /// Vector of 2-4 floating point values
};

} // namespace

static Type const* deduceRetTypeImpl(Context& ctx, SymType symRet,
                                     std::span<SymType const> symArgs,
                                     std::span<Type const* const> args) {
    using enum SymType;
    if (symArgs.size() != args.size()) {
        return nullptr;
    }
    int N = -1;
    for (auto [symArg, arg]: zip(symArgs, args)) {
        switch (symArg) {
        case Float:
            if (arg != ctx.floatType()) {
                return nullptr;
            }
            break;
        case Float3:
            if (arg != ctx.float3Type()) {
                return nullptr;
            }
            break;
        case FloatN: {
            auto* vecType = dyncast<VectorType const*>(arg);
            if (!vecType) {
                return nullptr;
            }
            if (N != -1 && N != vecType->numComponents()) {
                return nullptr;
            }
            N = vecType->numComponents();
            break;
        }
        case VectorN: {
            auto* vecType = dyncast<VectorType const*>(arg);
            if (!vecType || vecType->numComponents() < 2) {
                return nullptr;
            }
            if (N != -1 && N != vecType->numComponents()) {
                return nullptr;
            }
            N = vecType->numComponents();
            break;
        }
        }
    }
    switch (symRet) {
    case Float:
        return ctx.floatType();
    case Float3:
        return ctx.float3Type();
    case FloatN:
        [[fallthrough]];
    case VectorN:
        if (N == -1) {
            return nullptr;
        }
        return ctx.vectorType(N);
    }
}

static Type const* deduceReturnType(Context& ctx, BuiltinFunctionKind function,
                                    std::span<Type const* const> args) {
    switch (function) {
        using enum SymType;
#define UNPACK(...) __VA_ARGS__
#define BL_MATGRAPH_BUILTIN(Name, Spelling, RetType, Args)                     \
    case BuiltinFunctionKind::Name:                                            \
        return deduceRetTypeImpl(ctx, RetType, { { UNPACK Args } }, args);
#include "Bloom/MaterialCompiler/Lists.def"
    }
}

static FunctionType const* deduceFunctionType(
    Context& ctx, BuiltinFunctionKind function,
    std::span<Type const* const> args) {
    auto* ret = deduceReturnType(ctx, function, args);
    if (!ret) {
        throw std::runtime_error("Invalid arguments for builtin function");
    }
    return ctx.functionType(ret, args);
}

static List<FunctionParameter> allocateParameters(
    std::span<Type const* const> args) {
    auto params = args | transform([index = 0](Type const* type) mutable {
        return new FunctionParameter(utl::strcat("_", index++), type, {}, {});
    });
    List<FunctionParameter> result;
    for (auto* param: params) {
        result.push_back(param);
    }
    return result;
}

BuiltinFunction::BuiltinFunction(Context& ctx, BuiltinFunctionKind kind,
                                 std::span<Type const* const> argTypes):
    Callable(ValueNodeType::BuiltinFunction,
             std::string(magic_enum::enum_name(kind)),
             deduceFunctionType(ctx, kind, argTypes),
             allocateParameters(argTypes)),
    _kind(kind) {}

struct FunctionCall::PrivArgs {
    static PrivArgs make(Context& ctx, std::string name,
                         BuiltinFunctionKind kind,
                         std::span<Value* const> arguments) {
        auto* function =
            ctx.builtinFunction(kind, arguments | transform(&Value::type) |
                                          ToSmallVector<>);
        return { .name = std::move(name),
                 .returnType = function->returnType(),
                 .operands = concat(single(function), arguments) |
                             ranges::to<std::vector<Value*>> };
    }

    static PrivArgs make(Context& ctx, std::string name,
                         BuiltinFunctionKind kind,
                         std::span<Type const* const> argumentTypes) {
        auto* function = ctx.builtinFunction(kind, argumentTypes);
        std::vector<Value*> operands(1 + argumentTypes.size());
        operands[0] = function;
        return { .name = std::move(name),
                 .returnType = function->returnType(),
                 .operands = std::move(operands) };
    }

    std::string name;
    Type const* returnType;
    std::vector<Value*> operands;
};

FunctionCall::FunctionCall(PrivArgs args):
    Instruction(ValueNodeType::FunctionCall, std::move(args.name),
                args.returnType, std::move(args.operands)) {}

FunctionCall::FunctionCall(Context& ctx, std::string name,
                           BuiltinFunctionKind kind,
                           std::span<Value* const> arguments):
    FunctionCall(PrivArgs::make(ctx, std::move(name), kind, arguments)) {}

FunctionCall::FunctionCall(Context& ctx, std::string name,
                           BuiltinFunctionKind kind,
                           std::span<Type const* const> argumentTypes):
    FunctionCall(PrivArgs::make(ctx, std::move(name), kind, argumentTypes)) {}

static void throwArithmeticTypeError() {
    throw std::runtime_error("Invalid operand types for arithmetic operation");
}

Type const* ArithmeticOperation::commonType(Type const* lhs, Type const* rhs) {
    auto* LHS = dyncast<VectorType const*>(lhs);
    auto* RHS = dyncast<VectorType const*>(rhs);
    if (!LHS || !RHS) {
        throwArithmeticTypeError();
    }
    if (LHS->numComponents() == 1) {
        return RHS;
    }
    if (RHS->numComponents() == 1) {
        return LHS;
    }
    if (LHS->numComponents() != RHS->numComponents()) {
        throwArithmeticTypeError();
    }
    return LHS;
}

TextureSampler::TextureSampler(Context& ctx, std::string name,
                               TextureConstant* texture, Value* coords,
                               SampleMode mode):
    Instruction(ValueNodeType::TextureSampler, std::move(name),
                ctx.float4Type(), { texture, coords }),
    _mode(mode) {}

static int accNumComponents(std::span<Value const* const> values) {
    return ranges::accumulate(values, 0, ranges::plus{},
                              [](Value const* value) {
        return cast<VectorType const*>(value->type())->numComponents();
    });
}

VectorBuilder::VectorBuilder(Context& ctx, std::string name,
                             std::vector<Value*> values):
    Instruction(ValueNodeType::VectorBuilder, std::move(name),
                ctx.vectorType(accNumComponents(values)), std::move(values)) {}

VectorBuilder::VectorBuilder(std::string name, VectorType const* type,
                             int numInputs):
    Instruction(ValueNodeType::VectorBuilder, std::move(name), type,
                std::vector<Value*>(numInputs)) {}

ExtractVectorValue::ExtractVectorValue(Context& ctx, std::string name,
                                       int index, Value* operand):
    Instruction(ValueNodeType::ExtractVectorValue, std::move(name),
                ctx.floatType(), { operand }),
    _index(index) {}

AggregateBuilder::AggregateBuilder(std::string name, AggregateType const* type,
                                   std::vector<Value*> values):
    Instruction(ValueNodeType::AggregateBuilder, std::move(name), type,
                std::move(values)) {}

AggregateBuilder::AggregateBuilder(std::string name, AggregateType const* type):
    AggregateBuilder(std::move(name), type,
                     std::vector<Value*>(type->elements().size())) {}

ExtractAggregateValue::ExtractAggregateValue(std::string name, Value* baseValue,
                                             int index):
    Instruction(ValueNodeType::ExtractAggregateValue, std::move(name),
                cast<AggregateType const*>(baseValue->type())
                    ->element(index)
                    .type,
                { baseValue }),
    _index(index) {}

ExtractAggregateValue::ExtractAggregateValue(std::string name,
                                             Type const* elementType,
                                             int index):
    Instruction(ValueNodeType::ExtractAggregateValue, std::move(name),
                elementType, std::vector<Value*>(1)),
    _index(index) {}

DiscardFragment::DiscardFragment(Context& ctx, std::string name,
                                 Value* condition):
    Instruction(ValueNodeType::DiscardFragment, std::move(name), ctx.voidType(),
                { condition }) {}

ReturnInst::ReturnInst(Context& ctx, std::string name, Value* retVal):
    Instruction(ValueNodeType::ReturnInst, std::move(name), ctx.voidType(),
                { retVal }) {}
