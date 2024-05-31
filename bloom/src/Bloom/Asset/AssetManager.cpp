#include "Bloom/Asset/AssetManager.h"

#include <fstream>

#include <range/v3/algorithm.hpp>
#include <range/v3/view.hpp>
#include <scatha/Common/SourceFile.h>
#include <scatha/Invocation/CompilerInvocation.h>
#include <utl/filesystem_ext.hpp>
#include <utl/overload.hpp>
#include <utl/strcat.hpp>

#include "Bloom/Application/Application.h"
#include "Bloom/Asset/AssetImport.h"
#include "Bloom/Core/Core.h"
#include "Bloom/Core/Debug.h"
#include "Bloom/GPU/CommandBuffer.h"
#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/GPU/ShaderCompiler.h"
#include "Bloom/Graphics/Material/Material.h"
#include "Bloom/Graphics/Material/MaterialInstance.h"
#include "Bloom/Graphics/RenderGraph/RenderGraph.h"
#include "Bloom/Graphics/StaticMesh.h"
#include "Bloom/Graphics/Texture.h"
#include "Bloom/Runtime/ScriptBindings.h"
#include "Bloom/Runtime/ScriptSystem.h"

using namespace bloom;
using namespace vml::short_types;
using namespace ranges::views;

namespace {

static std::filesystem::path registryFilePath() { return ".registry"; }

/// List of possible asset file extensions
enum class FileExtension { Bmesh, Bmat, Bmatinst, Btex, Bscene, Scatha, Brg };

/// \Returns the file extension \p ext as a string with a preceding dot
/// character
static std::string toString(FileExtension ext) {
    switch (ext) {
    case FileExtension::Bmesh:
        return ".bmesh";
    case FileExtension::Bmat:
        return ".bmat";
    case FileExtension::Bmatinst:
        return ".bmatinst";
    case FileExtension::Btex:
        return ".btex";
    case FileExtension::Bscene:
        return ".bscene";
    case FileExtension::Scatha:
        return ".sc";
    case FileExtension::Brg:
        return ".brg";
    }
}

/// \Returns `filename.replace_extension(toString(ext))`
static std::filesystem::path append(std::filesystem::path filename,
                                    FileExtension ext) {
    return filename.replace_extension(toString(ext));
}

/// Converts the extension \p ext to `FileExtension`
static std::optional<FileExtension> toExtension(std::string_view ext) {
    if (ext == ".bmesh") {
        return FileExtension::Bmesh;
    }
    if (ext == ".bmat") {
        return FileExtension::Bmat;
    }
    if (ext == ".bmatinst") {
        return FileExtension::Bmatinst;
    }
    if (ext == ".btex") {
        return FileExtension::Btex;
    }
    if (ext == ".bscene") {
        return FileExtension::Bscene;
    }
    if (ext == ".sc") {
        return FileExtension::Scatha;
    }
    if (ext == ".brg") {
        return FileExtension::Brg;
    }
    return std::nullopt;
}

/// Converts the extension of \p path to `FileExtension`
static std::optional<FileExtension> toExtension(
    std::filesystem::path const& path) {
    return toExtension(std::string_view(path.extension().string()));
}

/// \Returns the file extension corresponding to \p type
static FileExtension toExtension(AssetType type) {
    switch (type) {
    case AssetType::Asset:
        BL_UNREACHABLE();
    case AssetType::Mesh:
        BL_UNREACHABLE();
    case AssetType::StaticMesh:
        return FileExtension::Bmesh;
    case AssetType::SkeletalMesh:
        BL_UNREACHABLE();
    case AssetType::Material:
        return FileExtension::Bmat;
    case AssetType::MaterialInstance:
        return FileExtension::Bmatinst;
    case AssetType::Texture2D:
        return FileExtension::Btex;
    case AssetType::Scene:
        return FileExtension::Bscene;
    case AssetType::ScriptSource:
        return FileExtension::Scatha;
    case AssetType::RenderGraph:
        return FileExtension::Brg;
    case AssetType::Invalid:
        BL_UNREACHABLE();
    }
}

/// \Returns the asset type corresponding to the file extension \p ext
static AssetType toAssetType(FileExtension ext) {
    switch (ext) {
    case FileExtension::Bmesh:
        return AssetType::StaticMesh;
    case FileExtension::Bmat:
        return AssetType::Material;
    case FileExtension::Bmatinst:
        return AssetType::MaterialInstance;
    case FileExtension::Btex:
        return AssetType::Texture2D;
    case FileExtension::Bscene:
        return AssetType::Scene;
    case FileExtension::Scatha:
        return AssetType::ScriptSource;
    case FileExtension::Brg:
        return AssetType::RenderGraph;
    }
}

/// \Returns the asset type corresponding `path.extension()`
static std::optional<AssetType> toAssetType(std::filesystem::path const& path) {
    auto ext = toExtension(path);
    return ext ? std::optional(toAssetType(*ext)) : std::nullopt;
}

/// Base class of all exceptions thrown by the asset manager
class AssetManagerError: public std::runtime_error {
public:
    /// For now
    explicit AssetManagerError(auto const&... args):
        runtime_error(utl::strcat(args...)) {}

    using runtime_error::runtime_error;
};

} // namespace

AssetRegistryEntry const* AssetRegistry::find(utl::uuid ID) const {
    auto itr = elementMap.find(ID);
    return itr != elementMap.end() ? &itr->second : nullptr;
}

AssetRegistryEntry* AssetRegistry::find(utl::uuid ID) {
    return const_cast<AssetRegistryEntry*>(std::as_const(*this).find(ID));
}

AssetRegistryEntry const* AssetRegistry::find(AssetHandle handle) const {
    return find(handle.ID());
}

AssetRegistryEntry const* AssetRegistry::findByName(
    std::string_view name) const {
    auto itr = ranges::find(*this, name, &AssetRegistryEntry::name);
    return itr != end() ? std::to_address(itr) : nullptr;
}

AssetRegistryEntry const* AssetRegistry::findByDiskLocation(
    std::filesystem::path location) const {
    auto itr = ranges::find(*this, location, &AssetRegistryEntry::diskLocation);
    return itr != end() ? std::to_address(itr) : nullptr;
}

AssetRegistryEntry* AssetRegistry::insert(AssetRegistryEntry entry) {
    auto [itr, success] =
        elementMap.insert({ entry.handle.ID(), std::move(entry) });
    return success ? &itr->second : nullptr;
}

void AssetRegistry::clear() { elementMap.clear(); }

bool AssetRegistry::erase(utl::uuid ID) { return elementMap.erase(ID) == 1; }

using Impl = AssetManager::Impl;
using RegistryEntry = AssetRegistryEntry;

struct AssetManager::Impl {
    AssetManager* assetManager;
    AssetRegistry registry;
    std::filesystem::path rootDir;
    Reference<ShaderCompiler> shaderCompiler;

    explicit Impl(AssetManager* assetManager);

    RegistryEntry* getEntry(utl::uuid ID);
    RegistryEntry const* getEntry(utl::uuid ID) const;

    /// Opens the registry file in mode \p mode
    std::fstream openRegistryFile(std::ios_base::openmode mode = std::ios::in |
                                                                 std::ios::out);

    /// Deletes the contents of the registry file
    /// \Warning Use with care. A deleted registry is a major headache for the
    /// user because all asset references need to be restored manually
    void clearRegistryFile();

    /// Loads every entry from the registry on disk into memory
    /// Opens the registry file or creates one if it doesn't exist.
    /// \pre `rootDir` must be set to a valid path
    void openRegistry();

    /// Scans the entire project directory and creates missing registry entries
    bool amendRegistry();

    /// Overrides the projects registry file with what is currently in memory
    void writeRegistryToDisk();

    /// Allocates an asset of the correct derived type for \p handle
    Reference<Asset> allocateAsset(AssetHandle handle, std::string name) const;

    /// \Returns the registry entry of the asset at \p location or null if the
    /// file does not exist _in the registry_.
    ///
    /// \Param location can be either relative to the project directory or
    /// absolute, in which case it must be in the current project directory
    RegistryEntry* findByDiskLocation(std::filesystem::path location);

    bool makeAvail(Asset& asset, RegistryEntry const& entry,
                   AssetRepresentation repr, bool forceReload);
    bool makeAvailImpl(StaticMesh&, RegistryEntry const& entry,
                       AssetRepresentation repr, bool force);
    bool makeAvailImpl(SkeletalMesh&, RegistryEntry const& entry,
                       AssetRepresentation repr, bool force);
    bool makeAvailImpl(Material&, RegistryEntry const& entry,
                       AssetRepresentation repr, bool force);
    bool makeAvailImpl(MaterialInstance&, RegistryEntry const& entry,
                       AssetRepresentation repr, bool force);
    bool makeAvailImpl(Texture2D&, RegistryEntry const& entry,
                       AssetRepresentation repr, bool force);
    bool makeAvailImpl(Scene&, RegistryEntry const& entry,
                       AssetRepresentation repr, bool force);
    bool makeAvailImpl(ScriptSource&, RegistryEntry const& entry,
                       AssetRepresentation repr, bool force);
    bool makeAvailImpl(RenderGraph&, RegistryEntry const& entry,
                       AssetRepresentation repr, bool force);

    Reference<StaticMeshData> readStaticMeshFromDisk(
        std::filesystem::path const& source);
    void loadMaterialFromDisk(Material& mat,
                              std::filesystem::path const& source);
    void loadMaterialInstanceFromDisk(MaterialInstance& inst,
                                      std::filesystem::path const& source);
    void loadTextureFromDisk(Texture2D& texture,
                             std::filesystem::path const& source);
    void loadSceneFromDisk(Scene& scene, std::filesystem::path const& source);
    void loadRenderGraphFromDisk(RenderGraph& rg,
                                 std::filesystem::path const& source);

    void uploadStaticMeshToGPU(StaticMesh& mesh, RegistryEntry const& entry);
    void uploadTextureToGPU(Texture2D& texture, RegistryEntry const& entry);

    void flushToDisk(AssetHandle handle, bool force = false);

    void flushToDiskImpl(StaticMesh const&, std::filesystem::path const& dest,
                         bool force);
    void flushToDiskImpl(SkeletalMesh const&, std::filesystem::path const& dest,
                         bool force);
    void flushToDiskImpl(Material const&, std::filesystem::path const& dest,
                         bool force);
    void flushToDiskImpl(MaterialInstance const&,
                         std::filesystem::path const& dest, bool force);
    void flushToDiskImpl(Texture2D const&, std::filesystem::path const& dest,
                         bool force);
    void flushToDiskImpl(Scene const&, std::filesystem::path const& dest,
                         bool force);
    void flushToDiskImpl(ScriptSource const&, std::filesystem::path const& dest,
                         bool force);
    void flushToDiskImpl(RenderGraph const&, std::filesystem::path const& dest,
                         bool force);

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
    void doImportImpl(Texture2D& dest,
                      std::filesystem::path const& source) const;
    void doImportImpl(StaticMesh& dest,
                      std::filesystem::path const& source) const;

    /// Creates a registry entry. All entries should be created through this
    /// method so we can sanitize the input
    RegistryEntry makeEntry(WeakReference<Asset> asset, AssetHandle handle,
                            std::string name,
                            std::filesystem::path diskLocation);

    YAML::Node serializeEntry(RegistryEntry const& entry);
    std::optional<RegistryEntry> deserializeEntry(YAML::Node const& node);

    /// \Returns \p path relative to the current project directory. If \p path
    /// is already relative it is returned as is
    std::filesystem::path makeRelative(std::filesystem::path const& path) const;

    /// \Returns \p path  prefixed with the current project directory if it is
    /// relative. If it is absolute we assert that it is in the current project
    /// directory
    std::filesystem::path makeAbsolute(std::filesystem::path const& path) const;

    /// \Returns `true` if \p path is an absolute path in the current project
    /// directory
    bool isAbsoluteInProjDir(std::filesystem::path const& path) const;

    /// Opens the file at \p path
    ///
    /// \Param path must be absolute and a subdirectory of the current project
    /// directory
    std::fstream openFile(std::filesystem::path const& path) const;

    /// \overload
    std::fstream openFile(std::filesystem::path const& path,
                          std::ios_base::openmode mode) const;

    /// Opens the file at \p path or creates it if it does not exist
    ///
    /// \Param path must be absolute and a subdirectory of the current project
    /// directory
    std::fstream openOrCreateFile(std::filesystem::path const& path) const;

    /// \overload
    std::fstream openOrCreateFile(std::filesystem::path const& path,
                                  std::ios_base::openmode mode) const;
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

AssetManager::AssetManager() {}

AssetManager::~AssetManager() = default;

AssetRegistry const& AssetManager::registry() const { return impl->registry; }

void AssetManager::init() {
    impl = std::make_unique<Impl>(this);
    listen([this](GetAssetManager msg) {
        if (msg.assetManager) {
            *msg.assetManager = this;
        }
    });
    listen([this](AMGetAsset const& msg) {
        auto asset = get(msg.handle, msg.representation);
        if (!asset) {
            Logger::Error("AssetManager: Failed to satisfy asset request");
            return;
        }
        if (msg.asset) {
            *msg.asset = std::move(asset);
        }
        if (msg.diskLocation) {
            auto* entry = registry().find(msg.handle);
            BL_ASSERT(entry,
                      "We found an asset so we must find a registry entry");
            *msg.diskLocation = entry->diskLocation;
        }
    });
}

HardwareDevice& AssetManager::device() const { return application().device(); };

CommandQueue& AssetManager::commandQueue() const {
    return application().commandQueue();
}

void AssetManager::openProject(std::filesystem::path path) {
    impl->rootDir = path.lexically_normal();
    if (!std::filesystem::exists(impl->rootDir)) {
        std::filesystem::create_directories(impl->rootDir);
    }
    /// Load working dir
    impl->registry.clear();
    refreshProject();
}

void AssetManager::refreshProject() {
    if (projectRootDir().empty()) {
        return;
    }
    impl->openRegistry();
    impl->amendRegistry();
    impl->writeRegistryToDisk();
    // TODO: Evaluate this
    compileScripts();
}

Impl::Impl(AssetManager* assetManager): assetManager(assetManager) {
    shaderCompiler = assetManager->device().createShaderCompiler();
}

std::fstream Impl::openRegistryFile(std::ios_base::openmode mode) {
    BL_EXPECT(std::filesystem::is_directory(rootDir));
    BL_EXPECT(std::filesystem::exists(rootDir));
    return openOrCreateFile(rootDir / registryFilePath(), mode);
}

void Impl::clearRegistryFile() {
    auto path = rootDir / registryFilePath();
    (void)std::fstream(path, std::ios::trunc | std::ios::out);
}

void Impl::openRegistry() {
    auto file = openRegistryFile();
    YAML::Node root;
    try {
        root = YAML::Load(file);
    }
    catch (YAML::ParserException const& e) {
        // TODO: Try to save what we can from the existing registry file
        Logger::Error("Failed to parse registry: ", e.what(),
                      " - Deleting existing registry file");
        clearRegistryFile();
        return;
    }
    if (root.IsNull()) {
        Logger::Trace("Empty registery, creating new registry");
        return;
    }
    if (!root.IsSequence()) {
        throw AssetManagerError("Invalid registry");
    }
    for (auto node: root) {
        try {
            auto diskEntry = deserializeEntry(node);
            if (!diskEntry) {
                continue;
            }
            registry.insert(*std::move(diskEntry));
        }
        catch (std::exception const& e) {
            Logger::Error("Failed to read registry entry");
        }
    }
}

/// \Returns the filename without extension
static std::string pathToName(std::filesystem::path const& path) {
    return path.filename().stem();
}

bool Impl::amendRegistry() {
    auto knownAssets = registry | transform(&RegistryEntry::diskLocation) |
                       ranges::to<utl::hashset<std::filesystem::path>>;
    auto dirItr = std::filesystem::recursive_directory_iterator(rootDir);
    auto filtered = dirItr | filter([](auto& entry) {
        auto& path = entry.path();
        return path.has_extension() && std::filesystem::is_regular_file(path) &&
               !utl::is_hidden(path);
    });
    bool modified = false;
    utl::hashset<std::filesystem::path> existingAssetPaths;
    for (auto& dirEntry: filtered) {
        auto path = dirEntry.path();
        existingAssetPaths.insert(path);
        if (knownAssets.contains(path)) {
            continue;
        }
        auto type = toAssetType(path);
        if (!type) {
            Logger::Trace("Unexpected file in project directory: ", path);
            continue;
        }
        modified = true;
        auto ID = utl::uuid::generate();
        registry.insert(
            makeEntry({}, AssetHandle(*type, ID), pathToName(path), path));
    }
    utl::small_vector<utl::uuid> IDsToDelete;
    for (auto& entry: registry) {
        if (!existingAssetPaths.contains(entry.diskLocation)) {
            Logger::Error("Asset in registry does not exist: ",
                          entry.diskLocation, " - deleting registry entry");
            IDsToDelete.push_back(entry.handle.ID());
        }
    }
    for (auto ID: IDsToDelete) {
        registry.erase(ID);
    }
    return modified;
}

void Impl::writeRegistryToDisk() {
    auto entries = ranges::to<std::vector>(registry);
    /// We sort the entries by disk path to make sure we only modify the file if
    /// the content changes in a meaningful way
    ranges::sort(entries, std::ranges::less{}, &RegistryEntry::diskLocation);
    YAML::Node root;
    for (auto& entry: entries) {
        root.push_back(serializeEntry(entry));
    }
    YAML::Emitter out;
    out << root;
    openRegistryFile(std::ios::out | std::ios::trunc) << out.c_str();
}

std::filesystem::path AssetManager::projectRootDir() const {
    return impl->rootDir;
}

static std::filesystem::path makeCanonical(std::filesystem::path const& path) {
    std::string str = path.string();
    if (str.size() >= 2 && str[0] == '.' &&
        str[1] == std::filesystem::path::preferred_separator)
    {
        str.erase(str.begin(), str.begin() + 2);
    }
    return std::move(str);
}

Reference<Asset> AssetManager::createAsset(AssetType type, std::string name,
                                           std::filesystem::path destDir) {
    BL_EXPECT(std::filesystem::is_directory(destDir));
    auto diskLocation =
        makeCanonical(destDir / append(name, toExtension(type)));
    if (auto* entry = impl->findByDiskLocation(diskLocation)) {
        Logger::Trace("Asset already exists: ", diskLocation);
        return get(entry->handle);
    }
    auto handle = AssetHandle::generate(type);
    Reference<Asset> ref = impl->allocateAsset(handle, name);
    auto entry = impl->makeEntry(ref, handle, name, diskLocation);
    impl->registry.insert(std::move(entry));
    impl->writeRegistryToDisk();
    impl->flushToDisk(handle, /* force: */ true);
    return ref;
}

AssetHandle AssetManager::importAsset(std::string name,
                                      std::filesystem::path source,
                                      std::filesystem::path destDir) {
    std::optional type =
        impl->deduceImportedAssetType(source.extension().string());
    /// Only static meshes can be imported for now
    if (!type) {
        Logger::Error("Failed to import resource ", source);
        return {};
    }
    auto diskLocation =
        makeCanonical(destDir / append(name, toExtension(*type)));
    auto asset = [&] {
        /// This is a reimport
        if (auto* entry = impl->findByDiskLocation(diskLocation)) {
            auto asset = entry->asset.lock();
            if (!asset) {
                asset = impl->allocateAsset(entry->handle, name);
                entry->asset = asset;
            }
            return asset;
        }
        /// We have not seen this asset before
        else {
            auto handle = AssetHandle::generate(*type);
            auto asset = impl->allocateAsset(handle, name);
            entry = impl->registry.insert(
                impl->makeEntry(asset, asset->handle(), name, diskLocation));
            BL_ASSERT(entry, "We checked above that this does not exist");
            impl->writeRegistryToDisk();
            return asset;
        }
    }();
    auto handle = asset->handle();
    impl->doImport(*asset, source);
    impl->flushToDisk(handle, /* force */ true);
    makeAvailable(handle, AssetRepresentation::GPU,
                  /* force: */ true);
    return handle;
}

/// \Returns \p path with it's filename replaced with \p newName
/// The extension of \p path is kept
static std::filesystem::path replaceFilename(std::filesystem::path path,
                                             std::string newName) {
    auto ext = path.extension().string();
    return path.remove_filename() / (newName + ext);
}

void AssetManager::renameAsset(AssetHandle handle, std::string name) {
    auto* entry = impl->registry.find(handle.ID());
    if (!entry) {
        Logger::Warn("Failed to rename asset, does not exist: ", handle);
        return;
    }
    entry->name = name;
    auto oldPath = entry->diskLocation;
    auto newPath = replaceFilename(oldPath, name);
    Logger::Trace("Renaming asset to: ", newPath);
    std::filesystem::rename(oldPath, newPath);
    entry->diskLocation = newPath;
    impl->writeRegistryToDisk();
    if (auto asset = entry->asset.lock()) {
        asset->mName = name;
    }
}

void AssetManager::relocateAsset(AssetHandle handle,
                                 std::filesystem::path newLocation) {
    auto* entry = impl->registry.find(handle.ID());
    if (!entry) {
        Logger::Warn("Failed to rename asset, does not exist: ", handle);
        return;
    }
    BL_ASSERT(newLocation.filename().stem() == entry->name);
    Logger::Trace("Moving asset to: ", newLocation);
    std::filesystem::rename(entry->diskLocation, newLocation);
    entry->diskLocation = newLocation;
    impl->writeRegistryToDisk();
}

void AssetManager::relocateFolder(std::filesystem::path oldLocation,
                                  std::filesystem::path newLocation) {
    BL_ASSERT(impl->isAbsoluteInProjDir(oldLocation));
    BL_ASSERT(impl->isAbsoluteInProjDir(newLocation));
    std::filesystem::rename(oldLocation, newLocation);
    for (auto& entry: impl->registry) {
        auto end = std::mismatch(oldLocation.begin(), oldLocation.end(),
                                 entry.diskLocation.begin())
                       .first;
        if (end == oldLocation.end()) {
            auto newPath = newLocation /
                           entry.diskLocation.lexically_relative(oldLocation);
            Logger::Trace("Moving asset to: ", newPath);
            entry.diskLocation = newPath;
        }
    }
    impl->writeRegistryToDisk();
}

void AssetManager::deleteAsset(AssetHandle handle) {
    auto* entry = impl->registry.find(handle);
    BL_ASSERT(entry);
    Logger::Trace("Deleting asset: ", entry->name);
    std::filesystem::remove(entry->diskLocation);
    impl->registry.erase(handle.ID());
    impl->writeRegistryToDisk();
}

void AssetManager::deleteFolder(std::filesystem::path dir) {
    BL_ASSERT(impl->isAbsoluteInProjDir(dir));
    utl::small_vector<RegistryEntry const*> toDelete;
    for (auto& entry: impl->registry) {
        auto end =
            std::mismatch(dir.begin(), dir.end(), entry.diskLocation.begin())
                .first;
        if (end == dir.end()) {
            toDelete.push_back(&entry);
        }
    }
    for (auto* entry: toDelete) {
        Logger::Trace("Deleting asset: ", entry->name);
        impl->registry.erase(entry->handle.ID());
    }
    impl->writeRegistryToDisk();
    std::filesystem::remove_all(dir);
}

AssetHandle AssetManager::findByName(std::string_view name) const {
    auto* entry = registry().findByName(name);
    return entry ? entry->handle : AssetHandle{};
}

Reference<Asset> AssetManager::get(AssetHandle handle,
                                   AssetRepresentation repr) {
    auto* entry = impl->registry.find(handle.ID());
    if (!entry) {
        Logger::Error("Failed to find asset ", handle);
        return nullptr;
    }
    auto asset = [&] {
        if (auto asset = entry->asset.lock()) {
            return asset;
        }
        auto asset = impl->allocateAsset(handle, entry->name);
        entry->asset = asset;
        return asset;
    }();
    if (repr != AssetRepresentation::None) {
        impl->makeAvail(*asset, *entry, repr, false);
    }
    return asset;
}

std::string AssetManager::getName(AssetHandle handle) const {
    auto* entry = impl->registry.find(handle);
    return entry ? entry->name : std::string{};
}

std::filesystem::path AssetManager::getAbsoluteFilepath(
    AssetHandle handle) const {
    if (auto* entry = impl->registry.find(handle)) {
        return entry->diskLocation;
    }
    return std::filesystem::path{};
}

std::filesystem::path AssetManager::getRelativeFilepath(
    AssetHandle handle) const {
    auto abs = getAbsoluteFilepath(handle);
    return abs.empty() ? abs : impl->makeRelative(abs);
}

static constexpr std::string_view MakeAvailErrorPrelude =
    "Failed to make asset available: ";

bool AssetManager::makeAvailable(RegistryEntry const& entry,
                                 AssetRepresentation repr, bool forceReload) {
    if (repr == AssetRepresentation::None) {
        return false;
    }
    auto asset = entry.asset.lock();
    if (!asset) {
        Logger::Error(MakeAvailErrorPrelude,
                      "No references to asset: ", entry.name);
        return false;
    }
    return impl->makeAvail(*asset, const_cast<RegistryEntry&>(entry), repr,
                           forceReload);
}

bool AssetManager::makeAvailable(AssetHandle handle, AssetRepresentation repr,
                                 bool forceReload) {
    if (repr == AssetRepresentation::None) {
        return false;
    }
    auto* entry = impl->registry.find(handle);
    if (!entry) {
        Logger::Error(MakeAvailErrorPrelude, "ID not found: ", handle.ID());
        return false;
    }
    return makeAvailable(*entry, repr, forceReload);
}

bool Impl::makeAvail(Asset& asset, RegistryEntry const& entry,
                     AssetRepresentation repr, bool forceReload) {
    try {
        return visit(asset, [&](auto& asset) {
            return makeAvailImpl(asset, entry, repr, forceReload);
        });
    }
    catch (std::exception const& e) {
        Logger::Error(MakeAvailErrorPrelude, e.what());
        return false;
    }
}

bool Impl::makeAvailImpl(StaticMesh& mesh, RegistryEntry const& entry,
                         AssetRepresentation repr, bool forceReload) {
    // clang-format off
    visitRepr(repr, utl::overload{
        [&](Tag<AssetRepresentation::CPU>) {
            if (mesh.mData && !forceReload) {
                return;
            }
            mesh.mData = readStaticMeshFromDisk(entry.diskLocation);
        },
        [&](Tag<AssetRepresentation::GPU>) {
            uploadStaticMeshToGPU(mesh, entry);
        },
    }); // clang-format on
    return true;
}

bool Impl::makeAvailImpl(SkeletalMesh&, RegistryEntry const&,
                         AssetRepresentation, bool) {
    BL_UNIMPLEMENTED();
}

bool Impl::makeAvailImpl(Material& mat, RegistryEntry const& entry,
                         AssetRepresentation repr, bool forceReload) {
    if (test(repr & AssetRepresentation::GPU) && !mat.model->hasCPURepr()) {
        /// We need the CPU representation to compile the shaders
        repr |= AssetRepresentation::CPU;
    }
    // clang-format off
    visitRepr(repr, utl::overload{
        [&](Tag<AssetRepresentation::CPU>) {
            loadMaterialFromDisk(mat, entry.diskLocation);
        },
        [&](Tag<AssetRepresentation::GPU>) {
            if (mat.rgMainPass && !forceReload) {
                return;
            }
            mat.compile(assetManager->device(), *assetManager);
            for (auto &[index, texture]: mat.constantFragmentTextures) {
                assetManager->makeAvailable(texture->handle(),
                                            AssetRepresentation::GPU);
            }
        },
    }); // clang-format on
    return true;
}

bool Impl::makeAvailImpl(MaterialInstance& inst, RegistryEntry const& entry,
                         AssetRepresentation repr, bool forceReload) {
    loadMaterialInstanceFromDisk(inst, entry.diskLocation);
    // clang-format off
    visitRepr(repr, utl::overload{
        [&](Tag<AssetRepresentation::CPU>) {
            Logger::Warn("No CPU Representation for materials yet");
        },
        [&](Tag<AssetRepresentation::GPU>) {
            if (inst.material() && !forceReload) {
                return;
            }
            loadMaterialInstanceFromDisk(inst, entry.diskLocation);
        },
    }); // clang-format on
    return true;
}

bool Impl::makeAvailImpl(Texture2D& texture, RegistryEntry const& entry,
                         AssetRepresentation repr, bool forceReload) {
    // clang-format off
    visitRepr(repr, utl::overload{
        [&](Tag<AssetRepresentation::CPU>) {
            if (!texture.pixelData || forceReload) {
                loadTextureFromDisk(texture, entry.diskLocation);
            }
        },
        [&](Tag<AssetRepresentation::GPU>) {
            if (!texture.GPUHandle || forceReload) {
                uploadTextureToGPU(texture, entry);                
            }
        },
    }); // clang-format on
    return true;
}

bool Impl::makeAvailImpl(Scene& scene, RegistryEntry const& entry,
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

bool Impl::makeAvailImpl(ScriptSource&, RegistryEntry const&,
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

bool Impl::makeAvailImpl(RenderGraph& rg, RegistryEntry const& entry,
                         AssetRepresentation repr, bool force) {
    if (test(repr & AssetRepresentation::GPU) && !rg.model().hasCPURepr()) {
        /// We need the CPU representation to compile the render graph
        repr |= AssetRepresentation::CPU;
    }
    // clang-format off
    visitRepr(repr, utl::overload{
        [&](Tag<AssetRepresentation::CPU>) {
            loadRenderGraphFromDisk(rg, entry.diskLocation);
        },
        [&](Tag<AssetRepresentation::GPU>) {
            if (!rg.getExecutionOrder().empty() && !force) {
                return;
            }
            rg.model().construct(rg);
            rg.compile();
        },
    }); // clang-format on
    return true;
}

bool AssetManager::isValid(AssetHandle handle) const {
    return registry().find(handle) != nullptr;
}

void AssetManager::saveToDisk(AssetHandle handle) { impl->flushToDisk(handle); }

void AssetManager::saveAll() {
    if (impl->rootDir.empty()) {
        Logger::Trace("Not saving: No asset directory open");
        return;
    }
    if (!std::filesystem::is_directory(impl->rootDir)) {
        Logger::Error("Asset root directory path is not a directory: ",
                      impl->rootDir);
        return;
    }
    Logger::Trace("Saving asset directory: ", impl->rootDir);
    impl->writeRegistryToDisk();
    for (auto& entry: impl->registry) {
        saveToDisk(entry.handle);
    }
}

namespace bloom {

/// Defined in "ScriptSystem.cpp"
void ScriptSystem_setTarget(ScriptSystem::Impl& impl, scatha::Target target);

} // namespace bloom

static bool compileEngineLibrary(std::filesystem::path destDir) {
    auto* sourceDir = std::getenv("BLOOM_SCRIPT_DIR");
    if (!sourceDir || !std::filesystem::exists(sourceDir)) {
        Logger::Error(
            "Cannot find engine script directory. Define environment variable BLOOM_SCRIPT_DIR and set it to an existing directory");
        return false;
    }
    using namespace scatha;
    std::vector<SourceFile> sourceFiles;
    for (auto entry: std::filesystem::recursive_directory_iterator(sourceDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".sc") {
            sourceFiles.push_back(SourceFile::load(entry.path()));
        }
    }
    sourceFiles.push_back(SourceFile::make(makeKeyTableSource()));
    CompilerInvocation inv(TargetType::StaticLibrary, "Bloom");
    inv.setInputs(sourceFiles);
    std::stringstream err;
    inv.setErrorStream(err);
    auto target = inv.run();
    if (!target) {
        Logger::Error("Failed to compiled engine script library:\n", err.str());
        return false;
    }
    target->writeToDisk(destDir);
    return true;
}

void AssetManager::compileScripts() {
    dispatch(DispatchToken::Now, ScriptsWillCompileEvent{});
    using namespace scatha;
    std::optional<scatha::Target> target;
    std::stringstream err;
    try {
        auto engineLibDir = projectRootDir() / ".int";
        if (!compileEngineLibrary(engineLibDir)) {
            return;
        }
        CompilerInvocation inv(TargetType::BinaryOnly, "main");
        inv.setLibSearchPaths({ engineLibDir });
        // FIXME: I temporarily disabled optimizations because of bugs in the
        // optimizer
        inv.setOptLevel(0);
        inv.setLinkerOptions({ .searchHost = true });
        auto sources = impl->registry | filter([](auto& entry) {
            return entry.handle.type() == AssetType::ScriptSource;
        }) | transform([&](auto& entry) {
            return SourceFile::load(impl->makeAbsolute(entry.diskLocation));
        });
        inv.setInputs(sources | ranges::to<std::vector>);
        inv.setErrorStream(err);
        target = inv.run();
    }
    catch (std::exception const& e) {
        Logger::Error("Script compiler error: ", e.what(), "\n", err.str());
    }
    if (!target) {
        Logger::Error("Failed to compile scripts: ", err.str());
        return;
    }
    Logger::Info("Successfully compiled scripts");
    ScriptSystem_setTarget(*application().coreSystems().scriptSystem().impl,
                           std::move(*target));
    dispatch(DispatchToken::Now, ScriptsDidCompileEvent{});
}

std::vector<AssetHandle> AssetManager::getAssetsOfType(AssetType type) const {
    std::vector<AssetHandle> result;
    for (auto& entry: impl->registry) {
        if (entry.handle.type() == type) {
            result.push_back(entry.handle);
        }
    }
    return result;
}

Reference<Asset> Impl::allocateAsset(AssetHandle handle,
                                     std::string name) const {
    return dispatchAssetType(
        handle.type(),
        utl::overload{ [&]<typename A>(utl::tag<A>) -> Reference<Asset> {
        return allocateRef<A>(handle, std::move(name));
    }, [&](utl::tag<RenderGraph>) -> Reference<Asset> {
        return allocateRef<RenderGraph>(handle, std::move(name),
                                        assetManager->device());
    } });
}

RegistryEntry* Impl::findByDiskLocation(std::filesystem::path path) {
    if (path.is_relative()) {
        path = makeAbsolute(path);
    }
    BL_EXPECT(isAbsoluteInProjDir(path));
    auto itr = ranges::find(registry, path, &RegistryEntry::diskLocation);
    if (itr == registry.end()) {
        return nullptr;
    }
    return std::to_address(itr);
}

/// MARK: - Disk -> Memory

namespace {

// FIXME: We are binary serializing the data here without worrying about byte
// order

struct StaticMeshHeader {
    size_t vertexDataSize;
    size_t indexDataSize;
};

static constexpr uint32_t TextureFormatVersion = 0;

struct Texture2DHeader {
    uint32_t version = TextureFormatVersion;
    TextureType type = TextureType::Texture1D;
    PixelFormat pixelFormat = PixelFormat::Invalid;
    vml::vector2<uint32_t> dimensions = 0;
    uint32_t numBytes = 0;
    uint8_t numMipMaps = 0;
    bool storesMipMaps = false;
    uint8_t numIrradianceSHCoefficients = 0;
};

} // namespace

static std::optional<StaticMeshHeader> readStaticMeshHeader(
    std::fstream& file) {
    StaticMeshHeader header{};
    file.read((char*)&header, sizeof header);
    if (!file.good()) {
        return std::nullopt;
    }
    return header;
}

static std::optional<Texture2DHeader> readTexture2DHeader(std::fstream& file) {
    Texture2DHeader header{};
    file.read((char*)&header, sizeof header);
    if (!file.good()) {
        return std::nullopt;
    }
    return header;
}

Reference<StaticMeshData> Impl::readStaticMeshFromDisk(
    std::filesystem::path const& source) {
    BL_EXPECT(toExtension(source) == FileExtension::Bmesh);
    auto file = openFile(makeAbsolute(source), std::ios::in | std::ios::binary);
    auto header = readStaticMeshHeader(file);
    if (!header) {
        Logger::Error("File was not a static mesh: ", source);
        return nullptr;
    }
    auto result = allocateRef<StaticMeshData>();
    result->vertices.resize(header->vertexDataSize / sizeof(Vertex3D),
                            utl::no_init);
    result->indices.resize(header->indexDataSize / sizeof(uint32_t),
                           utl::no_init);
    file.read((char*)result->vertices.data(), header->vertexDataSize);
    file.read((char*)result->indices.data(), header->indexDataSize);
    if (file.good()) {
        return result;
    }
    Logger::Error("Failed to read static mesh: ", source);
    return nullptr;
}

void Impl::loadMaterialFromDisk(Material& mat,
                                std::filesystem::path const& source) {
    assert(toExtension(source) == FileExtension::Bmat);
    auto file = openFile(makeAbsolute(source), std::ios::in | std::ios::binary);
    YAML::Node root = YAML::Load(file);
    mat.model->deserialize(root);
}

void Impl::loadMaterialInstanceFromDisk(MaterialInstance& inst,
                                        std::filesystem::path const& source) {
    assert(toExtension(source) == FileExtension::Bmatinst);
    auto file = openFile(makeAbsolute(source), std::ios::in | std::ios::binary);
    YAML::Node root = YAML::Load(file);
    inst.deserialize(root, *assetManager);
}

static bool readBytes(std::istream& stream, void* dest, size_t numBytes) {
    stream.read((char*)dest, numBytes);
    return stream.good();
}

void Impl::loadTextureFromDisk(Texture2D& texture,
                               std::filesystem::path const& source) {
    BL_EXPECT(toExtension(source) == FileExtension::Btex);
    auto file = openFile(makeAbsolute(source), std::ios::in | std::ios::binary);
    auto header = readTexture2DHeader(file);
    if (!header) {
        Logger::Error("File was not a texture: ", source);
        return;
    }
    auto buffer = std::make_unique<uint8_t[]>(header->numBytes);
    if (!readBytes(file, buffer.get(), header->numBytes)) {
        Logger::Error("Failed to read texture pixel data: ", source);
        return;
    }
    texture.type = header->type;
    texture.pixelFormat = header->pixelFormat;
    texture.size = header->dimensions;
    texture.pixelData = std::move(buffer);
    texture.irradianceSHCoefficients.resize(
        header->numIrradianceSHCoefficients);
    texture.numMipMaps = header->numMipMaps;
    texture.storesMipMaps = header->storesMipMaps;
    if (!readBytes(file, texture.irradianceSHCoefficients.data(),
                   header->numIrradianceSHCoefficients *
                       sizeof(texture.irradianceSHCoefficients[0])))
    {
        Logger::Error("Failed to read texture irradiance coefficients: ",
                      source);
        return;
    }
}

void Impl::loadSceneFromDisk(Scene& scene,
                             std::filesystem::path const& source) {
    assert(toExtension(source) == FileExtension::Bscene);
    auto file = openFile(makeAbsolute(source), std::ios::in | std::ios::binary);
    YAML::Node root = YAML::Load(file);
    scene.deserialize(root, *assetManager);
}

void Impl::loadRenderGraphFromDisk(RenderGraph& rg,
                                   std::filesystem::path const& source) {
    assert(toExtension(source) == FileExtension::Brg);
    auto file = openFile(makeAbsolute(source), std::ios::in | std::ios::binary);
    YAML::Node root = YAML::Load(file);
    rg.model().deserialize(root);
}

/// MARK: - Memory -> GPU

void Impl::uploadStaticMeshToGPU(StaticMesh& mesh, RegistryEntry const& entry) {
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

static size_t pixelSize(PixelFormat format) {
    using enum PixelFormat;
    switch (format) {
    case RGBA8Unorm:
    case RGBA8Uint:
    case RGBA8Sint:
    case RGBA8Snorm:
        return 4;
    case RGBA32Float:
    case RGBA32Uint:
    case RGBA32Sint:
        return 16;
    default:
        BL_UNIMPLEMENTED();
    }
}

void Impl::uploadTextureToGPU(Texture2D& texture, RegistryEntry const& entry) {
    bool hasMemoryRepr = texture.pixelData != nullptr;
    if (!hasMemoryRepr) {
        loadTextureFromDisk(texture, entry.diskLocation);
    }
    if (!texture.pixelData || texture.getPixelDataByteSize() == 0) {
        return;
    }
    auto handle = assetManager->device().createTexture(
        { .type = texture.type,
          .pixelFormat = texture.pixelFormat,
          .size = usize3(texture.size, 1),
          .mipmapLevelCount = texture.numMipMaps,
          .sampleCount = 1,
          .storageMode = StorageMode::Managed,
          .usage = TextureUsage::ShaderRead });
    size_t bytesPerRow = texture.size.x * pixelSize(texture.pixelFormat);
    size_t bytesPerImage =
        texture.size.x * texture.size.y * pixelSize(texture.pixelFormat);
    usize3 levelSize = handle.description().size;
    uint8_t* data = texture.pixelData.get();
    for (size_t mipLevel = 0; mipLevel < texture.numMipMaps; ++mipLevel,
                bytesPerRow /= 2, bytesPerImage /= 4,
                levelSize = vml::max(levelSize / 2, size3(1)))
    {
        if (mipLevel >= 1 && !texture.storesMipMaps) {
            break;
        }
        switch (texture.type) {
        case TextureType::Texture2D:
            copyMemoryToTexture(RenderAPI::Metal, handle, data, bytesPerRow,
                                { usize3{}, levelSize }, mipLevel);
            data += bytesPerImage;
            break;
        case TextureType::CubeTexture: {
            for (size_t i = 0; i < 6; ++i, data += bytesPerImage) {
                copyMemoryToTexture(RenderAPI::Metal, handle, data, bytesPerRow,
                                    bytesPerImage, { usize3{}, levelSize },
                                    mipLevel, i);
            }
            break;
        }
        default:
            BL_UNIMPLEMENTED();
        }
    }
    texture.GPUHandle = handle;
    if (!hasMemoryRepr) {
        texture.pixelData = nullptr;
    }
}

/// MARK: - Memory -> Disk

void Impl::flushToDisk(AssetHandle handle, bool force) {
    auto* entry = registry.find(handle);
    if (!entry) {
        Logger::Error("Failed to write asset to disk: ", handle);
        return;
    }
    auto asset = entry->asset.lock();
    if (!asset) {
        return;
    }
    visit(*asset, [&](auto& asset) {
        flushToDiskImpl(asset, makeAbsolute(entry->diskLocation), force);
    });
}

void Impl::flushToDiskImpl(StaticMesh const& mesh,
                           std::filesystem::path const& dest, bool force) {
    auto* data = mesh.mData.get();
    if (!data && !force) {
        Logger::Warn("Could not write mesh to disk: mesh does not exist");
        return;
    }
    std::uint64_t vertexDataSize =
        data->vertices.size() * sizeof(bloom::Vertex3D);
    std::uint64_t indexDataSize = data->indices.size() * sizeof(uint32_t);
    /// Make header
    StaticMeshHeader header{ .vertexDataSize = vertexDataSize,
                             .indexDataSize = indexDataSize };
    std::ios::sync_with_stdio(false);
    auto file = openOrCreateFile(dest, std::ios::out | std::ios::trunc |
                                           std::ios::binary);
    file.write((char*)&header, sizeof header);
    file.write((char*)data->vertices.data(), vertexDataSize);
    file.write((char*)data->indices.data(), indexDataSize);
}

void Impl::flushToDiskImpl(SkeletalMesh const&, std::filesystem::path const&,
                           bool) {
    BL_UNIMPLEMENTED();
}

void Impl::flushToDiskImpl(Material const& mat,
                           std::filesystem::path const& dest,
                           [[maybe_unused]] bool force) {
    YAML::Emitter out;
    out << mat.model->serialize();
    std::ios::sync_with_stdio(false);
    auto file = openOrCreateFile(dest, std::ios::out | std::ios::trunc |
                                           std::ios::binary);
    file << out.c_str();
}

void Impl::flushToDiskImpl(MaterialInstance const& inst,
                           std::filesystem::path const& dest,
                           [[maybe_unused]] bool force) {
    YAML::Emitter out;
    out << inst.serialize();
    std::ios::sync_with_stdio(false);
    auto file = openOrCreateFile(dest, std::ios::out | std::ios::trunc |
                                           std::ios::binary);
    file << out.c_str();
}

void Impl::flushToDiskImpl(Texture2D const& texture,
                           std::filesystem::path const& dest, bool force) {
    if (!texture.pixelData && !force) {
        Logger::Warn("Could not write texture data to disk: no data");
        return;
    }
    std::ios::sync_with_stdio(false);
    auto file = openOrCreateFile(dest, std::ios::out | std::ios::trunc |
                                           std::ios::binary);
    Texture2DHeader header{
        .type = texture.type,
        .pixelFormat = texture.pixelFormat,
        .dimensions = texture.size,
        .numBytes = utl::narrow_cast<uint32_t>(texture.getPixelDataByteSize()),
        .numIrradianceSHCoefficients = utl::narrow_cast<uint8_t>(
            (texture.irradianceSHCoefficients.size())),
        .numMipMaps = texture.numMipMaps,
        .storesMipMaps = texture.storesMipMaps
    };
    file.write((char*)&header, sizeof header);
    file.write((char*)texture.pixelData.get(), header.numBytes);
    file.write((char*)texture.irradianceSHCoefficients.data(),
               texture.irradianceSHCoefficients.size() *
                   sizeof(texture.irradianceSHCoefficients[0]));
}

void Impl::flushToDiskImpl(Scene const& scene,
                           std::filesystem::path const& dest,
                           [[maybe_unused]] bool force) {
    YAML::Emitter out;
    out << scene.serialize();
    std::ios::sync_with_stdio(false);
    auto file = openOrCreateFile(dest, std::ios::out | std::ios::trunc |
                                           std::ios::binary);
    file << out.c_str();
}

void Impl::flushToDiskImpl(ScriptSource const&,
                           std::filesystem::path const& dest,
                           [[maybe_unused]] bool force) {
    if (std::filesystem::exists(dest)) {
        Logger::Warn(
            "Script source is not written to disk because we don't have memory representation");
        return;
    }
    (void)openOrCreateFile(dest, std::ios::out);
}

void Impl::flushToDiskImpl(RenderGraph const& rg,
                           std::filesystem::path const& dest, bool) {
    YAML::Emitter out;
    out << rg.model().serialize();
    std::ios::sync_with_stdio(false);
    auto file = openOrCreateFile(dest, std::ios::out | std::ios::trunc |
                                           std::ios::binary);
    file << out.c_str();
}

std::optional<AssetType> Impl::deduceImportedAssetType(
    std::string_view extView) const {
    std::string ext(extView);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](auto c) { return std::toupper(c); });
    if (ext == ".FBX") {
        return AssetType::StaticMesh;
    }
    if (ext == ".TGA" || ext == ".PNG" || ext == ".EXR" || ext == ".HDR") {
        return AssetType::Texture2D;
    }
    return std::nullopt;
}

void Impl::doImport(Asset& asset, std::filesystem::path const& source) const {
    visit(asset, [&](auto& asset) { doImportImpl(asset, source); });
}

void Impl::doImportImpl(Texture2D& dest,
                        std::filesystem::path const& source) const {
    BL_EXPECT(source.has_filename());
    BL_EXPECT(source.has_extension());
    importTexture(dest, source, assetManager->device(),
                  assetManager->commandQueue());
}

void Impl::doImportImpl(StaticMesh& dest,
                        std::filesystem::path const& source) const {
    BL_EXPECT(source.has_filename());
    BL_EXPECT(source.has_extension());
    dest.mData = allocateRef<StaticMeshData>(importStaticMesh(source));
}

RegistryEntry Impl::makeEntry(WeakReference<Asset> asset, AssetHandle handle,
                              std::string name,
                              std::filesystem::path diskLocation) {
    if (diskLocation.is_relative()) {
        diskLocation = makeAbsolute(diskLocation);
    }
    BL_ASSERT(isAbsoluteInProjDir(diskLocation));
    return RegistryEntry(RegistryEntry::SecretTag{}, std::move(asset), handle,
                         std::move(name), std::move(diskLocation));
}

YAML::Node Impl::serializeEntry(RegistryEntry const& entry) {
    YAML::Node node;
    node["Handle"] = entry.handle;
    node["Name"] = entry.name;
    node["Location"] = makeRelative(entry.diskLocation);
    return node;
}

std::optional<RegistryEntry> Impl::deserializeEntry(YAML::Node const& node) {
    try {
        return RegistryEntry(RegistryEntry::SecretTag{}, {},
                             node["Handle"].as<AssetHandle>(),
                             node["Name"].as<std::string>(),
                             makeAbsolute(
                                 node["Location"].as<std::filesystem::path>()));
    }
    catch (YAML::Exception const& e) {
        Logger::Error("Failed to read registry entry: ", e.what());
        return std::nullopt;
    }
}

std::filesystem::path Impl::makeRelative(
    std::filesystem::path const& path) const {
    if (!path.is_absolute()) {
        return path;
    }
    BL_EXPECT(isAbsoluteInProjDir(path));
    return std::filesystem::relative(path, rootDir);
}

std::filesystem::path Impl::makeAbsolute(
    std::filesystem::path const& path) const {
    if (path.is_relative()) {
        return (rootDir / path).lexically_normal();
    }
    return path;
}

bool Impl::isAbsoluteInProjDir(std::filesystem::path const& path) const {
    auto normal = path.lexically_normal();
    auto [rootEnd, nothing] = ranges::mismatch(rootDir, normal);
    return rootEnd == rootDir.end();
}

std::fstream Impl::openFile(std::filesystem::path const& path) const {
    return openFile(path, std::ios_base::in | std::ios_base::out);
}

std::fstream Impl::openFile(std::filesystem::path const& path,
                            std::ios_base::openmode mode) const {
    BL_EXPECT(isAbsoluteInProjDir(path));
    std::fstream file(path, mode);
    if (!file) {
        throw AssetManagerError("Failed to open file: ", path, " (",
                                strerror(errno), ")");
    }
    return file;
}

std::fstream Impl::openOrCreateFile(std::filesystem::path const& path) const {
    return openOrCreateFile(path, std::ios_base::in | std::ios_base::out);
}

std::fstream Impl::openOrCreateFile(std::filesystem::path const& path,
                                    std::ios_base::openmode mode) const {
    BL_EXPECT(isAbsoluteInProjDir(path));
    std::fstream file(path, mode);
    if (file) {
        return file;
    }
    /// Create the file if it doesn't exist
    file.open(path, mode | std::ios::trunc);
    if (!file) {
        throw AssetManagerError("Failed to open file: ", path, " (",
                                strerror(errno), ")");
    }
    return file;
}

AssetHandle AssetManager::getHandleFromFile(std::filesystem::path path) const {
    if (path.is_relative()) {
        path = impl->makeAbsolute(path);
    }
    auto* entry = impl->findByDiskLocation(path);
    return entry ? entry->handle : AssetHandle{};
}
