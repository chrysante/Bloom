
set(poppy_sources
  src/Poppy/Core/Debug.h
  src/Poppy/Core/Common.h
  src/Poppy/Core/Profile.cpp
  src/Poppy/Core/Profile.h

  src/Poppy/Editor/Editor.cpp
  src/Poppy/Editor/Editor.h
  src/Poppy/Editor/EditorWindowDelegate.cpp
  src/Poppy/Editor/EditorWindowDelegate.h
  src/Poppy/Editor/SelectionContext.cpp
  src/Poppy/Editor/SelectionContext.h
  src/Poppy/Editor/Views/AssetBrowser.cpp
  src/Poppy/Editor/Views/AssetBrowser.h
  src/Poppy/Editor/Views/BasicSceneInspector.cpp
  src/Poppy/Editor/Views/BasicSceneInspector.h
  src/Poppy/Editor/Views/DebugViews.cpp
  src/Poppy/Editor/Views/DebugViews.h
  src/Poppy/Editor/Views/DirectoryView.cpp
  src/Poppy/Editor/Views/DirectoryView.h
  src/Poppy/Editor/Views/EntityInspector.cpp
  src/Poppy/Editor/Views/EntityInspector.h
  src/Poppy/Editor/Views/MaterialInstanceViewer.cpp
  src/Poppy/Editor/Views/MaterialInstanceViewer.h
  src/Poppy/Editor/Views/RendererDebuggerView.cpp
  src/Poppy/Editor/Views/RendererDebuggerView.h
  src/Poppy/Editor/Views/RendererSettingsView.cpp
  src/Poppy/Editor/Views/RendererSettingsView.h
  src/Poppy/Editor/Views/SceneOutliner.cpp
  src/Poppy/Editor/Views/SceneOutliner.h
  src/Poppy/Editor/Views/Viewport/Gizmo.cpp
  src/Poppy/Editor/Views/Viewport/Gizmo.h
  src/Poppy/Editor/Views/Viewport/Viewport.cpp
  src/Poppy/Editor/Views/Viewport/Viewport.h
  src/Poppy/Editor/Views/Viewport/ViewportCameraController.cpp
  src/Poppy/Editor/Views/Viewport/ViewportCameraController.h
  src/Poppy/Editor/Views/Viewport/ViewportOverlays.cpp
  src/Poppy/Editor/Views/Viewport/ViewportOverlays.h

  src/Poppy/Platform/MacOS/ImGuiContext.mm
  src/Poppy/Platform/MacOS/SystemAppearance.mm

  src/Poppy/Renderer/EditorDrawData.h
  src/Poppy/Renderer/EditorRenderer.cpp
  src/Poppy/Renderer/EditorRenderer.h
  src/Poppy/Renderer/EditorSceneRenderer.cpp
  src/Poppy/Renderer/EditorSceneRenderer.h

  src/Poppy/UI/Appearance.cpp
  src/Poppy/UI/Appearance.h
  src/Poppy/UI/Dockspace.cpp
  src/Poppy/UI/Dockspace.h
  src/Poppy/UI/Font.cpp
  src/Poppy/UI/Font.h
  src/Poppy/UI/Icons.cpp
  src/Poppy/UI/Icons.h
  src/Poppy/UI/ImGuiContext.cpp
  src/Poppy/UI/ImGuiContext.h
  src/Poppy/UI/ImGuiHelpers.cpp
  src/Poppy/UI/ImGuiHelpers.h
  src/Poppy/UI/PropertiesView.cpp
  src/Poppy/UI/PropertiesView.h
  src/Poppy/UI/SystemAppearance.h
  src/Poppy/UI/Toolbar.cpp
  src/Poppy/UI/Toolbar.h
  src/Poppy/UI/View.cpp
  src/Poppy/UI/View.h
)

set(poppy_shader_sources
  src/Poppy/Shaders/BloomShader.h
  src/Poppy/Shaders/BloomShader.metal
  src/Poppy/Shaders/Lights.h
  src/Poppy/Shaders/Lights.metal
  src/Poppy/Shaders/OutlinePass.metal
  src/Poppy/Shaders/PBRShader.h
  src/Poppy/Shaders/PBRShader.metal
  src/Poppy/Shaders/PCSSTest.metal
  src/Poppy/Shaders/PostProcess.h
  src/Poppy/Shaders/PostProcess.metal
  src/Poppy/Shaders/ShadowPassShader.metal
  src/Poppy/Shaders/editorPasses.metal
  src/Poppy/Shaders/mainPass.metal
)