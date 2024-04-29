#include "Poppy/Editor/EntitySelectionManager.h"

#include <range/v3/algorithm.hpp>

#include "Bloom/Core/Ranges.h"
#include "Bloom/Runtime/SceneSystem.h" // For UnloadSceneEvent

using namespace bloom;
using namespace poppy;
using namespace ranges::views;

EntitySelectionManager::EntitySelectionManager(bloom::Receiver receiver):
    bloom::Receiver(std::move(receiver)) {
    listen([this](UnloadSceneEvent event) {
        auto toRemove = entities() | filter([&](EntityHandle e) {
            return &e.scene() == event.scene;
        }) | ToSmallVector<>;
        ranges::for_each(toRemove, [&](EntityHandle e) { erase(e); });
    });
}
