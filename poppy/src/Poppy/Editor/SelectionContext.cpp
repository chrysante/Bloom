#include "Poppy/Editor/SelectionContext.hpp"

#include "Bloom/Runtime/SceneSystem.hpp"

using namespace bloom;

using namespace poppy;

SelectionContext::SelectionContext(bloom::Reciever reciever):
    bloom::Reciever(std::move(reciever)) {
    listen([this](UnloadSceneEvent event) {
        mEntities.erase(std::remove_if(
                            mEntities.begin(),
                            mEntities.end(),
                            [&](EntityHandle entity) {
            return &entity.scene() == event.scene;
                            }),
                        mEntities.end());
    });
}

void SelectionContext::select(bloom::EntityHandle entity) {
    clear();
    mEntities.push_back(entity);
}

void SelectionContext::addSelect(bloom::EntityHandle entity) {
    if (isSelected(entity)) {
        return;
    }
    mEntities.push_back(entity);
}

void SelectionContext::deselect(bloom::EntityHandle entity) {
    auto itr = std::find(mEntities.begin(), mEntities.end(), entity);
    if (itr == mEntities.end()) {
        return;
    }
    mEntities.erase(itr);
}

void SelectionContext::clear() { mEntities.clear(); }

bool SelectionContext::isSelected(bloom::EntityHandle entity) const {
    return std::find(mEntities.begin(), mEntities.end(), entity) !=
           mEntities.end();
}
