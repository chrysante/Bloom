#include "Bloom/GPU/HardwareDevice.h"

#include "Bloom/Platform/Metal/MetalObjects.h"

using namespace bloom;

Reference<HardwareDevice> HardwareDevice::create(RenderAPI api) {
    switch (api) {
    case RenderAPI::Metal:
        return createMetalDevice();

    default:
        return nullptr;
    }
}
