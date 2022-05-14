#define IMGUI_DEFINE_MATH_OPERATORS

#include "Viewport.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include "Poppy/ImGui/ImGui.hpp"

#include <utl/stdio.hpp>

#include "Bloom/Scene/Components/AllComponents.hpp"
#include "Bloom/Scene/SceneSystem.hpp"
#include "Bloom/Graphics/RenderContext.hpp"
#include "Bloom/Application/Application.hpp"
#include "Bloom/Application/Event.hpp"
#include "Bloom/Assets/ConcreteAssets.hpp"
 
#include "Poppy/Editor.hpp"
#include "Poppy/SelectionContext.hpp"

#include "AssetBrowser.hpp"

#include "Bloom/Platform/Metal/MetalRenderContext.hpp"
#include <Metal/Metal.hpp>

using namespace mtl::short_types;
using namespace bloom;

template<> struct YAML::convert<poppy::Viewport::Parameters> {
	static Node encode(poppy::Viewport::Parameters const& p) {
		Node node;
		node["Field of View"] = p.fieldOfView;
		return node;
	}
	
	static bool decode(Node const& node, poppy::Viewport::Parameters& p) {
		try {
			p.fieldOfView = node["Field of View"].as<float>();
			return true;
		}
		catch (InvalidNode const&) {
			return false;
		}
	}
};

namespace poppy{
	
	std::string_view toString(Projection proj) {
		return std::array {
			"Perspective",
			"Orthogonal"
		}[(std::size_t)proj];
	}
	
	Viewport::Viewport(bloom::Renderer* r):
		Panel("Viewport", PanelOptions{ .unique = false }),
		renderer(r)
	{
		padding = 0;
	}

	void Viewport::onEvent(bloom::Event& event) {
		event.dispatch<bloom::EventType::leftMouseDown>([&](bloom::MouseEvent const& e) {
			if (!viewportHovered || gizmo.isHovered()) {
				return;
			}
			
			auto const locationInView = windowSpaceToViewSpace(e.locationInWindow);
			auto const entity = readEntityID(locationInView);
			if (entity) {
				selection()->select(entity);
			}
			else {
				selection()->clear();
			}
		});
		
		event.dispatch<bloom::EventType::keyDown>([&](bloom::KeyEvent const& e) {
			using bloom::Key;
			switch (e.key) {
				case Key::tab:
					gizmo.cycleSpace();
					break;
				case Key::_1:
					gizmo.setOperation(Gizmo::Operation::translate);
					break;
				case Key::_2:
					gizmo.setOperation(Gizmo::Operation::rotate);
					break;
				case Key::_3:
					gizmo.setOperation(Gizmo::Operation::scale);
					break;
					
				case Key::G:
					gameView ^= true;
					
				default:
					break;
			}
		});
	}
	
	void Viewport::init() {
		params = settings["Parameters"].as<Parameters>(Parameters{});
		cameraActor = settings["Camera"].as<ViewportCameraActor>(ViewportCameraActor{});
		cameraActor.applyTransform();
		gizmo.setInput(getApplication().input());
		overlays.init(this);
	}
	
	void Viewport::shutdown() {
		settings["Parameters"] = params;
		settings["Camera"] = cameraActor;
	}
	
	void Viewport::display() {
		if (scene()) {
			drawScene(getApplication().renderer());
			
			ImGui::Image(gameView ?
						 frameBuffer.finalImage.nativeHandle() :
						 frameBuffer.finalImageEditor.nativeHandle(),
						 ImGui::GetWindowSize());
			if (!gameView) {
				overlays.display();
			}
		}
		
		gizmo.display(cameraActor.camera, scene(), selection());
		displayControls();
		
		auto const wantsInput = detectViewportInput(ImGuiButtonFlags_MouseButtonRight);
		viewportHovered = wantsInput.hovered;
		if (wantsInput.held) {
			cameraActor.update(getApplication().getRenderTime(), getApplication().input());
		}
		
		if (!getApplication().getSceneSystem()->isRunning() && GImGui->DragDropActive) {
			float const spacing = 6.5;
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::SetCursorPos({ spacing, spacing });
			ImGui::InvisibleButton("viewport-drag-drop-button",
								   ImGui::GetContentRegionAvail() - ImVec2{ spacing, spacing });
			ImGui::PopItemFlag();
			recieveSceneDragDrop();
		}
	}
	
	template <typename E>
	static bool inputControlCombo(char const* ID, E& e,
								  std::size_t count = (std::size_t)E::_count,
								  bool disabled = false)
	{
		using namespace bloom;
		
		auto* defaultFont = ImGui::GetFont();
		auto* boldFont = (ImFont*)poppy::ImGuiContext::instance().getFont(FontWeight::bold, FontStyle::roman);
		
		int comboFlags = 0;
		comboFlags |= ImGuiComboFlags_NoArrowButton;
		
		if (disabled) {
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::PushStyleColor(ImGuiCol_Text, GImGui->Style.Colors[ImGuiCol_TextDisabled]);
		}
		
		ImGui::PushFont(boldFont);
		
		auto const current = toString(e);
		float const width = ImGui::CalcTextSize(current.data()).x + GImGui->Style.FramePadding.x * 2;
		ImGui::SetNextItemWidth(width);
		
		bool result = false;
		if (ImGui::BeginCombo(ID, toString(e).data(), comboFlags)) {
			ImGui::PushFont(defaultFont);
			for (auto i: utl::enumerate<E>(count)) {
				bool const selected = e == i;
				if (ImGui::Selectable(toString(i).data(), selected)) {
					e = i;
					result = true;
				}
				if (selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::PopFont();
			ImGui::EndCombo();
		}
		ImGui::PopFont();
		
		if (disabled) {
			ImGui::PopItemFlag();
			ImGui::PopStyleColor();
		}
		return result;
	}
	
	void Viewport::displayControls() {
		ImGui::SetCursorPos({ 5, 5 });
		
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 5, 5 });
		if (ImGui::BeginCombo("##displayControlMenu", nullptr, ImGuiComboFlags_NoPreview)) {
			
			ImGui::Checkbox("Game View", &gameView);
			ImGui::SetNextItemWidth(100);
			ImGui::SliderFloat("Field of View", &params.fieldOfView, 30, 180);
			ImGui::SliderFloat("Near Clip Plane", &params.nearClip, 0, 1);
			ImGui::Checkbox("Alternate Light Frustum", &drawOptions.alternateLightFrustum);
			if (ImGui::Checkbox("Visualize Shadow Cascades", &drawOptions.visualizeShadowCascades) &&
				drawOptions.lightVizEntityID == 0xFFffFFff)
			{
				for (auto&& [entity, light]: scene()->view<DirectionalLightComponent const>().each()) {
					if (light.light.castsShadows) {
						drawOptions.lightVizEntityID = (uint32_t)entity;
						break;
					}
				}
			}
			if (drawOptions.visualizeShadowCascades) {
				EntityID const lightEntity{ drawOptions.lightVizEntityID };
				
				char const* preview = nullptr;
				if (!lightEntity ||
					getLightType(lightEntity) == LightType::none ||
					!scene()->getComponent<DirectionalLightComponent>(lightEntity).light.castsShadows)
				{
					preview = "Select Shadow Caster";
				}
				else {
					preview = scene()->getComponent<TagComponent>(lightEntity).name.data();
				}
				
				if (ImGui::BeginCombo("Shadow Caster", preview)) {
					for (auto&& [entity, light, tag]: scene()->view<DirectionalLightComponent const, TagComponent const>().each()) {
						if (light.light.castsShadows && ImGui::Selectable(tag.name.data())) {
							drawOptions.lightVizEntityID = (uint32_t)entity;
						}
					}
					ImGui::EndCombo();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::PopStyleVar();
		
		ImGui::SameLine();
		inputControlCombo("##DrawMode", drawOptions.mode);
		ImGui::SameLine();
		if (auto operation = gizmo.operation(); inputControlCombo("##-gizmo-operation", operation, 3)) {
			gizmo.setOperation(operation);
		}
		ImGui::SameLine();
		if (auto space = gizmo.space();
			inputControlCombo("##gizmo-space",
							  space, 2,
							  /* disable =  */ gizmo.operation() == Gizmo::Operation::scale))
		{
			gizmo.setSpace(space);
		}
		ImGui::SameLine();
		inputControlCombo("##Projection", cameraProjection, 2);
	}
	
	void Viewport::drawScene(Renderer& renderer) {
		using namespace bloom;
		updateRenderTarget(renderer, ImGui::GetWindowSize());
		
		auto& camera = cameraActor.camera;
		if (cameraProjection == Projection::perspective) {
			camera.setProjection(mtl::to_radians(params.fieldOfView), frameBuffer.size(), params.nearClip);
		}
		else {
			float const width = 1000; // ??
			float const aspect = frameBuffer.size().y / float(frameBuffer.size().x);
			float height = width * aspect;
			camera.setProjectionOrtho(-width / 2, width / 2,
									  -height / 2, height / 2,
									  0, 5000);
		}
		
		renderer.beginScene(cameraActor.camera, drawOptions);
		
		auto* const sceneSystem = getApplication().getSceneSystem();
		
		auto lock = sceneSystem->lock();
		
		sceneSystem->applyTransformHierarchy();
		
		sceneSystem->sumbitToRenderer(renderer);
		
		for (auto id: selection()->ids()) {
			auto entity = scene()->getHandle(id);
			if (!entity.has<TransformMatrixComponent>() || !entity.has<MeshRendererComponent>()) {
				continue;
			}
			auto const& transform = entity.get<TransformMatrixComponent>();
			auto const& meshRenderer = entity.get<MeshRendererComponent>();
			if (!meshRenderer.mesh || !meshRenderer.material) {
				continue;
			}
			EntityRenderData entityData;
			entityData.transform = transform.matrix;
			entityData.ID = id.raw();
			renderer.submitSelected(meshRenderer.mesh->getRenderMesh(),
									Reference<Material>(meshRenderer.material, &meshRenderer.material->material),
									entityData);
		}
		
		do if (drawOptions.visualizeShadowCascades) {
			auto const entity = scene()->getHandle(EntityID{ drawOptions.lightVizEntityID });
			if (!entity.has<DirectionalLightComponent>()) {
				break;
			}
			auto const& transform = entity.get<TransformComponent>();
			auto light = entity.get<DirectionalLightComponent>();
			light.light.direction = mtl::rotate({ 0, 0, 1 }, transform.orientation);
			renderer.submitShadowCascadeViz(light.light);
		} while (0);
		
		lock.unlock();
		
		renderer.endScene();
		
		renderer.draw(&frameBuffer);
		if (!gameView) {
			renderer.drawDebugInfo(&frameBuffer);
		}
	}
	
	void Viewport::updateRenderTarget(Renderer& renderer, mtl::usize2 size) {
		size *= 2; // TODO: Fix global DPI Scale
		if (frameBuffer.size() != size) {
			auto* const renderContext = renderer.getRenderContext();
			frameBuffer = bloom::EditorFrameBuffer::create(renderContext, size.x, size.y);
		}
	}
	
	bloom::EntityID Viewport::readEntityID(mtl::float2 const mousePosition) {
		static_assert(std::is_same_v<std::underlying_type_t<entt::entity>, std::uint32_t>);
		
		if (!scene()) {
			return {};
		}
		
		if (mousePosition.x < 0 || mousePosition.y < 0) {
			return {};
		}
		auto const panelSize = this->size();
		if (mousePosition.x >= panelSize.x || mousePosition.y >= panelSize.y) {
			return {};
		}
		
		auto* entityTexture = (MTL::Texture*)frameBuffer.entityID.nativeHandle();
		auto* mtlRenderContext = utl::down_cast<bloom::MetalRenderContext*>(renderer->getRenderContext());
		auto* commandBuffer = mtlRenderContext->commandQueue()->commandBuffer();
		auto* encoder = commandBuffer->blitCommandEncoder();
		encoder->synchronizeTexture(entityTexture, 0, 0);
		encoder->endEncoding();
		commandBuffer->commit();
		commandBuffer->waitUntilCompleted();
		
		std::uint32_t id = -1;
		auto const region = MTL::Region(mousePosition.x * 2, mousePosition.y * 2, 1, 1);
		entityTexture->getBytes(&id, 4, region, 0);
		
		return bloom::EntityID((entt::entity)id);
	}
	
	void Viewport::recieveSceneDragDrop() {
		auto payload = acceptAssetDragDrop(AssetType::scene);
		if (payload) {
			AssetHandle const handle = *payload;
			auto scene = assetManager()->get(handle);
			assetManager()->makeAvailable(handle, AssetRepresentation::CPU);
			getEditor().setScene(as<SceneAsset>(scene));
		}
	}
	
	mtl::float3 Viewport::worldSpaceToViewSpace(mtl::float3 const positionWS) {
		auto const viewProj = cameraActor.camera.viewProjection();
		auto const tmp = viewProj * mtl::float4(positionWS, 1);
		auto const ndc = tmp.xyz / tmp.w;
		
		auto posVS = ndc.xy;
		
		posVS = (posVS + 1) / 2;
		posVS.y = 1 - posVS.y;
		posVS *= this->size();
		return { posVS, ndc.z };
	}
	
	mtl::float3 Viewport::worldSpaceToWindowSpace(mtl::float3 position) {
		auto const vs = worldSpaceToViewSpace(position);
		return { viewSpaceToWindowSpace(vs.xy), vs.z };
	}
	
	mtl::float2 Viewport::worldSpaceDirToViewSpace(mtl::float3 const positionWS) {
		auto const viewProj = cameraActor.camera.viewProjection();
		auto const tmp = viewProj * mtl::float4(positionWS, 0);
		auto const ndc = tmp.xyz / tmp.w;
		
		auto dirVS = ndc.xy;
		
		
		dirVS.y = 1 - dirVS.y;
		dirVS *= this->size();
		return dirVS;
	}
	
	mtl::float2 Viewport::worldSpaceDirToWindowSpace(mtl::float3 direction) {
		return worldSpaceDirToViewSpace(direction);
	}
	
}
