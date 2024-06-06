#ifndef BLOOM_MATERIALCOMPILER_FUNCTION_H
#define BLOOM_MATERIALCOMPILER_FUNCTION_H

#include <span>
#include <vector>

#include "Bloom/MaterialCompiler/Fwd.h"

namespace bloom::matc {

class Function {
public:
    Function(std::string name, std::vector<FunctionParameter> parameters,
             std::vector<Value*> body):
        _name(std::move(name)), _params(std::move(parameters)), _body(body) {}

    std::string const& name() const { return _name; }

    Type const* returnType() const;

    std::span<FunctionParameter const> parameters() const { return _params; }

    std::span<Value* const> body() const { return _body; }

    Value* root() const;

private:
    std::string _name;
    std::vector<FunctionParameter> _params;
    std::vector<Value*> _body;
};

} // namespace bloom::matc

#endif // BLOOM_MATERIALCOMPILER_FUNCTION_H
