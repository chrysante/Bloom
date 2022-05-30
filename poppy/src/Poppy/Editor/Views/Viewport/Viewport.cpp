#define IMGUI_DEFINE_MATH_OPERATORS

#include "Viewport.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include "Poppy/UI/ImGuiHelpers.hpp"

#include <utl/stdio.hpp>
#include <utl/stack.hpp>

#include "Bloom/Scene/Scene.hpp"
#include "Bloom/Scene/Components/AllComponents.hpp"
#include "Bloom/Runtime/SceneSystem.hpp"
#include "Bloom/Application/Application.hpp"
#include "Bloom/Application/InputEvent.hpp"
#include "Bloom/Asset/AssetManager.hpp"
#include "Bloom/Graphics/Renderer/ForwardRenderer.hpp"

#include "Poppy/Editor/Editor.hpp"
#include "Poppy/Editor/SelectionContext.hpp"
#include "Poppy/Core/Profile.hpp"
#include "Poppy/Renderer/EditorRenderer.hpp"

#include "AssetBrowser.hpp"

using namespace mtl::short_types;
using namespace bloom;

namespace poppy{
	
	template <typename E>
	static bool enumCombo(E& e, std::size_t count = (std::size_t)E::_count) {
		bool result = false;
		for (auto i: utl::enumerate<E>(count)) {
			bool const selected = e == i;
			auto const label = toString(i);
			bool const pressed = ImGui::Selectable(label.data(), selected, ImGuiSelectableFlags_SpanAvailWidth);
			if (pressed) {
				e = i;
				result = true;
			}
			if (selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		return result;
	}
	
	POPPY_REGISTER_VIEW(Viewport, "Viewport", {});
	
	Viewport::Viewport(): BasicSceneInspector(this) {
		toolbar = {
			ToolbarDropdownMenu().content([this]{
				dropdownMenu();
			}).minWidth(400),
			
			ToolbarSpacer{},
			
//			ToolbarDropdownMenu().content([this]{
//				enumCombo(drawOptions.mode);
//			}).previewValue([this]{
//				return toString(drawOptions.mode);
//			}),
			
			ToolbarDropdownMenu().content([this]{
				auto operation = gizmo.operation();
				if (enumCombo(operation)) {
					gizmo.setOperation(operation);
				}
			}).previewValue([this]{
				return toString(gizmo.operation());
			}),
			
			ToolbarDropdownMenu().content([this]{
				auto space = gizmo.space();
				if (enumCombo(space)) {
					gizmo.setSpace(space);
				}
			}).previewValue([this]{
				return toString(gizmo.space());
			}),
			
			ToolbarDropdownMenu().content([this]{
				enumCombo(camera.data.projection);
			}).previewValue([this]{
				return toString(camera.data.projection);
			}),
			
			ToolbarSpacer{},
			
			ToolbarIconButton([this]{ return maximized() ? "resize-small" : "resize-full"; }).onClick([this]{
				bool const isMaximized = maximized();
				dispatch(DispatchToken::nextFrame, CustomCommand{ [=]{
					if (isMaximized) {
						restore();
						window().makeWindowed();
					}
					else {
						maximize();
						window().makeFullscreen();
					}
				} });
			})
		};
	}

	void Viewport::init() {
		setPadding(0);
		
		toolbar.setStyle({
			.height = 25,
			.buttonAlpha = 1,
			.buttonAlphaHovered = 1,
			.buttonAlphaActive = 1,
		});
		
		sceneRenderer.setRenderer(editor().coreSystems().renderer());
		
		gizmo.setInput(window().input());
		overlays.init(this);
	}
	
	void Viewport::shutdown() {
		
	}
	
	void Viewport::frame() {
		ImGui::BeginChild("Viewport Child");
		if (scenes().empty()) {
			displayEmptyWithReason("No active Scene");
		}
		else {
			displayScene();
		}
		
		/* Display toolbar with padding */ {
			float2 const padding = GImGui->Style.WindowPadding;
			ImGui::SetCursorPos(padding);
			auto* const window = ImGui::GetCurrentWindow();
			auto color = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
			color.w = 0.5;
			window->DrawList->AddRectFilled(window->Pos, (float2)window->Pos + float2{ window->Size.x, toolbar.getHeight() + 2 * padding.y },
											ImGui::ColorConvertFloat4ToU32(color));
			toolbar.display(size().x - 2 * padding.x);
		}
		
		if (!scenes().empty()) {
			auto const wantsInput = detectViewportInput(ImGuiButtonFlags_MouseButtonRight);
			viewportHovered = wantsInput.hovered;
			if (wantsInput.held) {
#warning Magic Var
				camera.update(Timestep{ 0, 0.166 }, window().input());
			}
		}
		
		if (/*!isSimulating() && */GImGui->DragDropActive) {
			float const spacing = 6.5;
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::SetCursorPos({ spacing, spacing });
			ImGui::InvisibleButton("viewport-drag-drop-button",
								   ImGui::GetContentRegionAvail() - ImVec2{ spacing, spacing });
			ImGui::PopItemFlag();
			recieveSceneDragDrop();
		}
		
		ImGui::EndChild();
	}

	YAML::Node Viewport::serialize() const {
		YAML::Node node;
		node["Parameters"] = params;
		node["Camera"] = camera;
		return node;
	}
	
	void Viewport::deserialize(YAML::Node node) {
		params = node["Parameters"].as<Parameters>();
		camera = node["Camera"].as<ViewportCameraController>();
		camera.applyTransform();
	}
	
	void* Viewport::selectImage() const {
		auto* const fwFramebuffer = dynamic_cast<ForwardRendererFramebuffer*>(framebuffer.get());
		
		if (fwFramebuffer && params.framebufferSlot != Parameters::FramebufferElements::postprocessed) {
			switch (params.framebufferSlot) {
				case Parameters::FramebufferElements::depth:
					return fwFramebuffer->depth.nativeHandle();
				case Parameters::FramebufferElements::raw:
					return fwFramebuffer->rawColor.nativeHandle();
				default:
					break;
			}
		}
		
		if (!gameView) {
			return editorFramebuffer->composed.nativeHandle();
		}
		
		if (fwFramebuffer) {
			return fwFramebuffer->postProcessed.nativeHandle();
		}
		
		return nullptr;
	}
	
	void Viewport::displayScene() {
		updateFramebuffer();
		if (!framebuffer) {
			return;
		}
		drawScene();
		
		void* const image = selectImage();
		if (!image) {
			return;
		}
		
		ImGui::Image(image, ImGui::GetWindowSize());
		
		if (!gameView) {
			overlays.display();
			gizmo.display(camera.camera, selection());
		}
	}
	
	void Viewport::drawScene() {
		if (!framebuffer) {
			return;
		}
		POPPY_PROFILE(frame);
		camera.applyProjection(framebuffer->size);
		
		
		auto& sceneSystem = editor().coreSystems().sceneSystem();
		sceneSystem.applyTransformHierarchy();
		
		if (gameView) {
			sceneRenderer.draw(sceneSystem.scenes(),
							   camera.camera,
							   *framebuffer,
							   window().commandQueue());
		}
		else {
			OverlayDrawDescription desc;
			
			sceneRenderer.drawWithOverlays(sceneSystem.scenes(),
										   editor().selection(),
										   camera.camera,
										   desc,
										   *framebuffer,
										   *editorFramebuffer,
										   window().commandQueue());
		}
	}
	
	void Viewport::updateFramebuffer() {
		auto const framebufferTargetSize = size() * window().contentScaleFactor();
		if (!framebuffer || framebuffer->size != framebufferTargetSize) {
			framebuffer = sceneRenderer.renderer().createFramebuffer(framebufferTargetSize);
			
			if (auto* const editorRenderer = dynamic_cast<EditorRenderer*>(&sceneRenderer.renderer())) {
				editorFramebuffer = editorRenderer->createEditorFramebuffer(framebufferTargetSize);
			}
		}
	}
	
	void Viewport::onInput(bloom::InputEvent& event) {
		event.dispatch<bloom::InputEventType::leftMouseDown>([&](bloom::MouseEvent const& e) {
			if (!viewportHovered || gizmo.isHovered() || gameView) {
				return false;
			}
			
			auto const locationInView = windowSpaceToViewSpace(e.locationInWindow);
			
			if (auto const entity = overlays.hitTest(locationInView)) {
				selection().select(entity);
				return true;
			}
			
			if (auto const entity = readEntityID(locationInView)) {
				selection().select(entity);
				return true;
			}
			
			selection().clear();
			return true;
		});
		
		event.dispatch<bloom::InputEventType::keyDown>([&](bloom::KeyEvent const& e) {
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
				
				case Key::escape:
					restore();
					
				case Key::G:
					gameView ^= true;
					
				default:
					break;
			}
		});
	}
	
	void Viewport::debugPanel() {
		auto matrix = [](float4x4 const& m){
			withFont(Font::UIDefault().setMonospaced(true), [&]{
				for (int i = 0; i < 4; ++i) {
					ImGui::TextUnformatted(utl::format("{}", m.row(i)).data());
				}
			});
		};
		
		ImGui::Begin("Viewport Debug");
		
		float4x4 const view = camera.camera.view();
		float4x4 const proj = camera.camera.projection();
		
		withFont(Font::UIDefault().setWeight(FontWeight::semibold), [&]{
			ImGui::Text("View Matrix:");
		});
		matrix(view);
		withFont(Font::UIDefault().setWeight(FontWeight::semibold), [&]{
			ImGui::Text("Projection Matrix:");
		});
		matrix(proj);
		
		
		
		ImGui::End();
	}
	
	static std::string toString(Viewport::Parameters::FramebufferElements value) {
		return std::array{
			"Depth",
			"Raw",
			"Postprocessed"
		}[(std::size_t)value];
	}
	
	void Viewport::dropdownMenu() {
		using namespace propertiesView;
		ImGui::SetWindowSize({ ImGui::GetWindowSize().x, std::min(ImGui::GetWindowSize().y, 250.0f) });
		header("Viewport Settings");
		if (beginSection()) {
			beginProperty("Game View");
			ImGui::Checkbox("##game-view", &gameView);
			
			beginProperty("Field Of View");
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			ImGui::SliderFloat("##field-of-view", &camera.data.fieldOfView, 30, 180);
			
			beginProperty("Near Clip Plane");
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			ImGui::SliderFloat("##near-clip-plane", &camera.data.nearClip, 0, 1);
			
//			beginProperty("Visualize Shadow Cascades");
//			if (ImGui::Checkbox("##viz-shadow-cascades", &drawOptions.visualizeShadowCascades) &&
//				!drawOptions.lightVizEntity)
//			{
//				// search for first dir light entity
//				for (auto scene: scenes()) {
//					for (auto&& [entity, light]: scene->view<DirectionalLightComponent const>().each()) {
//						if (light.light.castsShadows) {
//							drawOptions.lightVizEntity = scene->getHandle(entity);
//							break;
//						}
//					}
//				}
//			}
			
//			if (drawOptions.visualizeShadowCascades) {
//				EntityID const lightEntity{ drawOptions.lightVizEntityID };
//
//				char const* preview = nullptr;
//				if (!lightEntity ||
//					getLightType(lightEntity) == LightType::none ||
//					!scene()->getComponent<DirectionalLightComponent>(lightEntity).light.castsShadows)
//				{
//					preview = "Select Shadow Caster";
//				}
//				else {
//					preview = scene()->getComponent<TagComponent>(lightEntity).name.data();
//				}
//
//				beginProperty("Shadow Caster");
//				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
//				if (ImGui::BeginCombo("##shadow-caster", preview, ImGuiComboFlags_NoArrowButton))
//				{
//					if (scene()) {
//						for (auto&& [entity, light, tag]: scene()->view<DirectionalLightComponent const, TagComponent const>().each()) {
//							if (light.light.castsShadows && ImGui::Selectable(tag.name.data())) {
//								drawOptions.lightVizEntityID = (uint32_t)entity;
//							}
//						}
//					}
//					ImGui::EndCombo();
//				}
//			}
			
			
			endSection();
		}
		
		header("Framebuffer");
		if (beginSection()) {
			beginProperty("Slot");
			if (ImGui::BeginCombo("##shadow-caster", toString(params.framebufferSlot).data(), ImGuiComboFlags_NoArrowButton)) {
				enumCombo(params.framebufferSlot);
				ImGui::EndCombo();
			}
			endSection();
		}
	}
	
	bloom::EntityHandle Viewport::readEntityID(mtl::float2 const mousePosition) {
		static_assert(std::is_same_v<std::underlying_type_t<entt::entity>, std::uint32_t>);
		
//		if (!scene()) {
//			return {};
//		}
//
//		if (mousePosition.x < 0 || mousePosition.y < 0) {
//			return {};
//		}
//		auto const viewSize = this->size();
//		if (mousePosition.x >= viewSize.x || mousePosition.y >= viewSize.y) {
//			return {};
//		}
#warning !!
//		auto* entityTexture = (MTL::Texture*)framebuffer.entityID.nativeHandle();
//		auto* mtlRenderContext = utl::down_cast<bloom::MetalRenderContext*>(renderer->getRenderContext());
//		auto* commandBuffer = mtlRenderContext->commandQueue()->commandBuffer();
//		auto* encoder = commandBuffer->blitCommandEncoder();
//		encoder->synchronizeTexture(entityTexture, 0, 0);
//		encoder->endEncoding();
//		commandBuffer->commit();
//		commandBuffer->waitUntilCompleted();
//
//		std::uint32_t id = -1;
//		auto const region = MTL::Region(mousePosition.x * 2, mousePosition.y * 2, 1, 1);
//		entityTexture->getBytes(&id, 4, region, 0);
//
//		return bloom::EntityID((entt::entity)id);
		return {};
	}
	
	void Viewport::recieveSceneDragDrop() {
		auto payload = acceptAssetDragDrop(AssetType::scene);
		if (payload) {
			AssetHandle const handle = *payload;
			auto scene = assetManager().get(handle);
			assetManager().makeAvailable(handle, AssetRepresentation::CPU);
			
			editor().coreSystems().sceneSystem().unloadAll();
			editor().coreSystems().sceneSystem().loadScene(as<Scene>(scene));
		}
	}
	
	mtl::float3 Viewport::worldSpaceToViewSpace(mtl::float3 const positionWS) {
		auto const viewProj = camera.camera.viewProjection();
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
		auto const viewProj = camera.camera.viewProjection();
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
