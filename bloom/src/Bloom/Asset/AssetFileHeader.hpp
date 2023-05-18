#pragma once

#include "Bloom/Asset/Asset.hpp"

namespace bloom {

struct AssetFileHeader {
    static constexpr std::size_t customDataSize = 256;

    AssetFileHeader() = default;
    AssetFileHeader(AssetHandle const& handle,
                    FileFormat format,
                    std::string_view name):
        _handle(handle), _format(format), nameBuffer{} {
        std::strncpy(nameBuffer, name.data(), 127);
    }
    template <typename T>
        requires(sizeof(T) <= customDataSize)
    AssetFileHeader(AssetHandle const& handle,
                    FileFormat format,
                    std::string_view name,
                    T&& t):
        AssetFileHeader(handle, format, name) {
        ::new ((void*)customData) T(std::forward<T>(t));
    }

    AssetHandle handle() const { return _handle; }

    FileFormat format() const { return _format; }

    std::string name() const { return nameBuffer; }

    template <typename T>
        requires(sizeof(T) <= customDataSize)
    T customDataAs() const {
        std::aligned_storage_t<sizeof(T), alignof(T)> storage;
        std::memcpy(&storage, customData, sizeof(T));
        return reinterpret_cast<T&>(storage);
    }

    AssetHandle _handle;
    FileFormat _format;
    char nameBuffer[128];
    char customData[customDataSize];
};

struct MeshFileHeader {
    std::size_t vertexDataSize;
    std::size_t indexDataSize;
};

struct MaterialFileHeader {};

struct MaterialInstanceFileHeader {};

struct SceneFileHeader {};

} // namespace bloom
