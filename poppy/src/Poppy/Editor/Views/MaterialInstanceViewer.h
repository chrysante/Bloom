#ifndef POPPY_EDITOR_VIEWS_MATERIALINSTANCEVIEWER_H
#define POPPY_EDITOR_VIEWS_MATERIALINSTANCEVIEWER_H

#include "Bloom/Core/Core.h"
#include "Poppy/UI/View.h"

namespace bloom {

class MaterialInstance;

}

namespace poppy {

class MaterialInstanceViewer: public View {
public:
    void frame() override;

    void setMaterialInstance(bloom::Reference<bloom::MaterialInstance>);

private:
    bloom::Reference<bloom::MaterialInstance> inst;
};

} // namespace poppy

#endif // POPPY_EDITOR_VIEWS_MATERIALINSTANCEVIEWER_H
