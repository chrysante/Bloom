#include "Bloom/MaterialCompiler/CodeGen.h"

#include <span>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <utl/hashtable.hpp>
#include <utl/strcat.hpp>

#include "Bloom/MaterialCompiler/MaterialGraph.h"
#include "Bloom/MaterialCompiler/Values.h"

using namespace bloom;
using namespace matc;

namespace {

struct MetalCodeGenerator: CodeGenerator {
    std::string run(MaterialGraph const& graph) override;

    void codegen(Value const& value) {
        visit(value, [this](auto& value) { codegenImpl(value); });
    }

    void codegenImpl(Value const&) { assert(false); }
    void codegenImpl(FloatConstant const& value);
    void codegenImpl(AggregateBuilder const& value);

    std::string getName(Value const& value) {
        auto itr = nameMap.find(&value);
        if (itr != nameMap.end()) {
            return itr->second;
        }
        return nameMap.insert({ &value, generateNewName() }).first->second;
    }

    std::string generateNewName() { return utl::strcat("_", nameIndex++); }

    int nameIndex = 0;
    utl::hashmap<Value const*, std::string> nameMap;
    std::stringstream sstr;
};

} // namespace

std::unique_ptr<CodeGenerator> matc::makeCodeGenerator() {
    return std::make_unique<MetalCodeGenerator>();
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

std::string MetalCodeGenerator::run(MaterialGraph const& graph) {
    *this = {};
    auto order = topsort(graph.values() | ranges::to<std::vector>);
    for (auto* value: order) {
        codegen(*value);
    }
    return std::move(sstr).str();
}

void MetalCodeGenerator::codegenImpl(FloatConstant const& value) {
    sstr << "float " << getName(value) << " = " << value.get() << ";\n";
}

void MetalCodeGenerator::codegenImpl(AggregateBuilder const& value) {
    sstr << "TYPE " << getName(value) << " = {\n";
    for (auto* operand: value.operands()) {
        sstr << "    " << getName(*operand) << ",\n";
    }
    sstr << "};\n";
}
