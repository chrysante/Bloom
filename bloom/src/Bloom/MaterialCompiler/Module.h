#ifndef BLOOM_MATERIALCOMPILER_MODULE_H
#define BLOOM_MATERIALCOMPILER_MODULE_H

#include <vector>

#include <range/v3/view.hpp>

#include "Bloom/Core/Utils.h"
#include "Bloom/MaterialCompiler/Values.h"

namespace bloom::matc {

/// Set of shader functions for a material
class Module {
public:
    /// Adds the function \p F to this module
    Function* addFunction(DynUniquePtr<Function> F);

    /// \Returns a view over the functions in this module
    auto functions() const {
        return _functions | ranges::views::transform(ToAddress<Function const>);
    }

private:
    std::vector<DynUniquePtr<Function>> _functions;
};

} // namespace bloom::matc

#endif // BLOOM_MATERIALCOMPILER_MODULE_H
