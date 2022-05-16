#include "SceneSerialize.hpp"

#include "Scene.hpp"
#include "Components/AllComponents.hpp"
#include "Bloom/Assets/AssetManager.hpp"

#include <yaml-cpp/yaml.h>

namespace bloom {

	template <typename T>
	static void serializeComponent(YAML::Node& node, ConstEntityHandle entity, utl::tag<T>) {
		if (entity.has<T>()) {
			node[T::staticName()] = entity.get<T>();
		}
	}
	
	static void serializeComponent(YAML::Node& node, ConstEntityHandle entity, utl::tag<MeshRendererComponent>) {
		if (entity.has<MeshRendererComponent>()) {
			node[MeshRendererComponent::staticName()] = encode(entity.get<MeshRendererComponent>());
		}
	}
	
	template <typename T>
	static void deserializeComponent(YAML::Node const& node, EntityHandle entity, AssetManager* assetManager, utl::tag<T>) {
		YAML::Node componentNode = node[T::staticName()];
		if (componentNode.IsDefined()) {
			entity.add(componentNode.as<T>());
		}
	}
	
	static void deserializeComponent(YAML::Node const& node, EntityHandle entity, AssetManager* assetManager, utl::tag<MeshRendererComponent>) {
		if (YAML::Node componentNode = node[MeshRendererComponent::staticName()];
			componentNode.IsDefined())
		{
			entity.add(decodeMeshRendererComponent(componentNode, assetManager));
		}
	}
	
	static YAML::Node serializeEntity(Scene const* scene, EntityID id) {
		YAML::Node node;
		node["ID"] = id.raw();
		auto entity = scene->getHandle(id);
		forEachComponent(except<TransformMatrixComponent>, [&]<typename T>(utl::tag<T>) {
			serializeComponent(node, entity, utl::tag<T>{});
		});
		return node;
	}
	
	static void deserializeEntity(YAML::Node const& node, Scene* scene, AssetManager* assetManager) {
		EntityID const reference(node["ID"].as<EntityID::RawType>());
		EntityID const id = scene->createEmptyEntity(reference);
		auto entity = scene->getHandle(id);
		entity.add(TransformMatrixComponent{});
		bloomAssert(reference == entity);
		forEachComponent(except<TransformMatrixComponent>, [&]<typename T>(utl::tag<T>) {
			deserializeComponent(node, entity, assetManager, utl::tag<T>{});
		});
	}
	
	BLOOM_API std::string serialize(Scene const* scene) {
		YAML::Node root;
		scene->each([&](entt::entity entity){
			root.push_back(serializeEntity(scene, entity));
		});
		YAML::Emitter out;
		out << root;
		return out.c_str();
	}

	BLOOM_API void deserialize(std::string text, Scene* scene, AssetManager* m) {
		YAML::Node root = YAML::Load(std::move(text));
		
		if (!root.IsSequence()) {
			bloomLog(info, "Failed to deserialize Scene: Scene is empty.");
			return;
		}
		
		for (YAML::Node const& node: root) {
			deserializeEntity(node, scene, m);
		}
	}
	
}
