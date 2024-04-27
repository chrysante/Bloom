#ifndef POPPY_UI_TESTREGISTRY_H
#define POPPY_UI_TESTREGISTRY_H

#include <span>
#include <string>

#include <imgui_te_context.h>
#include <utl/common.hpp>

#include "Bloom/Application/MessageSystem.h"

namespace poppy {

class TestBase {
public:
    struct Impl;

protected:
    /// Sends an `OpenCommand` to the view manager and yields
    void openView(std::string name) const;

    ImGuiTestContext& context() const { return *_testCtx; };

    bloom::Emitter& emitter() const { return *_emitter; }

    virtual ~TestBase() = default;

private:
    virtual void doRun() const = 0;

    ImGuiTestContext* _testCtx;
    bloom::Emitter* _emitter;
};

/// UI test descriptor. This will be passed to the ImGui test engine on
/// initialization
struct TestRegistryEntry {
    std::string name;
    std::string category;
    char const* file;
    int line;
    TestBase* testObject;
};

namespace internal {

/// Adds a test to the registry. This is meant to be used by the `UI_TEST()`
/// macro
TestRegistryEntry& registerUITest(std::string name, std::string category,
                                  char const* file, int line,
                                  TestBase* testObject);

} // namespace internal

/// Statically registers a UI test. Meant to be "called" at source file scope
#define UI_TEST(ViewName, TestName)                                            \
    namespace {                                                                \
    struct UTL_UNIQUE_NAME(UITestClass): ::poppy::TestBase {                   \
        void doRun() const override;                                           \
    };                                                                         \
    }                                                                          \
    UTL_STATIC_INIT {                                                          \
        ::poppy::internal::registerUITest(TestName, ViewName, __FILE__,        \
                                          __LINE__,                            \
                                          new UTL_UNIQUE_NAME(UITestClass)()); \
    };                                                                         \
    void UTL_UNIQUE_NAME(UITestClass)::doRun() const

/// \Returns a view over all statically registered tests. This is deliberately
/// not const because the `emitter` field still needs to be set
std::span<TestRegistryEntry> getRegisteredTests();

} // namespace poppy

#endif // POPPY_UI_TESTREGISTRY_H
