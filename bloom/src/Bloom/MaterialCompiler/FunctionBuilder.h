#ifndef FunctionBuilder_hpp
#define FunctionBuilder_hpp

#include <cassert>
#include <concepts>
#include <span>

#include <utl/stack.hpp>

#include "Bloom/MaterialCompiler/Context.h"
#include "Bloom/MaterialCompiler/Values.h"

namespace bloom::matc {

/// Helper class to simplify adding instructions to a function
class FunctionBuilder {
public:
    using iterator = Function::iterator;
    using const_iterator = Function::const_iterator;

    explicit FunctionBuilder(Context& ctx, Function& F): ctx(ctx), F(&F) {
        pushInsertPoint(F.end());
    }

    Context& context() const { return ctx; }

    Function* function() const { return F; }

    Instruction* insert(Instruction* inst) {
        return F->insert(insertPoint(), inst);
    }

    template <std::derived_from<Instruction> Inst>
    Inst* insert(Inst* inst) {
        return cast<Inst*>(insert(static_cast<Instruction*>(inst)));
    }

    template <std::derived_from<Instruction> Inst, typename... Args>
        requires std::constructible_from<Inst, Args...>
    Inst* insert(Args&&... args) {
        return insert(new Inst(std::forward<Args>(args)...));
    }

    template <std::derived_from<Instruction> Inst, typename... Args>
        requires std::constructible_from<Inst, Context&, Args...>
    Inst* insert(Args&&... args) {
        return insert(new Inst(ctx, std::forward<Args>(args)...));
    }

    FunctionCall* callFunction(std::string name, Callable* callee,
                               std::span<Value* const> args);

    const_iterator insertPoint() const { return insertPointStack.top(); }

    void pushInsertPoint(const_iterator itr) { insertPointStack.push(itr); }

    void popInsertPoint() { insertPointStack.pop(); }

private:
    Context& ctx;
    Function* F = nullptr;
    utl::stack<const_iterator> insertPointStack;
};

} // namespace bloom::matc

#endif /* FunctionBuilder_hpp */
