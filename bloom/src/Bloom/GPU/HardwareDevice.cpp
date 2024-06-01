#include "Bloom/GPU/HardwareDevice.h"

#include <fstream>
#include <sstream>

#include "Bloom/Application/ResourceUtil.h"
#include "Bloom/GPU/ShaderCompiler.h"
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

Reference<ShaderLibrary> ShaderCompiler::compileFile(
    std::filesystem::path sourcePath, ShaderCompilerOptions const& options) {
    if (sourcePath.is_relative()) {
        sourcePath = resourceDir() / sourcePath;
    }
    std::fstream file(sourcePath);
    if (!file) {
        Logger::Error("Failed to open file: ", sourcePath);
        return {};
    }
    std::stringstream sstr;
    sstr << file.rdbuf();
    return compileSource(std::move(sstr).str(), options);
}
