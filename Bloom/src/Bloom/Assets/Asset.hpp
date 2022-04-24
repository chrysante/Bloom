#pragma once

#include "Bloom/Core/Base.hpp"

#include <filesystem>
#include <utl/common.hpp>
#include <utl/UUID.hpp>
#include <iosfwd>

namespace poppy { class EditorAssetManager; }

namespace bloom {
	
	enum class AssetType: unsigned {
		none = 0,
		staticMesh = 1 << 0,
		skinnedMesh = 1 << 1, // not supported yet...
		mesh = staticMesh | skinnedMesh,
		
		material = 1 << 2
	};
	UTL_ENUM_OPERATORS(AssetType);
	
	BLOOM_API std::string_view toString(AssetType);
	BLOOM_API AssetType assetTypeFromString(std::string_view);
	BLOOM_API std::ostream& operator<<(std::ostream&, AssetType);
	
	enum class AssetRepresentation {
		CPU = 1 << 0,
		GPU = 1 << 1
	};
	UTL_ENUM_OPERATORS(AssetRepresentation);
	
	class BLOOM_API AssetHandle {
	public:
		AssetHandle() = default;
		AssetHandle(AssetType type, utl::UUID id):
			_type(type),
			_id(id)
		{}
		
		AssetType type() const { return _type; }
		utl::UUID id() const { return _id; }
	
		static AssetHandle generate(AssetType);
		
		friend bool operator==(AssetHandle const&, AssetHandle const&);
		operator bool() const;
		
	private:
		AssetType _type = AssetType::none;
		utl::UUID _id;
	};
	
	/// Base Class for all assets
	class BLOOM_API Asset: public AssetHandle {
	public:
		Asset(AssetHandle handle, std::string name, std::filesystem::path diskLocation):
			AssetHandle(handle),
			_name(std::move(name)),
			_diskLocation(std::move(diskLocation))
		{}
		Asset(AssetHandle handle, std::string_view name, std::filesystem::path diskLocation):
			Asset(handle, std::string(name), std::move(diskLocation))
		{}
		
		virtual ~Asset() = default;
		
		std::filesystem::path diskLocation() const { return _diskLocation; }
		
		AssetHandle handle() const { return *this; }
		std::string_view name() const { return _name; }
		
	private:
		std::string _name;
		std::filesystem::path _diskLocation;
	};
	
}
