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

std::vector<ShaderFunctionHandle> ShaderCompiler::compile(
    std::filesystem::path sourcepath,
    std::span<std::string const> functionNames) {
    if (sourcepath.is_relative()) {
        sourcepath = resourceDir() / sourcepath;
    }
    std::fstream file(sourcepath);
    if (!file) {
        Logger::Error("Failed to open file: ", sourcepath);
        return {};
    }
    std::stringstream sstr;
    sstr << file.rdbuf();
    return compile(std::move(sstr).str(), std::move(functionNames));
}
