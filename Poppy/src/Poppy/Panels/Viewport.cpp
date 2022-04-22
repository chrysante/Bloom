#include "Viewport.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/ImGuizmo.h>
#include "Poppy/ImGui/ImGui.hpp"

#include <utl/stdio.hpp>

#include "Bloom/Scene/Components.hpp"
#include "Bloom/Graphics/RenderContext.hpp"
#include "Bloom/Application/Application.hpp"
#include "Bloom/Application/Event.hpp"
 
#include "Poppy/Editor.hpp"
#include "Poppy/SelectionContext.hpp"


#include "Bloom/Platform/Metal/MetalRenderContext.hpp"
#include <Metal/Metal.hpp>

using namespace mtl::short_types;

namespace poppy{
	
	std::string_view toString(GizmoMode mode) {
		return std::array {
			"Translate",
			"Rotate",
			"Scale"
		}[(std::size_t)mode];
	}
	
	std::string_view toString(CoordinateSpace mode) {
		return std::array {
			"World",
			"Local"
		}[(std::size_t)mode];
	}
	
	std::string_view toString(Projection proj) {
		return std::array {
			"Perspective",
			"Orthogonal"
		}[(std::size_t)proj];
	}
	
	Viewport::Viewport(SelectionContext* selection,
					   bloom::Scene* s,
					   bloom::Renderer* r):
		Panel("Viewport"),
		selection(selection),
		scene(s),
		renderer(r)
	{
		padding = 0;
		imguizmoContext = ImGuizmo::CreateContext();
	}
	
	Viewport::~Viewport() {
		ImGuizmo::DestroyContext((ImGuizmo::Context*)imguizmoContext);
	}

	void Viewport::onEvent(bloom::Event const& event) {
		event.dispatch<bloom::EventType::leftMouseDown>([&](bloom::MouseEvent const& e) {
			if (!viewportHovered || gizmoHovered) {
				return;
			}
			
			auto const locationInView = windowSpaceToViewSpace(e.locationInWindow);
			auto const entity = readEntityID(locationInView);
			if (entity) {
				selection->select(entity);
			}
			else {
				selection->clear();
			}
		});
		
		event.dispatch<bloom::EventType::keyDown>([&](bloom::KeyEvent const& e) {
			using bloom::Key;
			switch (e.key) {
				case Key::tab:
					if (gizmoSpace == CoordinateSpace::world) {
						gizmoSpace = CoordinateSpace::local;
					}
					else {
						gizmoSpace = CoordinateSpace::world;
					}
					break;
				case Key::_1:
					gizmoMode = GizmoMode::translate;
					break;
				case Key::_2:
					gizmoMode = GizmoMode::rotate;
					break;
				case Key::_3:
					gizmoMode = GizmoMode::scale;
					break;
				default:
					break;
			}
		});
	}
	
	void Viewport::init() {
		auto& settings = getEditor().settings();
		
		/* camera position */ {
			
			cameraActor.position[0] = settings.getFloat(utl::format("{}-Camera-Position-X", uniqueName())).value_or(0);
			cameraActor.position[1] = settings.getFloat(utl::format("{}-Camera-Position-Y", uniqueName())).value_or(-60);
			cameraActor.position[2] = settings.getFloat(utl::format("{}-Camera-Position-Z", uniqueName())).value_or(60);
		}
		
		/* camera angle */ {
			cameraActor.angleLR = settings.getFloat(utl::format("{}-Camera-Angle-Theta", uniqueName())).value_or(mtl::constants<>::pi / 2);
			cameraActor.angleUD = settings.getFloat(utl::format("{}-Camera-Angle-Phi",   uniqueName())).value_or(mtl::constants<>::pi / 2);
		}
		cameraActor.applyTransform();
	}
	
	void Viewport::shutdown() {
		auto& settings = getEditor().settings();
		
		/* camera position */ {
			settings.setFloat(utl::format("{}-Camera-Position-X", uniqueName()), cameraActor.position[0]);
			settings.setFloat(utl::format("{}-Camera-Position-Y", uniqueName()), cameraActor.position[1]);
			settings.setFloat(utl::format("{}-Camera-Position-Z", uniqueName()), cameraActor.position[2]);
		}
		
		/* camera angle */ {
			settings.setFloat(utl::format("{}-Camera-Angle-Theta", uniqueName()), cameraActor.angleLR);
			settings.setFloat(utl::format("{}-Camera-Angle-Phi",   uniqueName()), cameraActor.angleUD);
		}
	}
	
	void Viewport::display() {
		renderScene();
		ImGui::Image(frameBuffer.finalImageEditor().nativeHandle(), ImGui::GetWindowSize());
		
		drawOverlays();
		
		displayControls();
		
		ImGui::SetCursorPos({ 0, 0 });
		auto const wantsInput = detectViewportInput(ImGuiButtonFlags_MouseButtonRight);
		viewportHovered = wantsInput.hovered;
		if (wantsInput.held) {
			cameraActor.update(getApplication().getRenderTime(), getApplication().input());
		}
		
		
		
		if (!selection->empty()) {
			auto entity = selection->ids().front();
			auto& transform = scene->getComponent<bloom::TransformComponent>(entity);
			
			displayGizmo(transform);
		}
	}
	
	void Viewport::drawOverlays() {
		drawLightOverlays();
	}
	
	static std::uint32_t toUCol32(mtl::float3 color) {
		return ImGui::ColorConvertFloat4ToU32(mtl::float4{ color, 1 });
	}
	
	void Viewport::drawLightOverlays() {
		using namespace bloom;
		auto* const drawList = ImGui::GetWindowDrawList();
		auto view = scene->view<TransformComponent const, LightComponent const>();
		view.each([&](TransformComponent const& transform, LightComponent const& light) {
			auto const positionVS = worldSpaceToViewSpace(transform.position);
			if (positionVS.z < 0 || positionVS.z > 1) {
				return;
			}
			auto const positionInWindow = viewSpaceToWindowSpace(positionVS.xy);
			switch (light.type()) {
				case LightType::pointlight: {
					auto const p = light.get<PointLight>();
					drawList->AddCircle(positionInWindow, 20, toUCol32(p.color));
					break;
				}
					
				case LightType::spotlight: {
					auto  const s = light.get<SpotLight>();
					float const radius = 25;
					float2 const v0 = float2(0, 1) * radius + positionInWindow;
					float const angle1 = mtl::constants<>::pi * (0.5 + 2. / 3.);
					float2 const v1 = float2(std::cos(angle1), std::sin(angle1)) * radius + positionInWindow;
					float const angle2 = mtl::constants<>::pi * (0.5 + 4. / 3.);
					float2 const v2 = float2(std::cos(angle2), std::sin(angle2)) * radius + positionInWindow;
					
					drawList->AddTriangle(v0, v1, v2, toUCol32(s.color));
					break;
				}
					
				default:
					break;
			}
		});
		
	}
	
	template <typename E>
	static void inputControlCombo(char const* ID, E& e,
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
		
		std::string_view const current = toString(e);
		float const width = ImGui::CalcTextSize(current.data()).x + GImGui->Style.FramePadding.x * 2;
		ImGui::SetNextItemWidth(width);
		
		
		if (ImGui::BeginCombo(ID, toString(e).data(), comboFlags)) {
			ImGui::PushFont(defaultFont);
			for (auto i: utl::enumerate<E>(count)) {
				bool const selected = e == i;
				if (ImGui::Selectable(toString(i).data(), selected)) {
					e = i;
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
	}
	
	void Viewport::displayControls() {
		ImGui::SetCursorPos({ 5, 5 });
		
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 5, 5 });
		if (ImGui::BeginCombo("##displayControlMenu", nullptr, ImGuiComboFlags_NoPreview)) {
			ImGui::Button("Button");
			ImGui::SetNextItemWidth(100);
			ImGui::SliderFloat("Field of View", &fieldOfView, 30, 180);
			ImGui::EndCombo();
		}
		ImGui::PopStyleVar();
		
		ImGui::SameLine();
		inputControlCombo("##DrawMode", drawMode);
		ImGui::SameLine();
		inputControlCombo("##EditMode", gizmoMode, 3);
		ImGui::SameLine();
		inputControlCombo("##EditSpace", gizmoSpace, 2, /* disable =  */ gizmoMode == GizmoMode::scale);
		ImGui::SameLine();
		inputControlCombo("##Projection", cameraProjection, 2);
	}
	
	void Viewport::displayGizmo(bloom::TransformComponent& transform) {
		ImGuizmo::SetContext((ImGuizmo::Context*)imguizmoContext);
		ImGuizmo::BeginFrame();
		ImGuizmo::SetOrthographic(false);
		ImGuizmo::SetDrawlist();
		float2 const pos = ImGui::GetWindowPos();
		float2 const size = ImGui::GetWindowSize();
		ImGuizmo::SetRect(pos.x, pos.y, size.x, size.y);
		
		auto const view = mtl::transpose(cameraActor.camera.getView());
		auto const proj = mtl::transpose(cameraActor.camera.getProjection());
		auto matrix = mtl::transpose(transform.calculate());
		
		auto const operation = [&]{
			switch (gizmoMode) {
				case GizmoMode::translate:
					return ImGuizmo::OPERATION::TRANSLATE;
				case GizmoMode::rotate:
					return ImGuizmo::OPERATION::ROTATE;
				case GizmoMode::scale:
					return ImGuizmo::OPERATION::SCALE;
			}
		}();
		
		auto const space = [&]{
			switch (gizmoSpace) {
				case CoordinateSpace::world:
					return gizmoMode == GizmoMode::scale ?
						ImGuizmo::MODE::LOCAL :
						ImGuizmo::MODE::WORLD;
					
				case CoordinateSpace::local:
					return ImGuizmo::MODE::LOCAL;
			}
		}();
		
		if (ImGuizmo::Manipulate(view.data(), proj.data(), operation, space, matrix.data())) {
			mtl::float3 translation, rotation, scale;
			ImGuizmo::DecomposeMatrixToComponents(matrix.data(),
												  translation.data(),
												  rotation.data(),
												  scale.data());
			transform.position = translation;
			transform.orientation = mtl::to_quaternion(rotation.x, rotation.y, rotation.z);
			transform.scale = scale;
		}
		gizmoHovered = ImGuizmo::IsOver();
	}
	
	void Viewport::renderScene() {
		using namespace bloom;
		updateRenderTarget(ImGui::GetWindowSize());
		
		auto& camera = cameraActor.camera;
		if (cameraProjection == Projection::perspective) {
			camera.setProjection(mtl::to_radians(fieldOfView), frameBuffer.size());
		}
		else {
			float const width = 1000; // ??
			float const aspect = frameBuffer.size().y / float(frameBuffer.size().x);
			float height = width * aspect;
			camera.setProjectionOrtho(-width / 2, width / 2,
									  -height / 2, height / 2,
									  0, 5000);
		}
		
		
		renderer->beginScene(camera);
		/* submit meshes */ {
			auto view = scene->view<TransformComponent const, MeshRenderComponent const>();
			view.each([&](auto id, TransformComponent const& transform, MeshRenderComponent const& meshRenderer) {
				bloom::EntityRenderData entityData;
				entityData.transform = transform.calculate();
				entityData.ID = utl::to_underlying(id);
				renderer->submit(meshRenderer.mesh.get(),
								 meshRenderer.material.get(),
								 entityData,
								 selection->isSelected(bloom::EntityID(id)));
			});
		}
		/* submit lights */ {
			auto view = scene->view<TransformComponent const, LightComponent const>();
			view.each([&](auto id, TransformComponent const& transform, LightComponent const& light) {
				switch (light.type()) {
					case LightType::pointlight:
						renderer->submit(light.get<PointLight>(), transform.position);
						break;
					case LightType::spotlight:
						renderer->submit(light.get<SpotLight>(), transform.position, mtl::rotate({ 1, 0, 0 }, transform.orientation));
						break;
					default:
						break;
				}
			});
		}
		renderer->endScene();
		
		renderer->debugDraw(&frameBuffer, drawMode);
	}
	
	void Viewport::updateRenderTarget(mtl::usize2 size) {
		size *= 2; // TODO: Fix global DPI Scale
		if (frameBuffer.size() != size) {
			auto* const renderContext = renderer->getRenderContext();
			frameBuffer = bloom::EditorFrameBuffer::create(renderContext, size.x, size.y);
		}
	}
	
	bloom::EntityID Viewport::readEntityID(mtl::float2 const mousePosition) {
		static_assert(std::is_same_v<std::underlying_type_t<entt::entity>, std::uint32_t>);
		
		if (mousePosition.x < 0 || mousePosition.y < 0) {
			return {};
		}
		auto const panelSize = this->size();
		if (mousePosition.x >= panelSize.x || mousePosition.y >= panelSize.y) {
			return {};
		}
		
		auto* entityTexture = (MTL::Texture*)frameBuffer.entityID().nativeHandle();
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
	
	mtl::float3 Viewport::worldSpaceToViewSpace(mtl::float3 const positionWS) {
		auto const viewProj = cameraActor.camera.getMatrix();
		auto const tmp = viewProj * mtl::float4(positionWS, 1);
		auto const ndc = tmp.xyz / tmp.w;
		
		auto posVS = ndc.xy;
		
		posVS = (posVS + 1) / 2;
		posVS.y = 1 - posVS.y;
		posVS *= this->size();
		return { posVS, ndc.z };
	}
	
	void ViewportCameraActor::update(bloom::TimeStep time, bloom::Input const& input) {
		using namespace bloom;
		
		angleLR  = utl::mod(angleLR - input.mouseOffset().x / 180., mtl::constants<>::pi * 2);
		angleUD  = std::clamp(angleUD + input.mouseOffset().y / 180.f, 0.01f, mtl::constants<float>::pi - 0.01f);
		
		float offset = speed * time.delta;
		if (input.keyDown(bloom::Key::leftShift)) {
			offset *= 3;
		}
		
		if (input.keyDown(Key::A)) {
			position -= mtl::cross(front(), up()) * offset;
		}
		if (input.keyDown(Key::D)) {
			position += mtl::cross(front(), up()) * offset;
		}
		if (input.keyDown(Key::W)) {
			position += front() * offset;
		}
		if (input.keyDown(Key::S)) {
			position -= front() * offset;
		}
		if (input.keyDown(Key::Q)) {
			position -= up() * offset;
		}
		if (input.keyDown(Key::E)) {
			position += up() * offset;
		}
		
		applyTransform();
	}
	
	void ViewportCameraActor::applyTransform() {
		camera.setTransform(position, front());
	}
	
	mtl::float3 ViewportCameraActor::front() const {
		using std::sin;
		using std::cos;
		return {
			sin(angleUD) * cos(angleLR),
			sin(angleUD) * sin(angleLR),
			cos(angleUD)
		};
	}
	
}
