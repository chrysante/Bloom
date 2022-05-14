#pragma once

#include "Bloom/Core/Base.hpp"
#include "Bloom/Core/Debug.hpp"
#include "ConcreteAssetsFwd.hpp"

#include <filesystem>
#include <utl/common.hpp>
#include <utl/UUID.hpp>
#include <iosfwd>
#include <yaml-cpp/yaml.h>
#include <yaml-cpp/helpers.hpp>

namespace poppy { class EditorAssetManager; }

namespace bloom {
	
	/// MARK: - AssetType
	enum class AssetType: unsigned {
		none = 0,
		staticMesh = 1 << 0,
		skinnedMesh = 1 << 1, // not supported yet...
		mesh = staticMesh | skinnedMesh,
		
		material = 1 << 2,
		
		scene = 1 << 3
	};
	UTL_ENUM_OPERATORS(AssetType);
	
	BLOOM_API std::string toString(AssetType);
	BLOOM_API AssetType assetTypeFromString(std::string_view);
	BLOOM_API std::ostream& operator<<(std::ostream&, AssetType);
	
	BLOOM_API std::string toExtension(AssetType);
	BLOOM_API AssetType assetTypeFromExtension(std::string_view);
	
	/// MARK: - AssetRepresentation
	enum class AssetRepresentation {
		CPU = 1 << 0,
		GPU = 1 << 1
	};
	UTL_ENUM_OPERATORS(AssetRepresentation);
	
	
	/// MARK: - AssetHandle
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
	
	/// MARK: - Asset
	/// Base Class for all assets
	class BLOOM_API Asset {
	public:
		Asset(AssetHandle handle):
			_handle(handle)
		{}
		
		virtual ~Asset() = default;
		
		AssetHandle handle() const { return _handle; }
		
	private:
		AssetHandle _handle;
	};
	
	auto asConcreteAsset(AssetType type, auto&& f) {
		switch (type) {
			case AssetType::staticMesh:
				return f(utl::tag<StaticMeshAsset>{});
			case AssetType::material:
				return f(utl::tag<MaterialAsset>{});
			case AssetType::scene:
				return f(utl::tag<SceneAsset>{});
				
			default:
				bloomDebugbreak();
				std::terminate();
		}
	}
	
}


template <>
struct YAML::convert<bloom::AssetHandle> {
	static Node encode(bloom::AssetHandle const& h) {
		Node node;
		node["Type"] = std::string(toString(h.type()));
		node["ID"] = h.id();
		return node;
	}
	 
	static bool decode(Node const& node, bloom::AssetHandle& h) {
		auto const type = bloom::assetTypeFromString(node["Type"].as<std::string>());
		auto const id = node["ID"].as<utl::UUID>();
		h = bloom::AssetHandle(type, id);
		return true;
	}
};
