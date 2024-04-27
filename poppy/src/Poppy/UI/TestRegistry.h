#ifndef POPPY_UI_TESTREGISTRY_H
#define POPPY_UI_TESTREGISTRY_H

#include <span>
#include <string>

#include <imgui_te_context.h>

namespace poppy {

/// UI test descriptor. This will be passed to the ImGui test engine on
/// initialization
struct TestRegistryEntry {
    using TestFuncType = void(ImGuiTestContext*);

    std::string name;
    std::string category;
    char const* file;
    int line;
    TestFuncType* testFunction = nullptr;
};

namespace internal {

/// Adds a test to the registry. This is meant to be used by the `UI_TEST()`
/// macro
TestRegistryEntry& registerUITest(std::string name, std::string category,
                                  char const* file, int line);

} // namespace internal

/// Statically registers a UI test. Meant to be "called" at source file scope
#define UI_TEST(ViewName, TestName, Ctx)                                       \
    static TestRegistryEntry::TestFuncType* const UTL_UNIQUE_NAME(UITest) =    \
        ::poppy::internal::registerUITest(TestName, ViewName, __FILE__,        \
                                          __LINE__)                            \
            .testFunction = [](Ctx)

std::span<TestRegistryEntry const> getRegisteredTests();

} // namespace poppy

#endif // POPPY_UI_TESTREGISTRY_H
