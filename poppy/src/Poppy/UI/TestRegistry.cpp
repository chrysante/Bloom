#include "Poppy/UI/TestRegistry.h"

#include <vector>

#include "Poppy/UI/ViewManager.h"

using namespace poppy;
using namespace bloom;

void TestBase::openView(std::string name) const {
    emitter().dispatch(DispatchToken::Now,
                       ViewManager::OpenCommand{ std::move(name) });
    _testCtx->Yield();
}

static std::vector<TestRegistryEntry> gTestRegistry;

TestRegistryEntry& poppy::internal::registerUITest(std::string name,
                                                   std::string category,
                                                   char const* file, int line,
                                                   TestBase* testObject) {
    gTestRegistry.push_back({ .name = std::move(name),
                              .category = std::move(category),
                              file,
                              line,
                              .testObject = testObject });
    return gTestRegistry.back();
}

std::span<TestRegistryEntry> poppy::getRegisteredTests() {
    return gTestRegistry;
}
