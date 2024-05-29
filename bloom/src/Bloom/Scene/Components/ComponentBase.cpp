#include "Bloom/Scene/Components/ComponentBase.h"

using namespace bloom;

char const* bloom::componentName(ComponentKind kind) {
    switch (kind) {
#define BLOOM_COMPONENT_DEF(Type, Name)                                        \
    case ComponentKind::Type:                                                  \
        return Name;
#include "Bloom/Scene/Components/Components.def"
    }
}
