#include "Bloom/Asset/AssetManager.h"

#include <fstream>

#include <range/v3/view.hpp>
#include <scatha/Common/SourceFile.h>
#include <scatha/Invocation/CompilerInvocation.h>
#include <utl/filesystem_ext.hpp>
#include <utl/overload.hpp>
#include <utl/strcat.hpp>

#include "Bloom/Application/Application.h"
#include "Bloom/Asset/MeshImport.h"
#include "Bloom/Core/Core.h"
#include "Bloom/Core/Debug.h"
#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/Graphics/Material/Material.h"
#include "Bloom/Graphics/Material/MaterialInstance.h"
#include "Bloom/Graphics/StaticMesh.h"
#include "Bloom/Runtime/ScriptSystem.h"

using namespace bloom;
using namespace ranges::views;

namespace {

/// Object stored by the asset manager for each asset
struct TableEntry {
    ///
    WeakReference<Asset> asset;

    /// Handle of the asset
    AssetHandle handle;

    /// Name of the asset
    std::string name;

    /// Location on disk relative to working directory
    std::filesystem::path diskLocation;
};

} // namespace

struct AssetManager::Impl {
    AssetManager* assetManager;
    utl::hashmap<utl::uuid, TableEntry> assets;
    std::filesystem::path workingDir;

    explicit Impl(AssetManager* assetManager): assetManager(assetManager) {}

    Reference<Asset> allocateAsset(AssetHandle, std::string name) const;

    TableEntry* find(AssetHandle);

    TableEntry const* find(AssetHandle) const;

    void readAssetMetaData(std::filesystem::path const& diskLocation,
                           bool forceOverride = false);

    bool makeAvailImpl(StaticMesh&, TableEntry const& entry,
                       AssetRepresentation repr, bool force);
    bool makeAvailImpl(SkeletalMesh&, TableEntry const& entry,
                       AssetRepresentation repr, bool force);
    bool makeAvailImpl(Material&, TableEntry const& entry,
                       AssetRepresentation repr, bool force);
    bool makeAvailImpl(MaterialInstance&, TableEntry const& entry,
                       AssetRepresentation repr, bool force);
    bool makeAvailImpl(Scene&, TableEntry const& entry,
                       AssetRepresentation repr, bool force);
    bool makeAvailImpl(ScriptSource&, TableEntry const& entry,
                       AssetRepresentation repr, bool force);

    Reference<StaticMeshData> readStaticMeshFromDisk(
        std::filesystem::path const& source);
    void loadMaterialInstanceFromDisk(MaterialInstance& inst,
                                      std::filesystem::path const& source);
    void loadSceneFromDisk(Scene& scene, std::filesystem::path const& source);
    std::string loadTextFromDisk(std::filesystem::path const& source);

    void loadStaticMeshRenderer(StaticMesh& mesh, TableEntry const& entry);

    void flushToDisk(AssetHandle handle);

    void flushToDiskImpl(StaticMesh const&, std::filesystem::path const& dest);
    void flushToDiskImpl(SkeletalMesh const&,
                         std::filesystem::path const& dest);
    void flushToDiskImpl(Material const&, std::filesystem::path const& dest);
    void flushToDiskImpl(MaterialInstance const&,
                         std::filesystem::path const& dest);
    void flushToDiskImpl(Scene const&, std::filesystem::path const& dest);
    void flushToDiskImpl(ScriptSource const&,
                         std::filesystem::path const& dest);

    /// Deduces the asset type from the file extension \p ext of the source
    /// asset file
    std::optional<AssetType> deduceImportedAssetType(
        std::string_view ext) const;
    AssetHandle store(Reference<Asset> asset, std::string_view name,
                      std::filesystem::path dest);

    ///
    void doImport(Asset& dest, std::filesystem::path const& source) const;
    void doImportImpl(Asset const&, std::filesystem::path const&) const {
        BL_UNIMPLEMENTED();
    }
    void doImportImpl(StaticMesh& dest,
                      std::filesystem::path const& source) const;

    std::filesystem::path makeRelative(std::filesystem::path const& path) const;
    std::filesystem::path makeAbsolute(std::filesystem::path const& path) const;
    AssetFileHeader readHeader(std::filesystem::path const& path) const;
    AssetFileHeader readHeader(std::fstream& file) const;
    std::fstream openFile(std::filesystem::path const& path,
                          std::ios_base::openmode mode =
                              std::ios_base::in | std::ios_base::out) const;
};

namespace {

template <AssetRepresentation>
struct Tag {};

} // namespace

template <typename Callback>
static void visitRepr(AssetRepresentation repr, Callback&& callback) {
#define VISIT_CASE(KIND)                                                       \
    if (test(repr & KIND) && std::is_invocable_v<Callback, Tag<KIND>>)         \
    std::invoke(callback, Tag<KIND>{})
    VISIT_CASE(AssetRepresentation::CPU);
    VISIT_CASE(AssetRepresentation::GPU);
#undef VISIT_CASE
}

using Impl = AssetManager::Impl;

AssetManager::AssetManager(): impl(std::make_unique<Impl>(this)) {}

AssetManager::~AssetManager() = default;

HardwareDevice& AssetManager::device() const { return application().device(); };

void AssetManager::setWorkingDir(std::filesystem::path path) {
    impl->workingDir = path.lexically_normal();
    if (!std::filesystem::exists(impl->workingDir)) {
        std::filesystem::create_directories(impl->workingDir);
    }
    /// Load working dir
    impl->assets.clear();
    refreshWorkingDir();
}

void AssetManager::refreshWorkingDir(bool forceOverrides) {
    if (impl->workingDir.empty()) {
        return;
    }
    auto dirItr = std::filesystem::recursive_directory_iterator(workingDir());
    for (auto const& entry: dirItr) {
        if (!std::filesystem::is_regular_file(entry.path()) ||
            utl::is_hidden(entry.path()))
        {
            continue;
        }
        impl->readAssetMetaData(entry.path(), forceOverrides);
    }
    compileScripts();
}

std::filesystem::path AssetManager::workingDir() const {
    return impl->workingDir;
}

Reference<Asset> AssetManager::create(AssetType type, std::string name,
                                      std::filesystem::path destDir) {
    BL_EXPECT(std::filesystem::is_directory(destDir));
    auto handle = AssetHandle::generate(type);
    Reference<Asset> ref = impl->allocateAsset(handle, name);
    TableEntry entry{ .asset = ref,
                      .name = name,
                      .diskLocation = destDir / append(name, toExtension(type)),
                      .handle = handle };
    impl->assets.insert({ handle.ID(), std::move(entry) });
    impl->flushToDisk(handle);
    return ref;
}

/// MARK: - Import
AssetHandle AssetManager::import(std::filesystem::path source,
                                 std::filesystem::path dest) {
    std::string name = source.filename().replace_extension();
    std::optional type =
        impl->deduceImportedAssetType(source.extension().string());
    /// Only static meshes can be imported for now
    if (!type || *type != AssetType::StaticMesh) {
        Logger::Error("Failed to import resource ", source);
        return {};
    }
    auto diskLocation = dest / append(name, toExtension(*type));
    auto [entry, asset] = [&]() -> std::tuple<TableEntry&, Reference<Asset>> {
        if (std::filesystem::exists(diskLocation)) {
            auto handle = impl->readHeader(diskLocation).handle();
            auto* entry = impl->find(handle);
            assert(entry);
            auto asset = entry->asset.lock();
            if (!asset) {
                asset = impl->allocateAsset(handle, name);
                entry->asset = asset;
            }
            return { *entry, std::move(asset) };
        }
        else {
            auto handle = AssetHandle::generate(*type);
            auto asset = impl->allocateAsset(handle, name);
            auto [itr, success] = impl->assets.insert(
                { handle.ID(), TableEntry{ .asset = asset,
                                           .name = name,
                                           .diskLocation = diskLocation } });
            return { itr->second, std::move(asset) };
        }
    }();
    auto handle = asset->handle();
    impl->doImport(*asset, source);
    impl->flushToDisk(handle);
    return handle;
}

/// MARK: - Remove
void AssetManager::remove(AssetHandle handle) {
    auto const itr = impl->assets.find(handle.ID());
    if (itr == impl->assets.end()) {
        Logger::Warn("Failed to remove asset: does not exist.");
        return;
    }
    std::filesystem::remove(itr->second.diskLocation);
    impl->assets.erase(itr);
}

/// MARK: - Access
Reference<Asset> AssetManager::get(AssetHandle handle) {
    auto itr = impl->assets.find(handle.ID());
    if (itr == impl->assets.end()) {
        Logger::Error("Failed to get asset ", handle);
        return nullptr;
    }
    auto& entry = itr->second;
    if (auto asset = entry.asset.lock()) {
        return asset;
    }
    auto asset = impl->allocateAsset(handle, entry.name);
    entry.asset = asset;
    return asset;
}

std::string AssetManager::getName(AssetHandle handle) const {
    if (auto* entry = impl->find(handle)) {
        return entry->name;
    }
    return std::string{};
}

std::filesystem::path AssetManager::getAbsoluteFilepath(
    AssetHandle handle) const {
    auto rel = getRelativeFilepath(handle);
    return rel.empty() ? rel : impl->makeAbsolute(rel);
}

std::filesystem::path AssetManager::getRelativeFilepath(
    AssetHandle handle) const {
    if (auto* entry = impl->find(handle)) {
        return entry->diskLocation;
    }
    return std::filesystem::path{};
}

bool AssetManager::makeAvailable(AssetHandle handle, AssetRepresentation repr,
                                 bool force) {
    static constexpr std::string_view ErrorPrelude =
        "Failed to make asset available: ";
    auto* entry = impl->find(handle);
    if (!entry) {
        Logger::Error(ErrorPrelude, "ID ", handle.ID(), " not found");
        return false;
    }
    auto asset = entry->asset.lock();
    if (!asset) {
        Logger::Error(ErrorPrelude, "No references to asset \"", entry->name,
                      "\"");
        return false;
    }
    try {
        return visit(*asset, [&](auto& asset) {
            return impl->makeAvailImpl(asset, *entry, repr, force);
        });
    }
    catch (std::exception const& e) {
        Logger::Error(ErrorPrelude, e.what());
        return false;
    }
}

bool Impl::makeAvailImpl(StaticMesh& mesh, TableEntry const& entry,
                         AssetRepresentation repr, bool force) {
    // clang-format off
    visitRepr(repr, utl::overload{
        [&](Tag<AssetRepresentation::CPU>) {
            if (mesh.mData && !force) {
                return;
            }
            mesh.mData = readStaticMeshFromDisk(entry.diskLocation);
        },
        [&](Tag<AssetRepresentation::GPU>) {
            loadStaticMeshRenderer(mesh, entry);
        },
    }); // clang-format on
    return true;
}

bool Impl::makeAvailImpl(SkeletalMesh&, TableEntry const&, AssetRepresentation,
                         bool) {
    BL_UNIMPLEMENTED();
}

bool Impl::makeAvailImpl(Material& mat, TableEntry const&,
                         AssetRepresentation repr, bool force) {
    // clang-format off
    visitRepr(repr, utl::overload{
        [&](Tag<AssetRepresentation::CPU>) {
            Logger::Warn("No CPU Representation for materials yet");
        },
        [&](Tag<AssetRepresentation::GPU>) {
            if (mat.mainPass && !force) {
                return;
            }
            mat.makeDefault(assetManager->device());
        },
    }); // clang-format on
    return true;
}

bool Impl::makeAvailImpl(MaterialInstance& inst, TableEntry const& entry,
                         AssetRepresentation repr, bool) {
    loadMaterialInstanceFromDisk(inst, entry.diskLocation);
    // clang-format off
    visitRepr(repr, utl::overload{
        [&](Tag<AssetRepresentation::CPU>) {
            Logger::Warn("No CPU Representation for materials yet");
        },
        [&](Tag<AssetRepresentation::GPU>) {
            assetManager->makeAvailable(inst.material()->handle(),
                                        AssetRepresentation::GPU);
        },
    }); // clang-format on
    return true;
}

bool Impl::makeAvailImpl(Scene& scene, TableEntry const& entry,
                         AssetRepresentation repr, bool) {
    // clang-format off
    visitRepr(repr, utl::overload{
        [&](Tag<AssetRepresentation::CPU>) {
            loadSceneFromDisk(scene, entry.diskLocation);
        },
        [&](Tag<AssetRepresentation::GPU>) {
            Logger::Error("No GPU Representation for scenes");
        },
    }); // clang-format on
    return true;
}

bool Impl::makeAvailImpl(ScriptSource&, TableEntry const&,
                         AssetRepresentation repr, bool) {
    // clang-format off
    visitRepr(repr, utl::overload{
        [&](Tag<AssetRepresentation::CPU>) {
            Logger::Error("No GPU Representation for script source files");
        },
        [&](Tag<AssetRepresentation::GPU>) {
            Logger::Error("No GPU Representation for script source files");
        },
    }); // clang-format on
    return false;
}

bool AssetManager::isValid(AssetHandle handle) const {
    return impl->find(handle);
}

void AssetManager::saveToDisk(AssetHandle handle) { impl->flushToDisk(handle); }

void AssetManager::saveAll() {
    for (auto& [ID, entry]: impl->assets) {
        saveToDisk(entry.handle);
    }
}

AssetHandle AssetManager::getHandleFromFile(std::filesystem::path path) const {
    return impl->readHeader(path).handle();
}

namespace bloom {

/// Defined in "ScriptSystem.cpp"
void ScriptSystem_setTarget(ScriptSystem::Impl& impl, scatha::Target target);

} // namespace bloom

void AssetManager::compileScripts() {
    Logger::Trace("Compiling scripts");
    dispatch(DispatchToken::Now, ScriptsWillLoadEvent{});
    using namespace scatha;
    CompilerInvocation inv(TargetType::BinaryOnly, "main");
    auto sources = impl->assets | values | filter([](auto& entry) {
        return entry.handle.type() == AssetType::ScriptSource;
    }) | transform([](auto& entry) {
        return SourceFile::load(entry.diskLocation);
    });
    try {
        inv.setInputs(sources | ranges::to<std::vector>);
    }
    catch (std::exception const& e) {
        Logger::Error(e.what());
    }
    auto target = inv.run();
    if (!target) {
        return;
    }
    ScriptSystem_setTarget(*application().coreSystems().scriptSystem().impl,
                           std::move(*target));
    dispatch(DispatchToken::Now, ScriptsDidLoadEvent{});
}

Reference<Asset> Impl::allocateAsset(AssetHandle handle,
                                     std::string name) const {
    return dispatchAssetType(handle.type(),
                             [&]<typename A>(utl::tag<A>) -> Reference<Asset> {
        return allocateRef<A>(handle, std::move(name));
    });
}

TableEntry* Impl::find(AssetHandle handle) {
    return const_cast<TableEntry*>(utl::as_const(*this).find(handle));
}

TableEntry const* Impl::find(AssetHandle handle) const {
    if (!handle) {
        return nullptr;
    }
    auto const itr = assets.find(handle.ID());
    if (itr == assets.end()) {
        return nullptr;
    }
    return &itr->second;
}

void Impl::readAssetMetaData(std::filesystem::path const& diskLocation,
                             bool forceOverride) {
    AssetFileHeader const header = readHeader(makeAbsolute(diskLocation));
    auto const handle = header.handle();
    if (find(handle)) {
        if (!forceOverride) return;
    }
    assets[handle.ID()] = { .asset = allocateAsset(handle, header.name()),
                            .name = header.name(),
                            .diskLocation = diskLocation,
                            .handle = handle };
}

Reference<StaticMeshData> Impl::readStaticMeshFromDisk(
    std::filesystem::path const& source) {
    assert(toExtension(source) == FileExtension::Bmesh);
    auto file = openFile(makeAbsolute(source), std::ios::in | std::ios::binary);
    auto header = readHeader(file);
    if (header.handle().type() != AssetType::StaticMesh) {
        Logger::Error("File was not a static mesh");
        return nullptr;
    }
    auto meshHeader = header.customDataAs<MeshFileHeader>();
    auto result = allocateRef<StaticMeshData>();
    result->vertices.resize(meshHeader.vertexDataSize / sizeof(Vertex3D),
                            utl::no_init);
    result->indices.resize(meshHeader.indexDataSize / sizeof(uint32_t),
                           utl::no_init);
    file.read((char*)result->vertices.data(), meshHeader.vertexDataSize);
    file.read((char*)result->indices.data(), meshHeader.indexDataSize);
    return result;
}

void Impl::loadMaterialInstanceFromDisk(MaterialInstance& inst,
                                        std::filesystem::path const& source) {
    assert(toExtension(source) == FileExtension::Bmatinst);
    auto file = openFile(makeAbsolute(source), std::ios::in | std::ios::binary);
    auto header = readHeader(file);
    if (header.handle().type() != AssetType::MaterialInstance) {
        Logger::Error("File was not a Material Instance");
        // TODO: Throw exception here
        BL_UNIMPLEMENTED();
    }
    auto materialInstanceHeader =
        header.customDataAs<MaterialInstanceFileHeader>();
    (void)materialInstanceHeader;
    YAML::Node root = YAML::Load(file);
    inst.deserialize(root, *assetManager);
}

void Impl::loadSceneFromDisk(Scene& scene,
                             std::filesystem::path const& source) {
    assert(toExtension(source) == FileExtension::Bscene);
    auto file = openFile(makeAbsolute(source), std::ios::in | std::ios::binary);
    if (readHeader(file).handle().type() != AssetType::Scene) {
        Logger::Error("File was not a Scene");
        // TODO: Throw exception here
        BL_UNIMPLEMENTED();
    }
    YAML::Node root = YAML::Load(file);
    scene.deserialize(root, *assetManager);
}

std::string Impl::loadTextFromDisk(std::filesystem::path const& source) {
    assert(toExtension(source) == FileExtension::Scatha);
    auto file = openFile(makeAbsolute(source), std::ios::in);
    std::stringstream sstr;
    sstr << file.rdbuf();
    return sstr.str();
}

/// MARK: - Memory -> GPU
void Impl::loadStaticMeshRenderer(StaticMesh& mesh, TableEntry const& entry) {
    if (mesh.mRenderer) {
        return;
    }
    if (!mesh.mData) {
        mesh.mData = readStaticMeshFromDisk(entry.diskLocation);
    }
    mesh.mRenderer = allocateRef<StaticMeshRenderer>();
    auto* renderer = mesh.mRenderer.get();
    BufferDescription desc;
    auto& vertices = mesh.mData->vertices;
    desc.data = vertices.data();
    desc.size = vertices.size() * sizeof(vertices[0]);
    desc.storageMode = StorageMode::Shared;
    renderer->mVertexBuffer = assetManager->device().createBuffer(desc);
    desc.data = mesh.mData->indices.data();
    desc.size = mesh.mData->indices.size() * 4;
    renderer->mIndexBuffer = assetManager->device().createBuffer(desc);
}

void Impl::flushToDisk(AssetHandle handle) {
    auto* entry = find(handle);
    if (!entry) {
        Logger::Error("Failed to write asset ", handle, " to disk");
        return;
    }
    auto asset = entry->asset.lock();
    if (!asset) {
        return;
    }
    visit(*asset, [&](auto& asset) {
        flushToDiskImpl(asset, makeAbsolute(entry->diskLocation));
    });
}

void Impl::flushToDiskImpl(StaticMesh const& mesh,
                           std::filesystem::path const& dest) {
    auto* data = mesh.mData.get();
    if (!data) {
        return;
    }
    std::uint64_t vertexDataSize =
        data->vertices.size() * sizeof(bloom::Vertex3D);
    std::uint64_t indexDataSize = data->indices.size() * sizeof(uint32_t);
    /// Make header
    AssetFileHeader header(mesh.handle(), FileFormat::Binary, mesh.name(),
                           MeshFileHeader{ .vertexDataSize = vertexDataSize,
                                           .indexDataSize = indexDataSize });
    std::ios::sync_with_stdio(false);
    auto file =
        openFile(dest, std::ios::out | std::ios::trunc | std::ios::binary);
    file.write((char*)&header, sizeof header);
    file.write((char*)data->vertices.data(), vertexDataSize);
    file.write((char*)data->indices.data(), indexDataSize);
}

void Impl::flushToDiskImpl(SkeletalMesh const&, std::filesystem::path const&) {
    BL_UNIMPLEMENTED();
}

void Impl::flushToDiskImpl(Material const& mat,
                           std::filesystem::path const& dest) {
    AssetFileHeader header(mat.handle(), FileFormat::Binary, mat.name(),
                           MaterialFileHeader{});
    std::ios::sync_with_stdio(false);
    auto file =
        openFile(dest, std::ios::out | std::ios::trunc | std::ios::binary);
    file.write((char*)&header, sizeof header);
}

void Impl::flushToDiskImpl(MaterialInstance const& inst,
                           std::filesystem::path const& dest) {
    AssetFileHeader header(inst.handle(), FileFormat::Text, inst.name(),
                           MaterialInstanceFileHeader{});
    std::ios::sync_with_stdio(false);
    auto file =
        openFile(dest, std::ios::out | std::ios::trunc | std::ios::binary);
    file.write((char*)&header, sizeof header);
    YAML::Emitter out;
    out << inst.serialize();
    file << out.c_str();
}

void Impl::flushToDiskImpl(Scene const& scene,
                           std::filesystem::path const& dest) {
    AssetFileHeader header(scene.handle(), FileFormat::Text, scene.name(),
                           SceneFileHeader{});
    std::ios::sync_with_stdio(false);
    auto file =
        openFile(dest, std::ios::out | std::ios::trunc | std::ios::binary);
    file.write((char*)&header, sizeof header);
    YAML::Emitter out;
    out << scene.serialize();
    file << out.c_str();
}

void Impl::flushToDiskImpl(ScriptSource const&,
                           std::filesystem::path const& dest) {
    if (std::filesystem::exists(dest)) {
        Logger::Warn(
            "Script source is not written to disk because we don't have memory representation");
        return;
    }
    std::fstream file(dest, std::ios::out);
    if (!file) {
        Logger::Warn("Failed to create file ", dest);
    }
}

std::optional<AssetType> Impl::deduceImportedAssetType(
    std::string_view extView) const {
    std::string ext(extView);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](auto c) { return std::toupper(c); });
    if (ext == ".FBX") {
        return AssetType::StaticMesh;
    }
    return std::nullopt;
}

void Impl::doImport(Asset& asset, std::filesystem::path const& source) const {
    visit(asset, [&](auto& asset) { doImportImpl(asset, source); });
}

void Impl::doImportImpl(StaticMesh& dest,
                        std::filesystem::path const& source) const {
    BL_EXPECT(source.has_filename());
    BL_EXPECT(source.has_extension());
    dest.mData = allocateRef<StaticMeshData>(importStaticMesh(source));
}

std::filesystem::path Impl::makeRelative(
    std::filesystem::path const& path) const {
    if (path.is_absolute()) {
        return std::filesystem::relative(path, workingDir);
    }
    return path;
}

std::filesystem::path Impl::makeAbsolute(
    std::filesystem::path const& path) const {
    if (path.is_relative()) {
        return (workingDir / path).lexically_normal();
    }
    return path;
}

AssetFileHeader Impl::readHeader(std::filesystem::path const& path) const {
    FileExtension extension = toExtension(path).value();
    if (hasHeader(extension)) {
        auto file = openFile(makeAbsolute(path));
        return readHeader(file);
    }
    return AssetFileHeader{
        AssetHandle(toAssetType(extension),
                    toUUID(path.lexically_normal().string())),
        toFileFormat(extension), path.filename().replace_extension().string()
    };
}

AssetFileHeader Impl::readHeader(std::fstream& file) const {
    AssetFileHeader header;
    file.read((char*)&header, sizeof(AssetFileHeader));
    return header;
}

std::fstream Impl::openFile(std::filesystem::path const& path,
                            std::ios_base::openmode mode) const {
    std::fstream file(path, mode);
    if (!file) {
        throw std::runtime_error(utl::strcat("Failed to open file ", path));
    }
    return file;
}
