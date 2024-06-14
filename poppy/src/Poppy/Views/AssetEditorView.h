#ifndef POPPY_VIEWS_ASSETEDITORVIEW_H
#define POPPY_VIEWS_ASSETEDITORVIEW_H

#include <concepts>

#include "Bloom/Asset/Asset.h"
#include "Bloom/Core/Reference.h"
#include "Poppy/UI/View.h"

namespace poppy {

class AssetEditorViewBase: public View {
protected:
    using View::View;

    YAML::Node serialize() const override;
    void deserialize(YAML::Node) override;

private:
    virtual void _openAsset(bloom::Asset&) = 0;
    virtual void _setAsset(bloom::Reference<bloom::Asset>) = 0;
    virtual bloom::Asset* _getAsset() const = 0;
};

template <std::derived_from<bloom::Asset> A>
class AssetEditorView: public AssetEditorViewBase {
protected:
    using AssetEditorViewBase::AssetEditorViewBase;

    A* asset() const { return _asset.get(); }

    bloom::Reference<A> assetRef() const { return _asset; }

private:
    virtual void openAsset(A&) {};

    void _openAsset(bloom::Asset& asset) final {
        openAsset(bloom::dyncast<A&>(asset));
    }

    void receiveData(std::any data) final {
        if (auto* ref = std::any_cast<bloom::Reference<A>>(&data)) {
            _setAsset(*ref);
            openAsset(*_asset);
        }
    }

    void _setAsset(bloom::Reference<bloom::Asset> a) final {
        _asset = as<A>(a);
    }

    bloom::Asset* _getAsset() const final { return _asset.get(); }

    bloom::Reference<A> _asset;
};

} // namespace poppy

#endif // POPPY_VIEWS_ASSETEDITORVIEW_H
