// #include "RendererSettingsView.hpp"
//
// #include "Poppy/Editor/Editor.hpp"
//
// #include <imgui.h>
//
// using namespace bloom;
//
// namespace poppy {
//
//	POPPY_REGISTER_VIEW(RendererSettingsView, "Renderer Settings");
//
//	RendererSettingsView::RendererSettingsView() {
//
//	}
//
//
//	void RendererSettingsView::frame() {
//		shadowProperties();
//	}
//
//	void RendererSettingsView::shadowProperties() {
//		Renderer* renderer = &editor().coreSystems().renderer();
//
//		if (!renderer) {
//			displayEmptyWithReason("No Renderer available");
//			return;
//		}
//
//		header("Shadow Properties");
//		if (beginSection()) {
//			beginProperty("Resolution");
//			mtl::uint2 resolution =
// renderer->getShadowMapResolution();
//
//			if (ImGui::BeginCombo("##-shadow-resolution",
// utl::format("{}", resolution.x).data())) { 				for (int i = 6; i <=
// 12;
// ++i) { 					mtl::uint2 const value = 1u << i;
// bool const selected = resolution == value; 					if
// (ImGui::Selectable(utl::format("{}", value.x).data(), selected)) {
// renderer->setShadowMapResolution(value);
//					}
//					if (selected) {
//						ImGui::SetItemDefaultFocus();
//					}
//				}
//				ImGui::EndCombo();
//			}
//
//			beginProperty("Cull Front Faces");
//			auto const cullMode = renderer->getShadowCullMode();
//			bool frontFaces = cullMode ==
// bloom::TriangleCullMode::front; 			if
// (ImGui::Checkbox("##-shadow-cull-mode", &frontFaces)) {
// renderer->setShadowCullMode(frontFaces ? bloom::TriangleCullMode::front :
// bloom::TriangleCullMode::back);
//			}
//
//			endSection();
//		}
//
//	}
//
// }
