#ifndef CodeGen_hpp
#define CodeGen_hpp

#include <memory>
#include <string>

#include "Bloom/MaterialCompiler/Fwd.h"

namespace bloom::matc {

class CodeGenerator {
public:
    virtual ~CodeGenerator() = default;

    virtual std::string run() = 0;

protected:
    CodeGenerator() = default;
};

std::unique_ptr<CodeGenerator> makeCodeGenerator(Context const& ctx,
                                                 MaterialGraph const& graph);

} // namespace bloom::matc

#endif /* CodeGen_hpp */
