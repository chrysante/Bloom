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
	
	Viewport::Viewport(bloom::Renderer* r):
		Panel("Viewport"),
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
		params = settings["Parameters"].as<Parameters>(Parameters{});
		cameraActor = settings["Camera"].as<ViewportCameraActor>(ViewportCameraActor{});
		cameraActor.applyTransform();
	}
	
	void Viewport::shutdown() {
		settings["Parameters"] = params;
		settings["Camera"] = cameraActor;
	}
	
	void Viewport::display() {
		if (!scene()) {
			return;
		}
		
		calculateTransforms();
		renderScene();
		ImGui::Image(frameBuffer.finalImageEditor().nativeHandle(), ImGui::GetWindowSize());
		
		drawOverlays();
		
		if (!selection()->empty()) {
			auto entity = selection()->ids().front();
			displayGizmo(entity);
		}
		
		displayControls();
		
		auto const wantsInput = detectViewportInput(ImGuiButtonFlags_MouseButtonRight);
		viewportHovered = wantsInput.hovered;
		if (wantsInput.held) {
			cameraActor.update(getApplication().getRenderTime(), getApplication().input());
		}
	}
	
	void Viewport::drawOverlays() {
		drawLightOverlays();
	}
	
	static std::uint32_t toUCol32(mtl::float4 color) {
		return ImGui::ColorConvertFloat4ToU32(color);
	}
	
	static std::uint32_t toUCol32(mtl::float3 color) {
		return toUCol32(mtl::float4{ color, 1 });
	}
	
	void Viewport::drawLightOverlays() {
		using namespace bloom;
		
		auto view = scene()->view<TransformComponent const, LightComponent const>();
		view.each([&](auto const entity, TransformComponent const& transform, LightComponent const& light) {
			auto const positionVS = worldSpaceToViewSpace(scene()->calculateTransformRelativeToWorld(entity).column(3).xyz);
			if (positionVS.z < 0 || positionVS.z > 1) {
				return;
			}
			auto const positionInWindow = viewSpaceToWindowSpace(positionVS.xy);
			switch (light.type()) {
				case LightType::pointlight: {
					drawPointLightIcon(positionInWindow, light.get<PointLight>().common.color);
					break;
				}
					
				case LightType::spotlight: {
					auto  const s = light.get<SpotLight>();
					drawSpotLightIcon(positionInWindow, s.common.color);
					if (selection()->isSelected(entity)) {
						drawSpotlightVizWS(entity,
										   s.radius, (s.innerCutoff + s.outerCutoff) / 2,
										   (s.common.color + mtl::colors<float3>::white) / 2);
					}
					
					break;
				}
					
				default:
					break;
			}
		});
		
	}
	
	void Viewport::drawPointLightIcon(mtl::float2 position, mtl::float3 color) {
		float const size = 20;
		auto* const drawList = ImGui::GetWindowDrawList();
		drawList->AddCircle(position, size, toUCol32({ 0, 0, 0, 0.5 }), 0, 4);
		drawList->AddCircle(position, size, toUCol32(color), 0, 2);
		
	}
	
	void Viewport::drawSpotLightIcon(mtl::float2 position, mtl::float3 color) {
		float const radius = 25;
		float2 const v0 = float2(0, 1) * radius + position;
		float const angle1 = mtl::constants<>::pi * (0.5 + 2. / 3.);
		float2 const v1 = float2(std::cos(angle1), std::sin(angle1)) * radius + position;
		float const angle2 = mtl::constants<>::pi * (0.5 + 4. / 3.);
		float2 const v2 = float2(std::cos(angle2), std::sin(angle2)) * radius + position;

		auto* const drawList = ImGui::GetWindowDrawList();
		drawList->AddTriangle(v0, v1, v2, toUCol32(color));
		
	}
	
	
	void Viewport::drawSpotlightVizWS(bloom::EntityID entity,
									  float radius,
									  float angle,
									  float3 color)
	{
		auto* const drawList = ImGui::GetWindowDrawList();
		int const segments = 32;
		utl::small_vector<float3, 32> circlePoints1;
		for (int i = 0; i < segments; ++i) {
			float const tau = mtl::constants<>::pi * 2;
			float const angle = i * tau / segments;
			circlePoints1.push_back({ 0, cos(angle), sin(angle) });
		}
		float const theta = angle;
		float const r1 = radius;
		float const r2 = radius + 50;
		float const rho1 = r1 * sin(theta);
		float const rho2 = r2 * sin(theta);
		float const sigma1 = r1 * cos(theta);
		float const sigma2 = r2 * cos(theta);
		
		float4x4 const transform = scene()->calculateTransformRelativeToWorld(entity);
		
		auto circlePoints2 = circlePoints1;
		for (auto& p: circlePoints1) {
			p.x = sigma1;
			p.y *= rho1;
			p.z *= rho1;
			p = (transform * float4(p, 1)).xyz;
		}
		for (auto& p: circlePoints2) {
			p.x = sigma2;
			p.y *= rho2;
			p.z *= rho2;
			p = (transform * float4(p, 1)).xyz;
		}
		
		utl::small_vector<float2, 32> pointsWindowSpace1;
		std::transform(circlePoints1.begin(), circlePoints1.end(),
					   std::back_inserter(pointsWindowSpace1),
					   [this](float3 x) { return worldSpaceToWindowSpace(x).xy; });
		utl::small_vector<float2, 32> pointsWindowSpace2;
		std::transform(circlePoints2.begin(), circlePoints2.end(),
					   std::back_inserter(pointsWindowSpace2),
					   [this](float3 x) { return worldSpaceToWindowSpace(x).xy; });
		
		for (int i = 0; i < segments; i += 4) {
			drawList->AddLine(pointsWindowSpace1[i], pointsWindowSpace2[i], toUCol32(color));
		}
		for (int i = 0; i < segments; ++i) {
			drawList->AddLine(pointsWindowSpace1[i], pointsWindowSpace1[(i + 1) % segments], toUCol32(color));
			drawList->AddLine(pointsWindowSpace2[i], pointsWindowSpace2[(i + 1) % segments], toUCol32(color));
		}
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
			ImGui::SliderFloat("Field of View", &params.fieldOfView, 30, 180);
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
	
	static std::pair<bool, mtl::float4x4> manipulateGizmo(ImGuizmo::Context* context,
														  mtl::float4x4 view,
														  mtl::float4x4 proj,
														  GizmoMode operation,
														  CoordinateSpace space,
														  mtl::float4x4 transform)
	{
		ImGuizmo::SetContext(context);
		ImGuizmo::BeginFrame();
		ImGuizmo::SetOrthographic(false);
		ImGuizmo::SetDrawlist();
		float2 const pos = ImGui::GetWindowPos();
		float2 const size = ImGui::GetWindowSize();
		ImGuizmo::SetRect(pos.x, pos.y, size.x, size.y);
		
		
		view = mtl::transpose(view);
		proj = mtl::transpose(proj);
		transform = mtl::transpose(transform);
		
		if (ImGuizmo::Manipulate(view.data(), proj.data(),
								 (ImGuizmo::OPERATION)operation,
								 (ImGuizmo::MODE)(1 - (int)space),
								 transform.data()))
		{
			return { true, mtl::transpose(transform) };
		}
		return { false, 0 };
	}
	
	void Viewport::displayGizmo(bloom::EntityID entity) {
		using namespace bloom;
		
		bloom::TransformComponent& transformComponent = scene()->getComponent<TransformComponent>(entity);
		EntityID const parent = scene()->getComponent<HierarchyComponent>(entity).parent;
		
		auto const view = cameraActor.camera.getView();
		auto const proj = cameraActor.camera.getProjection();
		auto const localTransform = transformComponent.calculate();
		auto const parentTransform = scene()->calculateTransformRelativeToWorld(parent);
		
		auto const entityWSTransform = parentTransform * localTransform;
		
		auto const [manipulated, newEntityWSTransform] = manipulateGizmo((ImGuizmo::Context*)imguizmoContext,
																		 view,
																		 proj,
																		 gizmoMode,
																		 gizmoSpace,
																		 entityWSTransform);
		
		gizmoHovered = ImGuizmo::IsOver();
		
		if (manipulated) {
			auto const newLocalTransform = mtl::inverse(parentTransform) * newEntityWSTransform;
			
			std::tie(transformComponent.position,
					 transformComponent.orientation,
					 transformComponent.scale) = mtl::decompose_transform(newLocalTransform);
		}
	}
	
	void Viewport::calculateTransforms() {
		using namespace bloom;
		auto view = scene()->view<TransformComponent const, TransformMatrixComponent>();
		view.each([&](auto const id, TransformComponent const& transform, TransformMatrixComponent& transformMatrix) {
			transformMatrix.matrix = transform.calculate();
		});
		
		std::stack<EntityID, utl::small_vector<EntityID>> stack(scene()->gatherRoots());
		
		while (!stack.empty()) {
			auto const current = stack.top();
			auto const& currentTransform = scene()->getComponent<TransformMatrixComponent>(current);
			stack.pop();
			
			auto const children = scene()->gatherChildren(current);
			for (auto const c: children) {
				auto& childTransform = scene()->getComponent<TransformMatrixComponent>(c);
				childTransform.matrix = currentTransform.matrix * childTransform.matrix;
				stack.push(c);
			}
		}
	}
	
	void Viewport::renderScene() {
		using namespace bloom;
		updateRenderTarget(ImGui::GetWindowSize());
		
		auto& camera = cameraActor.camera;
		if (cameraProjection == Projection::perspective) {
			camera.setProjection(mtl::to_radians(params.fieldOfView), frameBuffer.size());
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
			auto view = scene()->view<TransformMatrixComponent const, MeshRenderComponent const>();
			view.each([&](auto const id, TransformMatrixComponent const& transform, MeshRenderComponent const& meshRenderer) {
				if (!meshRenderer.mesh || !meshRenderer.material) {
					return;
				}
				bloom::EntityRenderData entityData;
				entityData.transform = transform.matrix;
				entityData.ID = utl::to_underlying(id);
				renderer->submit(meshRenderer.mesh.get(),
								 meshRenderer.material.get(),
								 entityData,
								 selection()->isSelected(bloom::EntityID(id)));
			});
		}
		
		/* submit lights */ {
			auto view = scene()->view<TransformMatrixComponent const, LightComponent const>();
			view.each([&](auto id, TransformMatrixComponent const& transform, LightComponent const& light) {
				switch (light.type()) {
					case LightType::pointlight:
						renderer->submit(light.get<PointLight>(),
										 transform.matrix.column(3).xyz);
						break;
					case LightType::spotlight:
						renderer->submit(light.get<SpotLight>(),
										 transform.matrix.column(3).xyz,
										 mtl::normalize((transform.matrix * mtl::float4{ 1, 0, 0, 0 }).xyz));
						break;
					case LightType::directional:
						renderer->submit(light.get<DirectionalLight>(),
										 mtl::normalize((transform.matrix * mtl::float4{ 1, 0, 0, 0 }).xyz));
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
	
	mtl::float3 Viewport::worldSpaceToWindowSpace(mtl::float3 position) {
		auto const vs = worldSpaceToViewSpace(position);
		return { viewSpaceToWindowSpace(vs.xy), vs.z };
	}
	
	
	
}
