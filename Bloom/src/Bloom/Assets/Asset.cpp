#include "Asset.hpp"

#include "Bloom/Core/Debug.hpp"

namespace bloom {

	/// MARK: - AssetType
	BLOOM_API std::string toString(AssetType type) {
		auto const i = utl::to_underlying(type);
		bloomAssert(std::popcount(i) <= 1, "'type' is a Mask");
		if (type == AssetType::none) {
			return "None";
		}
		return std::array {
			"Static Mesh",
			"Skinned Mesh",
			"Material",
			"Scene"
		}[utl::log2(i)];
	}
	
	BLOOM_API AssetType assetTypeFromString(std::string_view str) {
		if (str == "Static Mesh") {
			return AssetType::staticMesh;
		}
		if (str == "Skinned Mesh") {
			return AssetType::skinnedMesh;
		}
		if (str == "Material") {
			return AssetType::material;
		}
		if (str == "Scene") {
			return AssetType::scene;
		}
		return AssetType::none;
	}
	
	BLOOM_API std::ostream& operator<<(std::ostream& str, AssetType type) {
		return str << toString(type);
	}
	
	BLOOM_API std::string toExtension(AssetType type) {
		auto const i = utl::to_underlying(type);
		bloomAssert(std::popcount(i) <= 1, "'type' is a Mask");
		if (type == AssetType::none) {
			return std::string{};
		}
		return std::array {
			".bmesh",
			".invalid",
			".bmat",
			".bsc"
		}[utl::log2(i)];
	}
	BLOOM_API AssetType assetTypeFromExtension(std::string_view extension) {
		if (extension == ".bmesh") {
			return AssetType::staticMesh;
		}
		if (extension == ".bmat") {
			return AssetType::material;
		}
		if (extension == ".bsc") {
			return AssetType::scene;
		}
		return AssetType::none;
	}
	
	/// MARK: - AssetHandle
	AssetHandle AssetHandle::generate(AssetType type) {
		return AssetHandle(type, utl::UUID::generate());
	}
	
	bool operator==(AssetHandle const& a, AssetHandle const& b) {
		bool const result = a.id() == b.id();
		bloomAssert(!result || a.type() == b.type(),
					"ID is the same but type differs");
		return result;
	}
	
	AssetHandle::operator bool() const {
		return !utl::is_null(id());
	}
	
}
