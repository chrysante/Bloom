#include "Bloom/GPU/HardwareDevice.hpp"

#include "Bloom/Platform/Metal/MetalDevice.h"

using namespace bloom;

std::unique_ptr<HardwareDevice> HardwareDevice::create(RenderAPI api) {
    switch (api) {
    case RenderAPI::metal:
        return createMetalDevice();

    default:
        return nullptr;
    }
}
