#pragma once

namespace bloom {
	
	
	
	struct AssetFileHeader {
		static constexpr std::size_t customDataSize = 256;
		
		AssetFileHeader() = default;
		AssetFileHeader(AssetHandle const& handle, std::string_view name):
			type(handle.type()),
			id(handle.id()),
			nameBuffer{}
		{
			std::strncpy(nameBuffer, name.data(), 127);
		}
		template <typename T>
		AssetFileHeader(AssetHandle const& handle, std::string_view name, T&& t):
			AssetFileHeader(handle, name)
		{
			static_assert(sizeof(T) <= customDataSize);
			::new ((void*)customData) T(std::forward<T>(t));
		}
		
		explicit operator AssetHandle() const {
			return getAssetHandle();
		}
		
		AssetHandle getAssetHandle() const {
			return AssetHandle(type, id);
		}
		
		std::string name() const { return nameBuffer; }
		
		template <typename T>
		T customDataAs() const {
			static_assert(sizeof(T) <= customDataSize);
			std::aligned_storage_t<sizeof(T), alignof(T)> storage;
			std::memcpy(&storage, customData, sizeof(T));
			return reinterpret_cast<T&>(storage);
		}
		
		AssetType type;
		utl::UUID id;
		char nameBuffer[128];
		char customData[customDataSize];
	};
	
	struct MeshFileHeader {
		std::size_t vertexDataSize;
		std::size_t indexDataSize;
	};
	
	
}
