#ifndef POPPY_VIEWS_TEXTUREVIEWER_H
#define POPPY_VIEWS_TEXTUREVIEWER_H

#include <memory>

#include "Bloom/Asset/Fwd.h"
#include "Bloom/Core/Reference.h"

namespace poppy {

class TextureViewer {
public:
    enum class DisplayMode { Expanded, Compact };

    struct Options {
        DisplayMode displayMode = DisplayMode::Expanded;
        float rounding = 0;
    };

    TextureViewer();
    explicit TextureViewer(bloom::Reference<bloom::Texture2D> texture,
                           Options options);
    TextureViewer(TextureViewer&&) noexcept;
    TextureViewer& operator=(TextureViewer&&) noexcept;
    ~TextureViewer();

    void display();

    bloom::Texture2D const* getTexture();

    struct Data;

private:
    std::unique_ptr<Data> data;
};

} // namespace poppy

#endif // POPPY_VIEWS_TEXTUREVIEWER_H
