#include "Bloom/Graphics/RenderGraph/Node.h"

using namespace bloom;

void RGPassNode::declareInput(RGBufferDescription const&) {
    BL_UNIMPLEMENTED();
}

void RGPassNode::declareInput(RGTextureDescription const&) {
    BL_UNIMPLEMENTED();
}

void RGPassNode::declareInput(RGSceneDescription const&) { BL_UNIMPLEMENTED(); }

void RGPassNode::declareOutput(RGBufferDescription const&) {
    BL_UNIMPLEMENTED();
}

void RGPassNode::declareOutput(RGTextureDescription const&) {
    BL_UNIMPLEMENTED();
}
