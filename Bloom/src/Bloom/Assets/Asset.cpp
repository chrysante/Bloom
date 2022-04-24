#include "Asset.hpp"

#include "Bloom/Core/Debug.hpp"

namespace bloom {

	BLOOM_API std::string_view toString(AssetType type) {
		auto const i = utl::to_underlying(type);
		bloomAssert(std::popcount(i) <= 1, "'type' is a Mask");
		if (type == AssetType::none) {
			return "None";
		}
		return std::array {
			"Static Mesh",
			"Skinned Mesh",
			"Material"
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
		return AssetType::none;
	}
	
	BLOOM_API std::ostream& operator<<(std::ostream& str, AssetType type) {
		return str << toString(type);
	}
	
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
