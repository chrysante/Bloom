#include "Bloom/MaterialCompiler/CodeGen.h"

#include <span>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <utl/hashtable.hpp>
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

    std::string operator()(T const& elem) {
        auto itr = map.find(elem);
        if (itr != map.end()) {
            return itr->second;
        }
        return map.insert({ elem, generateNewName() }).first->second;
    }

    void setName(T const& elem, std::string name) {
        map[elem] = std::move(name);
    }

private:
    std::string generateNewName() { return utl::strcat(prefix, index++); }

    std::string prefix;
    int index = 0;
    utl::hashmap<T, std::string> map;
};

struct Data {};

struct MetalCodeGenerator: CodeGenerator, Data, Indenter {
    MetalCodeGenerator(Context const& ctx, MaterialGraph const& graph):
        Indenter(sstr), ctx(ctx), graph(graph) {}

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

    void codegenImpl(Value const&) { assert(false); }
    void codegenImpl(VectorConstant const& value);
    void codegenImpl(AggregateBuilder const& value);

    Context const& ctx;
    MaterialGraph const& graph;

    Type const* returnType;
    NameGenerator<Type const*> typeNameGen{ "Type_" };
    NameGenerator<Value const*> valueNameGen{ "Value_" };
    std::stringstream sstr;
};

} // namespace

std::unique_ptr<CodeGenerator> matc::makeCodeGenerator(
    Context const& ctx, MaterialGraph const& graph) {
    return std::make_unique<MetalCodeGenerator>(ctx, graph);
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
    returnType = graph.root()->type();
    typeNameGen.setName(returnType, "MaterialType");
    auto order = topsort(graph.values() | ranges::to<std::vector>);
    for (auto* type: ctx.types()) {
        declareType(type);
    }
    beginFunction();
    for (auto* value: order) {
        codegen(*value);
    }
    endFunction();
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
    // TODO: This does not handle nested structs
    sstr << "struct " << typeNameGen(type);
    pushScope();
    for (auto [index, member]: type->elements() | enumerate) {
        sstr << beginLine() << typeNameGen(member) << " _" << index << ";\n";
    }
    popScope(";");
}

void MetalCodeGenerator::beginFunction() {
    sstr << "static " << typeNameGen(returnType) << " " << "material() ";
    pushScope();
}

void MetalCodeGenerator::endFunction() {
    sstr << beginLine() << "return " << valueNameGen(graph.root()) << ";\n";
    popScope();
}

void MetalCodeGenerator::codegenImpl(VectorConstant const& value) {
    sstr << beginLine() << typeNameGen(value.type()) << " "
         << valueNameGen(&value);
    if (value.numComponents() == 1) {
        sstr << " = " << value.get().x << ";\n";
    }
    else {
        pushScope();
        sstr << beginLine();
        for (int i = 0; i < value.numComponents(); ++i) {
            if (i != 0) {
                sstr << ", ";
            }
            sstr << value.get()[i];
        }
        sstr << "\n";
        popScope(";");
    }
}

void MetalCodeGenerator::codegenImpl(AggregateBuilder const& value) {
    sstr << beginLine() << typeNameGen(value.type()) << " "
         << valueNameGen(&value);
    pushScope();
    for (auto* operand: value.operands()) {
        sstr << beginLine() << valueNameGen(operand) << ",\n";
    }
    popScope(";");
}
