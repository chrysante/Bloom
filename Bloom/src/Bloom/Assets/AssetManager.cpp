#include "AssetManager.hpp"

#include "Bloom/Core/Debug.hpp"

#include "ConcreteAssets.hpp"
#include "MeshImporter.hpp"
#include "Bloom/Graphics/RenderContext.hpp"
#include "Bloom/Graphics/StaticRenderMesh.hpp"

#include "Bloom/Scene/SceneSerialize.hpp"

#include "Bloom/ScriptEngine/ScriptEngine.hpp"

#include <fstream>
#include <utl/filesystem_ext.hpp>

namespace bloom {
	
	BLOOM_API std::string_view extension(AssetType type) {
		switch (type) {
			case AssetType::staticMesh:
				return ".bmesh";
			case AssetType::skeletalMesh:
				return ".bskeletalMesh";
			case AssetType::material:
				return ".bmaterial";
			case AssetType::scene:
				return ".bscene";
				
			default:
				return "";
		}
	}
	
	AssetManager::~AssetManager() = default;
	
	/// MARK: - Environment
	void AssetManager::setWorkingDir(std::filesystem::path path) {
		_workingDir = path.lexically_normal();
		if (!std::filesystem::exists(_workingDir)) {
			std::filesystem::create_directories(_workingDir);
		}
		
		// load working dir
		assets.clear();
		refreshWorkingDir();
	}
	
	void AssetManager::refreshWorkingDir(bool forceOverrides) {
		if (_workingDir.empty()) {
			return;
		}
		auto dirItr = std::filesystem::recursive_directory_iterator(workingDir());
		for (auto const& entry: dirItr) {
			if (!std::filesystem::is_regular_file(entry.path())) {
				continue;
			}
			if (utl::is_hidden(entry.path())) {
				continue;
			}
			readAssetMetaData(entry.path(), forceOverrides);
		}
	}

	/// MARK: - Create
	Reference<Asset> AssetManager::create(AssetType type,
										  std::string_view name,
										  std::filesystem::path dest)
	{
		auto const handle = AssetHandle::generate(type);
		Reference<Asset> const ref = allocateAsset(handle);
		InternalAsset asset {
			.theAsset = ref,
			.name = std::string(name),
			.diskLocation = dest / utl::format("{}{}", name, toExtension(type)),
			.handle = handle
		};
		assets.insert({ handle.id(), asset });
		flushToDisk(handle);
		return ref;
	}
	
	/// MARK: - Import
	AssetHandle AssetManager::import(std::filesystem::path source,
									 std::filesystem::path dest)
	{
		std::string const name = source.filename().replace_extension();
		AssetType const type = getImportType(source.extension().string());
		
		if (type != AssetType::staticMesh) {
			bloomLog(error, "Failed to import resource {}", source);
			return {};
		}
		
		auto const diskLocation = dest / (name + toExtension(type));
		
		auto const [asset, assetRef] = [&]() {
			if (std::filesystem::exists(diskLocation)) {
				auto const handle = readHeader(diskLocation).handle();
				auto* const asset = find(handle);
				bloomAssert(asset);
				auto assetRef = asset->theAsset.lock();
				if (!assetRef) {
					assetRef = allocateAsset(handle);
					asset->theAsset = assetRef;
				}
				return std::tuple<InternalAsset&, Reference<Asset>>{
					*asset, assetRef
				};
			}
			else {
				auto const handle = AssetHandle::generate(type);
				auto const assetRef = allocateAsset(handle);
				auto [itr, success] = assets.insert({ handle.id(), InternalAsset{
					.theAsset = assetRef,
					.name = name,
					.diskLocation = diskLocation
				} });
				return std::tuple<InternalAsset&, Reference<Asset>>{
					itr->second, assetRef
				};
			}
		}();
		auto const handle = assetRef->handle();
		
		switch (type) {
			case AssetType::staticMesh:
				importStaticMesh(utl::down_cast<StaticMeshAsset*>(assetRef.get()), source);
				break;
				
			default:
				bloomDebugbreak();
				std::terminate();
		}
		
		flushToDisk(handle);
		
		return handle;
	}
	
	/// MARK: - Remove
	void AssetManager::remove(AssetHandle handle) {
		auto const itr = assets.find(handle.id());
		if (itr == assets.end()) {
			bloomLog(warning, "Failed to remove asset: Asset does not exist.");
			return;
		}
		std::filesystem::remove(itr->second.diskLocation);
		assets.erase(itr);
	}
	
	/// MARK: - Access
	Reference<Asset> AssetManager::get(AssetHandle handle) {
		auto const itr = assets.find(handle.id());
		if (itr == assets.end()) {
			bloomLog(error, "Failed to get Asset");
			return nullptr;
		}
		
		auto& weakAsset = itr->second.theAsset;
		
		
		if (auto const asset = weakAsset.lock()) {
			bloomAssert(handle == asset->handle());
			return asset;
		}
		
		auto const asset = allocateAsset(handle);
		weakAsset = asset;
		return asset;
	}
	
	std::string AssetManager::getName(AssetHandle handle) const {
		if (auto const* asset = find(handle)) {
			return asset->name;
		}
		return std::string{};
	}
	
	std::filesystem::path AssetManager::getAbsoluteFilepath(AssetHandle handle) const {
		auto rel = getRelativeFilepath(handle);
		return rel.empty() ? rel : makeAbsolute(rel);
	}
	
	std::filesystem::path AssetManager::getRelativeFilepath(AssetHandle handle) const {
		if (auto const* asset = find(handle)) {
			return asset->diskLocation;
		}
		return std::filesystem::path{};
	}
	
	void AssetManager::makeAvailable(AssetHandle handle, AssetRepresentation rep, bool force) {
		auto const itr = assets.find(handle.id());
		if (itr == assets.end()) {
			bloomLog(error, "Failed to make available: ID not found [ID = {}]", handle.id());
			return;
		}
		
		auto const assetRef = itr->second.theAsset.lock();
		
		if (!assetRef) {
			bloomLog(error, "Failed to make available: No references to asset \"{}\"", itr->second.name);
			return;
		}
		
		bloomAssert(handle == assetRef->handle());
		
		switch (handle.type()) {
			case AssetType::staticMesh:
				makeStaticMeshAvailable(itr->second, rep, force);
				break;
			
			case AssetType::material:
				makeMaterialAvailable(itr->second, rep, force);
				break;
				
			case AssetType::scene:
				makeSceneAvailable(itr->second, rep, force);
				break;
				
			case AssetType::script:
				makeScriptAvailable(itr->second, rep, force);
				break;
				
			default:
				bloomDebugfail("Unimplemented");
				break;
		}
	}
	
	bool AssetManager::isValid(AssetHandle handle) const {
		return !!find(handle);
	}
	
	/// MARK: - Save
	void AssetManager::saveToDisk(AssetHandle handle) {
		flushToDisk(handle);
	}
	
	/// MARK: - Uncategorized
	AssetHandle AssetManager::getHandleFromFile(std::filesystem::path path) const {
		return readHeader(path).handle();
	}
	
	void AssetManager::loadScripts(ScriptEngine& engine) {
		_scriptClasses.clear();
		engine.restoreBaseState();
		for (auto&& [id, internal]: assets) {
			if (internal.handle.type() != AssetType::script) {
				continue;
			}
			auto const script = as<ScriptAsset>(get(internal.handle));
			makeAvailable(internal.handle, AssetRepresentation::CPU, true);
			try {
				engine.eval(script->text);
				for (auto&& name: script->classes) {
					_scriptClasses.push_back(name);
				}
			}
			catch (std::exception const& e) {
				bloomLog(error, "Failed to evaluate script: {}", e.what());
			}
		}
	}
	
	/// MARK: - Internals
	Reference<Asset> AssetManager::allocateAsset(AssetHandle handle) const {
		return asConcreteAsset(handle.type(), [&]<typename A>(utl::tag<A>) -> Reference<Asset> {
			return allocateRef<A>(handle);
		});
	}
	
	AssetManager::InternalAsset* AssetManager::find(AssetHandle handle) {
		return const_cast<InternalAsset*>(utl::as_const(*this).find(handle));
	}
	
	AssetManager::InternalAsset const* AssetManager::find(AssetHandle handle) const {
		if (!handle) {
			return nullptr;
		}
		auto const itr = assets.find(handle.id());
		if (itr == assets.end()) {
			return nullptr;
		}
		return &itr->second;
	}
	
	void AssetManager::readAssetMetaData(std::filesystem::path diskLocation, bool forceOverride) {
		AssetFileHeader const header = readHeader(makeAbsolute(diskLocation));
		auto const handle = header.handle();
		if (find(handle)) {
			if (!forceOverride)
				return;
		}
		
		auto iAsset = InternalAsset{
			.theAsset = allocateAsset(handle),
			.name = header.name(),
			.diskLocation = diskLocation,
			.handle = handle
		};
		
		auto const [itr, success] = assets.insert({ handle.id(), iAsset });
		if (!success) {
			itr->second = iAsset;
		}
	}
	
	/// MARK: - Make Available
	void AssetManager::makeStaticMeshAvailable(InternalAsset& ia, AssetRepresentation rep, bool force) {
		StaticMeshAsset* smAsset = utl::down_cast<StaticMeshAsset*>(ia.theAsset.lock().get());
		
		if (test(rep & AssetRepresentation::CPU) && (!smAsset->_meshData || force)) {
			smAsset->_meshData = readStaticMeshFromDisk(ia.diskLocation);
		}
		
		if (test(rep & AssetRepresentation::GPU) && (!smAsset->_renderMesh || force)) {
			loadStaticRenderMesh(ia);
		}
	}
		
	void AssetManager::makeMaterialAvailable(InternalAsset& ia, AssetRepresentation rep, bool force) {
		MaterialAsset* mAsset = utl::down_cast<MaterialAsset*>(ia.theAsset.lock().get());
		
		if (test(rep & AssetRepresentation::CPU)) {
			bloomLog(warning, "No CPU Representation for materials yet");
		}
		
		if (test(rep & AssetRepresentation::GPU)) {
			mAsset->material = Material::makeDefaultMaterial(renderContext());
		}
	}
	
	void AssetManager::makeSceneAvailable(InternalAsset& ia, AssetRepresentation rep, bool force) {
		SceneAsset* asset = utl::down_cast<SceneAsset*>(ia.theAsset.lock().get());
		
		if (test(rep & AssetRepresentation::CPU) && (asset->scene.empty() || force)) {
			asset->scene = loadSceneFromDisk(ia.diskLocation);
		}
		
		if (test(rep & AssetRepresentation::GPU)) {
			bloomLog(warning, "No GPU Representation for scenes");
		}
	}
	
	void AssetManager::makeScriptAvailable(InternalAsset& ia, AssetRepresentation rep, bool force) {
		ScriptAsset* asset = utl::down_cast<ScriptAsset*>(ia.theAsset.lock().get());
		
		if (test(rep & AssetRepresentation::CPU)) {
			asset->setText(loadTextFromDisk(ia.diskLocation));
		}
		
		if (test(rep & AssetRepresentation::GPU)) {
			bloomLog(warning, "No GPU Representation for scripts");
		}
	}
	
	///MARK: Disk -> Memory
	Reference<StaticMeshData> AssetManager::readStaticMeshFromDisk(std::filesystem::path source) {
		bloomExpect(toExtension(source) == FileExtension::bmesh);
		source = makeAbsolute(source);
		std::fstream file(source, std::ios::in | std::ios::binary);
		handleFileError(file, source);
		
		auto const header = readHeader(file);
		
		if (header.handle().type() != AssetType::staticMesh) {
			bloomLog(error, "File was not a Mesh");
			bloomDebugbreak();
			return {};
		}
	
		auto const meshHeader = header.customDataAs<MeshFileHeader>();
		
		auto const result = allocateRef<StaticMeshData>();
		result->vertices.resize(meshHeader.vertexDataSize / sizeof(Vertex3D), utl::no_init);
		result->indices.resize(meshHeader.indexDataSize  / sizeof(uint32_t), utl::no_init);
		file.read((char*)result->vertices.data(), meshHeader.vertexDataSize);
		file.read((char*)result->indices.data(),  meshHeader.indexDataSize);
		
		return result;
	}
	
	Scene AssetManager::loadSceneFromDisk(std::filesystem::path source) {
		bloomExpect(toExtension(source) == FileExtension::bscene);
		source = makeAbsolute(source);
		std::fstream file(source, std::ios::in | std::ios::binary);
		handleFileError(file, source);
		
		auto const header = readHeader(file);
		
		if (header.handle().type() != AssetType::scene) {
			bloomLog(error, "File was not a Scene");
			bloomDebugbreak();
			return {};
		}
	
		auto const sceneHeader = header.customDataAs<SceneFileHeader>();
		(void)sceneHeader;
		
		std::stringstream sstr;
		sstr << file.rdbuf();
		
		Scene scene;
		deserialize(sstr.str(), &scene, this);
		
		return scene;
	}
	
	std::string AssetManager::loadTextFromDisk(std::filesystem::path source) {
		bloomExpect(toExtension(source) == FileExtension::chai);
		source = makeAbsolute(source);
		std::fstream file(source, std::ios::in);
		handleFileError(file, source);
		std::stringstream sstr;
		sstr << file.rdbuf();
		return sstr.str();
	}
	
	/// MARK: - Memory -> GPU
	void AssetManager::loadStaticRenderMesh(InternalAsset& ia) {
		if (!renderContext()) {
			bloomLog(warning, "RenderContext is null. Failed to load static mesh to GPU");
			return;
		}
		
		auto* const asset = utl::down_cast<StaticMeshAsset*>(ia.theAsset.lock().get());
		if (asset->_renderMesh) {
			return;
		}
		
		Reference<StaticMeshData> smData = asset->_meshData;
		if (!smData) {
			smData = readStaticMeshFromDisk(ia.diskLocation);
		}
		
		asset->_renderMesh = allocateRef<StaticRenderMesh>();
		auto* const mesh = asset->_renderMesh.get();
		
		mesh->_handle = asset->handle();
		mesh->vertexBuffer = renderContext()->createVertexBuffer(smData->vertices.data(),
																 smData->vertices.size() * sizeof(smData->vertices[0]));
		mesh->indexBuffer = renderContext()->createIndexBuffer(smData->indices);
	}
	
	
	/// MARK: - Memory -> Disk
	void AssetManager::flushToDisk(AssetHandle handle) {
		switch (handle.type()) {
			case AssetType::staticMesh:
				flushStaticMeshToDisk(handle);
				break;
			case AssetType::material:
				flushMaterialToDisk(handle);
				break;
			case AssetType::scene:
				flushSceneToDisk(handle);
				break;
			case AssetType::script:
				flushScriptToDisk(handle);
				break;
				
			default:
				bloomDebugbreak();
				break;
		}
		
	}
	
	void AssetManager::flushStaticMeshToDisk(AssetHandle handle) {
		auto const* const asset = find(handle);
		
		bloomAssert(asset);
		auto const assetRef = asset->theAsset.lock();
		bloomAssert(!!assetRef);
		bloomAssert(assetRef->handle() == handle);
		
		auto const& mesh = *utl::down_cast<StaticMeshAsset const*>(assetRef.get())->_meshData;
		
		std::uint64_t const vertexDataSize = mesh.vertices.size() * sizeof(bloom::Vertex3D);
		std::uint64_t const indexDataSize = mesh.indices.size() * sizeof(uint32_t);
		
		// make header
		AssetFileHeader const header(handle, FileFormat::binary, asset->name, MeshFileHeader{
			.vertexDataSize = vertexDataSize,
			.indexDataSize = indexDataSize
		});
		
		std::ios::sync_with_stdio(false);
		
		auto const dest = makeAbsolute(asset->diskLocation);
		std::fstream file(dest, std::ios::out | std::ios::trunc | std::ios::binary);
		handleFileError(file, dest);
		
		file.write((char*)&header, sizeof(AssetFileHeader));
		file.write((char*)mesh.vertices.data(), vertexDataSize);
		file.write((char*)mesh.indices.data(), indexDataSize);
	}
	
	void AssetManager::flushMaterialToDisk(AssetHandle handle) {
		auto const* const asset = find(handle);
		bloomAssert(asset);
		auto const assetRef = asset->theAsset.lock();
		bloomAssert(!!assetRef);
		bloomAssert(assetRef->handle() == handle);
		
//		auto const& material = *utl::down_cast<MaterialAsset const*>(assetRef.get())->_meshData;
		
		// make header
		AssetFileHeader const header(handle, FileFormat::binary, asset->name, MaterialFileHeader{});
		
		std::ios::sync_with_stdio(false);
		
		auto const dest = makeAbsolute(asset->diskLocation);
		auto const destLocation = std::filesystem::path{ dest }.remove_filename();
		if (!std::filesystem::exists(destLocation)) {
			std::filesystem::create_directory(destLocation);
		}
		std::fstream file(dest, std::ios::out | std::ios::trunc | std::ios::binary);
		handleFileError(file, dest);
		
		file.write((char*)&header, sizeof(AssetFileHeader));
	}
	
	void AssetManager::flushSceneToDisk(AssetHandle handle) {
		auto const* const asset = find(handle);
		bloomAssert(asset);
		auto const assetRef = asset->theAsset.lock();
		bloomAssert(!!assetRef);
		bloomAssert(assetRef->handle() == handle);
		
		auto const& scene = utl::down_cast<SceneAsset const*>(assetRef.get())->scene;
		
		// make header
		AssetFileHeader const header(handle, FileFormat::text, asset->name, SceneFileHeader{});
		
		std::ios::sync_with_stdio(false);
		
		auto const dest = makeAbsolute(asset->diskLocation);
		std::fstream file(dest, std::ios::out | std::ios::trunc | std::ios::binary);
		handleFileError(file, dest);
		
		file.write((char*)&header, sizeof(AssetFileHeader));
		
		file << serialize(&scene);
	}
	
	void AssetManager::flushScriptToDisk(AssetHandle handle) {
		auto const* const asset = find(handle);
		bloomAssert(asset);
		auto const assetRef = asset->theAsset.lock();
		bloomAssert(!!assetRef);
		bloomAssert(assetRef->handle() == handle);
		
		auto const& script = utl::down_cast<ScriptAsset const*>(assetRef.get())->text;
		
		auto const dest = makeAbsolute(asset->diskLocation);
		std::fstream file(dest, std::ios::out | std::ios::trunc);
		handleFileError(file, dest);
				
		file << script;
	}
	
	/// MARK: - Import
	AssetType AssetManager::getImportType(std::string_view extView) const {
		std::string ext(extView);
		std::transform(ext.begin(), ext.end(), ext.begin(), [](auto c) { return std::toupper(c); });
		if (ext == ".FBX") {
			return AssetType::staticMesh;
		}
		return AssetType::none;
	}
	
	void AssetManager::importStaticMesh(StaticMeshAsset* meshOut, std::filesystem::path source) const {
		bloomAssert(source.has_filename());
		bloomAssert(source.has_extension());
		
		// import
		MeshImporter importer;
		meshOut->_meshData = allocateRef<StaticMeshData>(importer.import(source));
	}

	/// MARK: - File Handling
	std::filesystem::path AssetManager::makeRelative(std::filesystem::path const& path) const {
		if (path.is_absolute()) {
			return std::filesystem::relative(path, workingDir());
		}
		return path;
	}
	
	std::filesystem::path AssetManager::makeAbsolute(std::filesystem::path const& path) const {
		if (path.is_relative()) {
			return (workingDir() / path).lexically_normal();
		}
		return path;
	}
	
	AssetFileHeader AssetManager::readHeader(std::filesystem::path path) const {
		FileExtension const extension = toExtension(path);
		if (hasHeader(extension)) {
			path = makeAbsolute(path);
			std::fstream file(path);
			handleFileError(file, path);
			return readHeader(file);
		}
		else {
			return AssetFileHeader{
				AssetHandle(toAssetType(extension), toUUID(path.lexically_normal().string())),
				toFileFormat(extension),
				path.filename().replace_extension().string()
			};
		}
	}
	
	AssetFileHeader AssetManager::readHeader(std::fstream& file) const {
		AssetFileHeader header;
		file.read((char*)&header, sizeof(AssetFileHeader));
		return header;
	}
	
	void AssetManager::handleFileError(std::fstream& file,
									   std::filesystem::path const& path) const
	{
		if (!file) {
			bloomDebugbreak();
			throw std::runtime_error(utl::format("Failed to open file {}", path));
		}
	}
	
}
