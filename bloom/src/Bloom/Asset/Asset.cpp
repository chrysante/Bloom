#include "Asset.hpp"

#include <utl/math.hpp>

#include "Bloom/Core/Debug.hpp"

namespace bloom {

	FileExtension toExtension(std::filesystem::path const& path) {
		return toExtension(std::string_view(path.extension().string()));
	}
	
	FileExtension toExtension(std::string_view extension) {
		if (extension == ".bmesh") {
			return FileExtension::bmesh;
		}
		if (extension == ".bmat") {
			return FileExtension::bmat;
		}
		if (extension == ".bmatinst") {
			return FileExtension::bmatinst;
		}
		if (extension == ".bscene") {
			return FileExtension::bscene;
		}
		if (extension == ".chai") {
			return FileExtension::chai;
		}
		return FileExtension::invalid;
	}
	
	bool hasHeader(FileExtension ext) {
		return ext != FileExtension::chai;
	}
	
	utl::uuid toUUID(std::string_view str) {
		char const* const begin = str.data();
		std::size_t const offset = str.size() / 2;
		struct { std::size_t first, second; } values = {
			utl::hash_string(std::string_view(begin, offset)),
			utl::hash_string(std::string_view(begin + offset, str.size() - offset))
		};
		return utl::uuid::construct_from_value(utl::bit_cast<utl::uuid::value_type>(values));
	}
	
	/// MARK: - AssetType
	BLOOM_API std::string toString(AssetType type) {
		auto const i = utl::to_underlying(type);
		bloomAssert(std::popcount(i) <= 1 && "'type' is a Mask");
		if (type == AssetType::none) {
			return "None";
		}
		return std::array {
			"Static Mesh",
			"Skeletal Mesh",
			"Material",
			"Material Instance",
			"Scene",
			"Script"
		}[utl::log2(i)];
	}
	
	BLOOM_API AssetType assetTypeFromString(std::string_view str) {
		if (str == "Static Mesh") {
			return AssetType::staticMesh;
		}
		if (str == "Skeletal Mesh") {
			return AssetType::skeletalMesh;
		}
		if (str == "Material") {
			return AssetType::material;
		}
		if (str == "Material Instance") {
			return AssetType::materialInstance;
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
		bloomAssert(std::popcount(i) <= 1 && "'type' is a Mask");
		if (type == AssetType::none) {
			return std::string{};
		}
		return std::array {
			".bmesh",
			".invalid",
			".bmat",
			".bmatinst",
			".bscene",
			".chai",
		}[utl::log2(i)];
	}
	BLOOM_API AssetType toAssetType(FileExtension ext) {
		switch (ext) {
			case FileExtension::bmesh:
				return AssetType::staticMesh;
				
			case FileExtension::bmat:
				return AssetType::material;
				
			case FileExtension::bmatinst:
				return AssetType::materialInstance;
				
			case FileExtension::bscene:
				return AssetType::scene;
				
			case FileExtension::chai:
				return AssetType::script;
				
			default:
				return AssetType::none;
		}
	}
	
	BLOOM_API FileFormat toFileFormat(FileExtension ext) {
		switch (ext) {
			case FileExtension::bmesh:
				return FileFormat::binary;
				
			case FileExtension::bmat:
				return FileFormat::binary;
				
			case FileExtension::bmatinst:
				return FileFormat::text;
				
			case FileExtension::bscene:
				return FileFormat::text;
				
			case FileExtension::chai:
				return FileFormat::text;
				
			default:
				return FileFormat::text;
		}
	}
	
	/// MARK: - AssetHandle
	AssetHandle AssetHandle::generate(AssetType type) {
		return AssetHandle(type, utl::uuid::generate());
	}
	
	bool operator==(AssetHandle const& a, AssetHandle const& b) {
		bool const result = a.id() == b.id();
		bloomAssert(!result || a.type() == b.type() &&
					"ID is the same but type differs");
		return result;
	}
	
	AssetHandle::operator bool() const {
		return !utl::is_null(id());
	}
	
}
