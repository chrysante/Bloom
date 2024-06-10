#include "Bloom/MaterialCompiler//Values.h"

#include <magic_enum.hpp>
#include <range/v3/algorithm.hpp>
#include <range/v3/numeric.hpp>
#include <range/v3/view.hpp>
#include <utl/overload.hpp>
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

void Value::replace(Value* other) {
    auto users = _users | keys | ToSmallVector<>;
    for (auto* user: users) {
        user->replaceOperand(this, other);
    }
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
        if (value) {
            value->removeUser(this);
        }
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

void User::replaceOperand(Value* existing, Value* replacement) {
    for (auto& value: _operands) {
        if (value == existing) {
            existing->removeUser(this);
            replacement->addUser(this);
            value = replacement;
        }
    }
}

void User::killOperand(Value* operand) {
    ranges::replace(_operands, operand, nullptr);
}

VectorConstant::VectorConstant(Context& ctx, std::string name,
                               int numComponents, float4 value):
    Constant(ValueNodeType::VectorConstant, std::move(name),
             ctx.vectorType(numComponents)),
    _value(value),
    numc(numComponents) {}

IntConstant::IntConstant(std::string name, IntType const* type,
                         long long value):
    Constant(ValueNodeType::IntConstant, std::move(name), type),
    _value(value) {}

TextureConstant::TextureConstant(std::string name, TextureType const* type,
                                 AssetHandle asset):
    Constant(ValueNodeType::TextureConstant, std::move(name), type),
    _asset(asset) {}

static Type const* selectType(Context& ctx, RasterizerInputKind kind) {
    using enum RasterizerInputKind;
    switch (kind) {
    case ClipPosition:
        return ctx.float4Type();
    case TextureCoords:
        return ctx.float2Type();
    case WorldNormals:
        [[fallthrough]];
    case WorldTangents:
        [[fallthrough]];
    case WorldBinormals:
        [[fallthrough]];
    case WorldPosition:
        [[fallthrough]];
    case ModelNormals:
        return ctx.float3Type();
    case VertexColor:
        return ctx.float4Type();
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

FunctionType const* Callable::computeType(Context& ctx,
                                          Type const* returnType) {
    auto args = parameters() | transform(&Value::type) | ToSmallVector<>;
    _type = ctx.functionType(returnType, args);
    return type();
}

void Callable::setParameters(List<FunctionParameter> params) {
    assert(_parameters.empty());
    _parameters = std::move(params);
}

Function::Function(std::string name, FunctionType const* type,
                   ShaderKind shaderKind, List<FunctionParameter> parameters,
                   List<Instruction> body):
    Callable(ValueNodeType::Function, std::move(name), type,
             std::move(parameters)),
    _body(std::move(body)),
    _shaderKind(shaderKind) {
    for (auto& inst: *this) {
        inst._function = this;
    }
}

Instruction* Function::push_front(Instruction* inst) {
    _body.push_front(inst);
    inst->_function = this;
    return inst;
}
Instruction* Function::push_back(Instruction* inst) {
    _body.push_back(inst);
    inst->_function = this;
    return inst;
}

ReturnInst const* Function::getReturn() const {
    if (_body.empty()) {
        return nullptr;
    }
    return dyncast<ReturnInst const*>(&_body.back());
}

static Type const* deduceRetTypeImpl(Context& ctx, SymbolicType symRet,
                                     std::span<SymbolicType const> symArgs,
                                     std::span<Type const* const> args) {
    using enum SymbolicType;
    if (symArgs.size() != args.size()) {
        return nullptr;
    }
    int N = -1;
    for (auto [symArg, arg]: zip(symArgs, args)) {
        switch (symArg) {
        case Float:
            [[fallthrough]];
        case Float2:
            [[fallthrough]];
        case Float3:
            [[fallthrough]];
        case Float4:
            if (arg != toConcreteType(ctx, symArg)) {
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
        [[fallthrough]];
    case Float2:
        [[fallthrough]];
    case Float3:
        [[fallthrough]];
    case Float4:
        return toConcreteType(ctx, symRet);
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
        using enum SymbolicType;
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

List<FunctionParameter> matc::allocateParameters(
    std::span<ParameterDescriptor const> args) {
    auto params = args |
                  transform([index = 0](ParameterDescriptor desc) mutable {
        if (desc.name.empty()) {
            desc.name = utl::strcat("_", index++);
        }
        return new FunctionParameter(desc);
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
             allocateParameters(argTypes | transform([](auto* type) {
                                    return ParameterDescriptor{ .type = type };
                                }) |
                                ToSmallVector<>)),
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

FunctionCall::FunctionCall(std::string name, Callable* function,
                           std::span<Value* const> arguments):
    FunctionCall(
        { .name = std::move(name),
          .returnType = function->returnType(),
          .operands = concat(single(static_cast<Value*>(function)), arguments) |
                      ranges::to<std::vector> }) {}

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

Type const* ArithmeticInst::commonType(Context& ctx, Type const* lhs,
                                       Type const* rhs) {
    if (!lhs || !rhs) {
        throwArithmeticTypeError();
    }
    // clang-format off
    return visit<Type const*>(*lhs, *rhs, utl::overload{
        [&](VectorType const& LHS, VectorType const& RHS) {
            if (LHS.numComponents() == 1) {
                return &RHS;
            }
            if (RHS.numComponents() == 1) {
                return &LHS;
            }
            if (LHS.numComponents() != RHS.numComponents()) {
                throwArithmeticTypeError();
            }
            return &LHS;
        },
        [&](MatrixType const& LHS, MatrixType const& RHS) {
            if (LHS.columns() != RHS.rows()) {
                throwArithmeticTypeError();
            }
            return ctx.matrixType(LHS.rows(), RHS.columns());
        },
        [&](MatrixType const& LHS, VectorType const& RHS) {
            if (LHS.columns() != RHS.numComponents()) {
                throwArithmeticTypeError();
            }
            return ctx.vectorType(LHS.rows());
        },
        [&](VectorType const& LHS, MatrixType const& RHS) {
            if (LHS.numComponents() != RHS.columns()) {
                throwArithmeticTypeError();
            }
            return ctx.vectorType(RHS.columns());
        },
        [&](Type const&, Type const&) {
            throwArithmeticTypeError();
        }
    }); // clang-format on
}

CompareInst::CompareInst(Context& ctx, std::string name, Operation operation,
                         Value* lhs, Value* rhs):
    Instruction(ValueNodeType::CompareInst, std::move(name), ctx.boolType(),
                { lhs, rhs }),
    _operation(operation) {}

TextureSampler::TextureSampler(Context& ctx, std::string name, Value* texture,
                               Value* coords, SampleMode mode):
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

MatrixBuilder::MatrixBuilder(Context& ctx, std::string name,
                             std::vector<Value*> columns):
    Instruction(ValueNodeType::MatrixBuilder, std::move(name),
                ctx.matrixType(cast<VectorType const*>(columns[0]->type())
                                   ->numComponents(),
                               (int)columns.size()),
                std::move(columns)) {}

MatrixBuilder::MatrixBuilder(std::string name, MatrixType const* type,
                             int numInputs):
    Instruction(ValueNodeType::MatrixBuilder, std::move(name), type,
                std::vector<Value*>(numInputs)) {}

AggregateBuilder::AggregateBuilder(std::string name, AggregateType const* type,
                                   std::vector<Value*> values):
    Instruction(ValueNodeType::AggregateBuilder, std::move(name), type,
                std::move(values)) {}

AggregateBuilder::AggregateBuilder(std::string name, AggregateType const* type):
    AggregateBuilder(std::move(name), type,
                     std::vector<Value*>(type->elements().size())) {}

ExtractVectorValue::ExtractVectorValue(Context& ctx, std::string name,
                                       Value* operand, vml::int4 indices,
                                       int numComponents):
    Instruction(ValueNodeType::ExtractVectorValue, std::move(name),
                ctx.vectorType(numComponents), { operand }),
    _indices(indices),
    _numComponents(numComponents) {}

static int getNumComponents(int indexA, int indexB, int indexC, int indexD) {
    int indices[] = { indexA, indexB, indexC, indexD };
    int num = 0;
    for (int i: indices) {
        if (i < 0) {
            break;
        }
        ++num;
    }
    return num;
}

ExtractVectorValue::ExtractVectorValue(Context& ctx, std::string name,
                                       Value* operand, int indexA, int indexB,
                                       int indexC, int indexD):
    ExtractVectorValue(ctx, std::move(name), operand,
                       { indexA, indexB, indexC, indexD },
                       getNumComponents(indexA, indexB, indexC, indexD)) {}

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

static Type const* getArrayElemType(Type const* type) {
    return visit<Type const*>(*type,
                              utl::overload{ [](PointerType const& type) {
        return type.baseType();
    }, [](ArrayType const& type) {
        return type.elementType();
    }, [](Type const&) { assert(false); } });
}

ArrayAccess::ArrayAccess(std::string name, Value* array, Value* index):
    Instruction(ValueNodeType::ArrayAccess, std::move(name),
                getArrayElemType(array->type()), { array, index }) {}

DiscardFragment::DiscardFragment(Context& ctx, std::string name,
                                 Value* condition):
    Instruction(ValueNodeType::DiscardFragment, std::move(name), ctx.voidType(),
                { condition }) {}

ReturnInst::ReturnInst(Context& ctx, std::string name, Value* retVal):
    Instruction(ValueNodeType::ReturnInst, std::move(name), ctx.voidType(),
                { retVal }) {}
