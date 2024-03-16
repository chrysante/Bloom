#include "Bloom/Scene/Scene.h"

#include <utl/hashmap.hpp>
#include <utl/stack.hpp>
#include <utl/strcat.hpp>

#include "Bloom/Core/Debug.h"
#include "Bloom/Core/Yaml.h"
#include "Bloom/Scene/Components/Hierarchy.h"
#include "Bloom/Scene/Components/Tag.h"
#include "Bloom/Scene/Components/Transform.h"

using namespace bloom;

EntityHandle Scene::createEmptyEntity() {
    return createEmptyEntity(EntityHandle{});
}

EntityHandle Scene::createEmptyEntity(EntityID hint) {
    return EntityHandle(_registry.create(hint.value()), this);
}

EntityHandle Scene::createEntity(std::string_view name) {
    EntityHandle const entity(_registry.create(), this);
    entity.add(Transform{});
    entity.add(TransformMatrixComponent{});
    entity.add(TagComponent{ std::string(name) });
    entity.add(HierarchyComponent{});

    return { entity, this };
}

EntityHandle Scene::cloneEntity(EntityID from) {
    EntityHandle const result = createEmptyEntity();

    forEachComponent([&]<typename C>(utl::tag<C>) {
        if (hasComponent<C>(from)) {
            C const& c = getComponent<C>(from);
            result.add(c);
        }
    });

    getComponent<HierarchyComponent>(result) = {};
    auto const fromHierarchy = getComponent<HierarchyComponent>(from);
    if (fromHierarchy.parent) {
        parent(result, fromHierarchy.parent);
    }

    return result;
}

void Scene::deleteEntity(EntityID id) { _registry.destroy(id.value()); }

Reference<Scene> Scene::clone() {
    utl::hashmap<EntityID, EntityID> idMap;
    auto result = allocateRef<Scene>(handle(), name());
    each([&](EntityID fromID) {
        EntityHandle fromHandle = this->getHandle(fromID);
        EntityID toID = result->createEmptyEntity(fromID);
        BL_EXPECT(fromID == toID);
        EntityHandle toHandle = result->getHandle(toID);
        forEachComponent([&]<typename C>(utl::tag<C>) {
            if (fromHandle.has<C>()) {
                toHandle.add(fromHandle.get<C>());
            }
        });
    });
    return result;
}

/// MARK: Serialize
template <typename T>
static void serializeComponent(YAML::Node& node, ConstEntityHandle entity,
                               utl::tag<T>) {
    if (entity.has<T>()) {
        node[T::staticName()] = entity.get<T>();
    }
}

static void serializeComponent(YAML::Node& node, ConstEntityHandle entity,
                               utl::tag<MeshRendererComponent>) {
    if (entity.has<MeshRendererComponent>()) {
        node[MeshRendererComponent::staticName()] =
            entity.get<MeshRendererComponent>().serialize();
    }
}

template <typename T>
static void deserializeComponent(YAML::Node const& node, EntityHandle entity,
                                 AssetManager&, utl::tag<T>) {
    YAML::Node componentNode = node[T::staticName()];
    if (componentNode.IsDefined()) {
        entity.add(componentNode.as<T>());
    }
}

static void deserializeComponent(YAML::Node const& node, EntityHandle entity,
                                 AssetManager& assetManager,
                                 utl::tag<MeshRendererComponent>) {
    if (YAML::Node componentNode = node[MeshRendererComponent::staticName()];
        componentNode.IsDefined())
    {
        entity.add(MeshRendererComponent{});
        entity.get<MeshRendererComponent>().deserialize(componentNode,
                                                        assetManager);
    }
}

static YAML::Node serializeEntity(Scene const& scene, EntityID id) {
    YAML::Node node;
    node["ID"] = id.raw();
    auto entity = scene.getHandle(id);
    forEachComponent(except<TransformMatrixComponent>,
                     [&]<typename T>(utl::tag<T>) {
        serializeComponent(node, entity, utl::tag<T>{});
    });
    return node;
}

static void deserializeEntity(YAML::Node const& node, Scene& scene,
                              AssetManager& assetManager) {
    EntityID reference(node["ID"].as<EntityID::RawType>());
    EntityID id = scene.createEmptyEntity(reference);
    auto entity = scene.getHandle(id);
    entity.add(TransformMatrixComponent{});
    assert(reference == entity);
    forEachComponent(except<TransformMatrixComponent>,
                     [&]<typename T>(utl::tag<T>) {
        deserializeComponent(node, entity, assetManager, utl::tag<T>{});
    });
}

YAML::Node Scene::serialize() const {
    YAML::Node root;
    each([&](EntityID entity) {
        root.push_back(serializeEntity(*this, entity));
    });
    return root;
}

void Scene::deserialize(YAML::Node const& root, AssetManager& assetManager) {
    if (!root.IsSequence()) {
        Logger::Info("Failed to deserialize Scene: Scene is empty.");
        return;
    }
    clear();
    for (YAML::Node const& node: root) {
        deserializeEntity(node, *this, assetManager);
    }
}

[[maybe_unused]] static void sanitizeHierachy(Scene* scene) {
    auto view = scene->view<HierarchyComponent>();
    view.each([&](auto e, HierarchyComponent entity) {
        EntityID entityID = e;
        /// Sanitize child -> parent relationship
        if (entity.parent) {
            EntityID parentID = entity.parent;
            auto parentsChildren = scene->gatherChildren(parentID);
            if (std::find(parentsChildren.begin(), parentsChildren.end(),
                          entityID) == parentsChildren.end())
            {
                Logger::Fatal("Entity ", entityID, " [",
                              scene->getComponent<TagComponent>(entityID).name,
                              "] thinks Entity ", parentID, " [",
                              scene->getComponent<TagComponent>(parentID).name,
                              "] is its "
                              "parent but ",
                              parentID, " doesn't know about it.");
                BL_UNREACHABLE();
            }
        }
        /// Sanitize parent -> children relationship
        if (entity.firstChild) {
            BL_ASSERT(entity.lastChild);
            auto ourChildren = scene->gatherChildren(entityID);
            for (auto childID: ourChildren) {
                auto child = scene->getComponent<HierarchyComponent>(childID);
                if (child.parent != entityID) {
                    Logger::Fatal(
                        "Entity ", entityID, " [",
                        scene->getComponent<TagComponent>(entityID).name,
                        "] thinks it's a parent but "
                        "supposed child ",
                        entityID, " [",
                        scene->getComponent<TagComponent>(entityID).name,
                        "] doesn't know about it.");
                    BL_UNREACHABLE();
                }
            }
        }
        /// Sanitize relationship among siblings
        if (entity.prevSibling) {
            BL_ASSERT(entity.nextSibling);
            auto leftSibling =
                scene->getComponent<HierarchyComponent>(entity.prevSibling);
            auto rightSibling =
                scene->getComponent<HierarchyComponent>(entity.nextSibling);
            BL_ASSERT(leftSibling.nextSibling == entityID,
                      "Our left sibling doesn't know about us");
            BL_ASSERT(rightSibling.prevSibling == entityID,
                      "Our right sibling doesn't know about us");
        }
    });
}

void Scene::parent(EntityID c, EntityID p) {
    HierarchyComponent& parent = getComponent<HierarchyComponent>(p);
    HierarchyComponent& newChild = getComponent<HierarchyComponent>(c);
    BL_ASSERT(!newChild.parent);
    newChild.parent = p;
    // Parent has no children yet
    if (!parent.firstChild) {
        BL_ASSERT(!parent.lastChild);
        parent.firstChild = c;
        parent.lastChild = c;
        newChild.prevSibling = c;
        newChild.nextSibling = c;
    }
    // Parent already has children
    else {
        EntityID listBegin = parent.firstChild;
        EntityID listEnd = parent.lastChild;
        HierarchyComponent& leftChild =
            getComponent<HierarchyComponent>(listBegin);
        HierarchyComponent& rightChild =
            getComponent<HierarchyComponent>(listEnd);
        rightChild.nextSibling = c;
        leftChild.prevSibling = c;
        newChild.prevSibling = listEnd;
        newChild.nextSibling = listBegin;
        parent.lastChild = c;
    }
    auto& t = getComponent<Transform>(c);
    mtl::float4x4 parentWorldTransform = calculateTransformRelativeToWorld(p);
    mtl::float4x4 childLocalTransform =
        mtl::inverse(parentWorldTransform) * t.calculate();
    t = Transform::fromMatrix(childLocalTransform);
#if BLOOM_DEBUGLEVEL
    sanitizeHierachy(this);
#endif
}

void Scene::unparent(EntityID c) {
    auto& child = getComponent<HierarchyComponent>(c);
    if (!child.parent) {
        return;
    }
    auto& parent = getComponent<HierarchyComponent>(child.parent);
    auto& leftSibling = getComponent<HierarchyComponent>(child.prevSibling);
    auto& rightSibling = getComponent<HierarchyComponent>(child.nextSibling);
    /// Remove element from circular linked list
    leftSibling.nextSibling = child.nextSibling;
    rightSibling.prevSibling = child.prevSibling;
    /// Adjust begin/end nodes if we were those
    if (c == parent.firstChild) {
        parent.firstChild = child.nextSibling;
    }
    if (c == parent.lastChild) {
        parent.lastChild = child.prevSibling;
    }
    /// Either both are true or neither is true
    assert(!((parent.firstChild == c) ^ (parent.lastChild == c)));
    /// Only one element was in the list
    if (parent.firstChild == c) {
        assert(c == child.prevSibling);
        assert(c == child.nextSibling);
        parent.lastChild = {};
        parent.firstChild = {};
    }
    auto& t = getComponent<Transform>(c);
    mtl::float4x4 const parentWorldTransform =
        calculateTransformRelativeToWorld(child.parent);
    mtl::float4x4 const childWorldTransform =
        parentWorldTransform * t.calculate();
    t = Transform::fromMatrix(childWorldTransform);
    child.parent = {};
    child.prevSibling = {};
    child.nextSibling = {};
#if BLOOM_DEBUGLEVEL
    sanitizeHierachy(this);
#endif
}

bool Scene::descendsFrom(EntityID descended, EntityID ancestor) const {
    if (!descended) {
        return false;
    }
    BL_ASSERT(hasComponent<HierarchyComponent>(descended));
    BL_ASSERT(hasComponent<HierarchyComponent>(ancestor));
    /// Walk up the tree until we find our potentil ancestor or the root
    do {
        if (descended == ancestor) {
            return true;
        }
        descended = getComponent<HierarchyComponent>(descended).parent;
    } while (descended);
    return false;
}

utl::small_vector<EntityID> Scene::gatherRoots() const {
    utl::small_vector<EntityID> roots;
    for (auto [entity, hierachy]: view<HierarchyComponent>().each()) {
        if (!hierachy.parent) {
            roots.push_back(EntityID(entity));
        }
    }
    return roots;
}

utl::small_vector<EntityID> Scene::gatherChildren(EntityID parent) const {
    if (!hasComponent<HierarchyComponent>(parent)) {
        return {};
    }
    auto& hierarchy = getComponent<HierarchyComponent>(parent);
    auto current = hierarchy.firstChild;
    auto end = hierarchy.lastChild;
    utl::small_vector<EntityID> children;
    while (current) {
        children.push_back(current);
        if (current == end) {
            break;
        }
        current = getComponent<HierarchyComponent>(current).nextSibling;
    }
    return children;
}

bool Scene::isLeaf(EntityID entity) const {
    bool const result = !getComponent<HierarchyComponent>(entity).firstChild;
    BL_ASSERT(result == !getComponent<HierarchyComponent>(entity).lastChild);
    return result;
}

mtl::float4x4 Scene::calculateTransformRelativeToWorld(EntityID entity) const {
    mtl::float4x4 result = 1;
    while (entity) {
        BL_ASSERT(hasComponent<HierarchyComponent>(entity),
                  "This API is supposed to be used with hierarchical entities");
        result *= getComponent<Transform>(entity).calculate();
        entity = getComponent<HierarchyComponent>(entity).parent;
    }
    return result;
}
