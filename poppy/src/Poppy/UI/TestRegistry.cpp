#include "Poppy/UI/TestRegistry.h"

#include <vector>

using namespace poppy;

static std::vector<TestRegistryEntry> gTestRegistry;

TestRegistryEntry& internal::registerUITest(std::string name,
                                            std::string category,
                                            char const* file, int line) {
    gTestRegistry.push_back({ .name = std::move(name),
                              .category = std::move(category),
                              file,
                              line });
    return gTestRegistry.back();
}

std::span<TestRegistryEntry const> poppy::getRegisteredTests() {
    return gTestRegistry;
}
