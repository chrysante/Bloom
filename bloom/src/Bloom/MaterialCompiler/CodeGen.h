#ifndef CodeGen_hpp
#define CodeGen_hpp

#include <memory>
#include <string>

#include "Bloom/MaterialCompiler/Fwd.h"

namespace bloom::matc {

class CodeGenerator {
public:
    virtual ~CodeGenerator() = default;

    virtual std::string run(MaterialGraph const& graph) = 0;

protected:
    CodeGenerator() = default;
};

std::unique_ptr<CodeGenerator> makeCodeGenerator();

} // namespace bloom::matc

#endif /* CodeGen_hpp */
