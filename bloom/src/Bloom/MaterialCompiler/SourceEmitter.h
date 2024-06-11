#ifndef BLOOM_MATERIALCOMPILER_SOURCEEMITTER_H
#define BLOOM_MATERIALCOMPILER_SOURCEEMITTER_H

#include <sstream>
#include <string>
#include <string_view>
#include <tuple>

#include <utl/stack.hpp>

#include "Bloom/MaterialCompiler/Debug.h"

namespace bloom::matc {

class SourceEmitter;

enum class IncludeType { System, Local };

enum class ScopeType { Paran, FuncParams, Bracket, CompoundStmt, BraceList };

template <typename Derived>
struct GuardBase {
    explicit GuardBase(SourceEmitter* emitter): emitter(emitter) {}

    ~GuardBase() {
        if (!didPop()) {
            pop();
        }
    }

    void pop() {
        VERIFY(!_didPop, "Have nothing to pop");
        static_cast<Derived*>(this)->doPop();
        _didPop = true;
    }

    bool didPop() const { return _didPop; }

protected:
    SourceEmitter* emitter;

private:
    bool _didPop = false;
};

struct ScopeGuard: GuardBase<ScopeGuard> {
    using GuardBase::GuardBase;

private:
    template <typename>
    friend struct GuardBase;

    void doPop();
};

struct ItemGuard: GuardBase<ItemGuard> {
    using GuardBase::GuardBase;

private:
    template <typename>
    friend struct GuardBase;

    void doPop();
};

class SourceEmitter {
public:
    explicit SourceEmitter(ScopeType baseScope) {
        scopeStack.push(Scope{ baseScope });
    }

    void writeRaw(auto const&... args) { ((sstr << args), ...); }

    void beginItem() {
        using enum ScopeType;
        VERIFY(!scopeStack.empty(), "Cannot begin item without scope");
        auto scope = scopeStack.top();
        switch (scope.type) {
        case Paran:
            [[fallthrough]];
        case Bracket:
            if (scope.didWriteItem) {
                writeRaw(", ");
            }
            break;
        case FuncParams:
            if (scope.didWriteItem) {
                writeRaw(", ");
            }
            endLine();
            beginLine();
            break;
        case CompoundStmt:
            [[fallthrough]];
        case BraceList:
            beginLine();
            break;
        }
    }

    void endItem() {
        using enum ScopeType;
        auto& scope = scopeStack.top();
        switch (scope.type) {
        case Paran:
            [[fallthrough]];
        case FuncParams:
            [[fallthrough]];
        case Bracket:
            break;
        case CompoundStmt:
            writeRaw(";");
            endLine();
            break;
        case BraceList:
            writeRaw(",");
            endLine();
            break;
        }
        scope.didWriteItem = true;
        /// In the global scope we always leave one black line between items
        if (isGlobalScope()) {
            endLine();
        }
    }

    ItemGuard makeItem() {
        beginItem();
        return ItemGuard(this);
    }

    void writeInclude(IncludeType type, std::string_view path) {
        VERIFY(isGlobalScope(), "Includes must be written in global scope");
        auto symbols = bracketSymbols(type);
        writeRaw("#include ", symbols[0], path, symbols[1]);
        endLine();
        endLine();
    }

    void writeItem(std::invocable auto fn) {
        beginItem();
        std::invoke(fn);
        endItem();
    }

    void writeItem(auto const&... args) {
        writeItem([&] { writeRaw(args...); });
    }

    void beginScope(ScopeType type) {
        scopeStack.push(Scope{ type });
        writeRaw(bracketSymbols(type)[0]);
        if (wantNewLine(type)) {
            endLine();
        }
    }

    void endScope() {
        using enum ScopeType;
        auto scope = scopeStack.pop();
        auto symbol = bracketSymbols(scope.type)[1];
        switch (scope.type) {
        case Paran:
            [[fallthrough]];
        case Bracket:
            writeRaw(symbol);
            break;
        case FuncParams:
            writeRaw(symbol);
            endLine();
            break;
        case CompoundStmt:
            beginLine();
            writeRaw(symbol);
            break;
        case BraceList:
            beginLine();
            writeRaw(symbol);
            break;
        }
    }

    ScopeGuard makeScope(ScopeType type) {
        beginScope(type);
        return ScopeGuard(this);
    }

    void writeScope(ScopeType type, std::invocable auto fn) {
        beginScope(type);
        std::invoke(fn);
        endScope();
    }

    void writeScope(ScopeType type, auto const&... args) {
        writeScope(type, [&] { writeRaw(args...); });
    }

    std::string takeSource() { return std::move(sstr).str(); }

private:
    struct Scope {
        ScopeType type;
        bool didWriteItem = false;
    };

    void beginLine() {
        for (size_t i = 1; i < scopeStack.size(); ++i) {
            sstr << "    ";
        }
    }

    void endLine() { sstr << "\n"; }

    static std::array<std::string_view, 2> bracketSymbols(ScopeType type) {
        using enum ScopeType;
        switch (type) {
        case Paran:
            [[fallthrough]];
        case FuncParams:
            return { "(", ")" };
        case Bracket:
            return { "[", "]" };
        case CompoundStmt:
            [[fallthrough]];
        case BraceList:
            return { "{", "}" };
        }
    }

    static std::array<std::string_view, 2> bracketSymbols(IncludeType type) {
        using enum IncludeType;
        switch (type) {
        case System:
            return { "<", ">" };
        case Local:
            return { "\"", "\"" };
        }
    }

    static bool wantNewLine(ScopeType type) {
        using enum ScopeType;
        return type == CompoundStmt || type == BraceList;
    }

    bool isGlobalScope() const { return scopeStack.size() == 1; }

    utl::stack<Scope> scopeStack;
    std::stringstream sstr;
};

inline void ScopeGuard::doPop() { emitter->endScope(); }

inline void ItemGuard::doPop() { emitter->endItem(); }

} // namespace bloom::matc

#endif // BLOOM_MATERIALCOMPILER_SOURCEEMITTER_H
