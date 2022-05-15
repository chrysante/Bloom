#include <Catch2/Catch2.hpp>

#include "Bloom/Assets/AssetManager.hpp"
#include "Bloom/Assets/ConcreteAssets.hpp"

#include <iostream>

using namespace bloom;

static auto getDirectories() {
	auto const projRootDir = (std::filesystem::current_path() / "../../..").lexically_normal();
	auto const workingDir = projRootDir / "Bloom/Tests/TestResources/Assets";
	std::filesystem::remove_all(workingDir);
	return std::pair{ workingDir, projRootDir };
}

TEST_CASE("AssetManager : Import and load Static Mesh") {
	auto const [workingDir, projRootDir] = getDirectories();
	
	AssetHandle handle;
	{
		AssetManager m;
		m.setWorkingDir(workingDir);

		handle = m.import(projRootDir / "Assets/Geometry/Cube.fbx", ".");
	}
	{
		AssetManager m;
		m.setWorkingDir(workingDir);
		auto cubeAsset = m.get(handle);
		StaticMeshAsset* mesh = utl::down_cast<StaticMeshAsset*>(cubeAsset.get());
		m.makeAvailable(handle, AssetRepresentation::CPU);
		CHECK(mesh->getMeshData()->vertices.size() == 4 * 6);
	}
}


TEST_CASE("AssetManager : Save and load Scene") {
	auto const [workingDir, projRootDir] = getDirectories();

	EntityID e0, e1;
	
	AssetHandle handle;
	{
		AssetManager m;
		m.setWorkingDir(workingDir);

		auto const meshHandle = m.import(projRootDir / "Assets/Geometry/Cube.fbx", ".");
		
		Scene scene;
		e0 = scene.createEntity("Entity0");
		scene.addComponent(e0, PointLightComponent{ PointLight{ { .color = {1, 0, 0} } } });
		e1 = scene.createEntity("Entity1");
		scene.addComponent(e1, SpotLightComponent{ SpotLight{ { .color = {0, 1, 0} } } });
		scene.addComponent(e1, MeshRendererComponent{
			.material = nullptr,
			.mesh = as<StaticMeshAsset>(m.get(meshHandle))
		});

		auto asset = m.create(AssetType::scene, "TestScene", ".");
		handle = asset->handle();
		utl::down_cast<SceneAsset*>(asset.get())->scene = std::move(scene);
		m.saveToDisk(handle);
	}
	{
		AssetManager m;
		m.setWorkingDir(workingDir);
		auto asset = m.get(handle);
		SceneAsset* sceneAsset = utl::down_cast<SceneAsset*>(asset.get());
		m.makeAvailable(handle, AssetRepresentation::CPU);
		auto& scene = sceneAsset->scene;
		REQUIRE(scene.hasComponent<PointLightComponent>(e0));
		CHECK(scene.getComponent<PointLightComponent>(e0).light.common.color == mtl::float3{ 1, 0, 0 });
		REQUIRE(scene.hasComponent<SpotLightComponent>(e1));
		CHECK(scene.getComponent<SpotLightComponent>(e1).light.common.color == mtl::float3{ 0, 1, 0 });
	}
}

TEST_CASE("AssetManager : create Material") {
	auto const [workingDir, rootProjDir] = getDirectories();
	
	AssetManager m;
	m.setWorkingDir(workingDir);
	
	auto material = m.create(AssetType::material, "Default Material", "./Materials");
	
}
