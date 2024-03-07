
set(bloom_sources
  src/Bloom/Application.hpp
  src/Bloom/Application/Application.cpp
  src/Bloom/Application/Application.hpp
  src/Bloom/Application/CoreSystem.hpp
  src/Bloom/Application/CoreSystemManager.cpp
  src/Bloom/Application/CoreSystemManager.hpp
  src/Bloom/Application/Input.cpp
  src/Bloom/Application/Input.hpp
  src/Bloom/Application/InputEvent.cpp
  src/Bloom/Application/InputEvent.hpp
  src/Bloom/Application/MessageSystem.hpp
  src/Bloom/Application/ResourceUtil.cpp
  src/Bloom/Application/ResourceUtil.hpp
  src/Bloom/Application/Window.cpp
  src/Bloom/Application/Window.hpp
  src/Bloom/Application/WindowDelegate.cpp
  src/Bloom/Application/WindowDelegate.hpp

  src/Bloom/Asset/Asset.cpp
  src/Bloom/Asset/Asset.hpp
  src/Bloom/Asset/AssetFileHeader.hpp
  src/Bloom/Asset/AssetManager.cpp
  src/Bloom/Asset/AssetManager.hpp
  src/Bloom/Asset/MeshImporter.cpp
  src/Bloom/Asset/MeshImporter.hpp

  src/Bloom/Core.hpp
  src/Bloom/Core/Autorelease.hpp
  src/Bloom/Core/Base.hpp
  src/Bloom/Core/Core.hpp
  src/Bloom/Core/Debug.cpp
  src/Bloom/Core/Debug.hpp
  src/Bloom/Core/EnumCount.hpp
  src/Bloom/Core/Profile.cpp
  src/Bloom/Core/Profile.hpp
  src/Bloom/Core/Reference.hpp
  src/Bloom/Core/Serialize.cpp
  src/Bloom/Core/Serialize.hpp
  src/Bloom/Core/Time.cpp
  src/Bloom/Core/Time.hpp
  src/Bloom/Core/Yaml.hpp

  src/Bloom/GPU.hpp
  src/Bloom/GPU/BlitContext.hpp
  src/Bloom/GPU/CommandQueue.hpp
  src/Bloom/GPU/ComputeContext.hpp
  src/Bloom/GPU/HardwareDevice.cpp
  src/Bloom/GPU/HardwareDevice.hpp
  src/Bloom/GPU/HardwarePrimitives.hpp
  src/Bloom/GPU/HardwareResourceHandle.hpp
  src/Bloom/GPU/RenderContext.hpp
  src/Bloom/GPU/Swapchain.hpp

  src/Bloom/Graphics/Material/Material.cpp
  src/Bloom/Graphics/Material/Material.hpp
  src/Bloom/Graphics/Material/MaterialInstance.cpp
  src/Bloom/Graphics/Material/MaterialInstance.hpp
  src/Bloom/Graphics/Material/MaterialParameters.hpp

  src/Bloom/Graphics/Renderer/BloomFramebuffer.hpp
  src/Bloom/Graphics/Renderer/BloomRenderer.cpp
  src/Bloom/Graphics/Renderer/BloomRenderer.hpp
  src/Bloom/Graphics/Renderer/ForwardRenderer.cpp
  src/Bloom/Graphics/Renderer/ForwardRenderer.hpp
  src/Bloom/Graphics/Renderer/Renderer.cpp
  src/Bloom/Graphics/Renderer/Renderer.hpp
  src/Bloom/Graphics/Renderer/RendererSanitizer.hpp
  src/Bloom/Graphics/Renderer/SceneRenderer.cpp
  src/Bloom/Graphics/Renderer/SceneRenderer.hpp
  src/Bloom/Graphics/Renderer/ShaderParameters.hpp

  src/Bloom/Graphics/Camera.cpp
  src/Bloom/Graphics/Camera.hpp
  src/Bloom/Graphics/Lights.cpp
  src/Bloom/Graphics/Lights.hpp
  src/Bloom/Graphics/Material
  src/Bloom/Graphics/MaterialProperties.hpp
  src/Bloom/Graphics/Renderer
  src/Bloom/Graphics/Renderer.hpp
  src/Bloom/Graphics/ShaderBase.hpp
  src/Bloom/Graphics/StaticMesh.cpp
  src/Bloom/Graphics/StaticMesh.hpp
  src/Bloom/Graphics/Vertex.hpp

  src/Bloom/Platform/MacOS/Autorelease.mm
  src/Bloom/Platform/MacOS/CocoaWindow.mm
  #src/Bloom/Platform/MacOS/Resource.mm
  src/Bloom/Platform/MacOS/ResourceUtil.mm

  src/Bloom/Platform/Metal/MetalBlitContext.h
  src/Bloom/Platform/Metal/MetalBlitContext.mm
  src/Bloom/Platform/Metal/MetalCommandQueue.h
  src/Bloom/Platform/Metal/MetalCommandQueue.mm
  src/Bloom/Platform/Metal/MetalComputeContext.h
  src/Bloom/Platform/Metal/MetalComputeContext.mm
  src/Bloom/Platform/Metal/MetalDevice.h
  src/Bloom/Platform/Metal/MetalDevice.mm
  src/Bloom/Platform/Metal/MetalRenderContext.h
  src/Bloom/Platform/Metal/MetalRenderContext.mm
  src/Bloom/Platform/Metal/MetalSwapchain.h
  src/Bloom/Platform/Metal/MetalSwapchain.mm

  src/Bloom/Runtime/CoreRuntime.cpp
  src/Bloom/Runtime/CoreRuntime.hpp
  src/Bloom/Runtime/SceneSystem.cpp
  src/Bloom/Runtime/SceneSystem.hpp
  src/Bloom/Runtime/ScriptSystem.cpp
  src/Bloom/Runtime/ScriptSystem.hpp

  src/Bloom/Scene/Components/AllComponents.hpp
  src/Bloom/Scene/Components/ComponentBase.hpp
  src/Bloom/Scene/Components/Hierarchy.hpp
  src/Bloom/Scene/Components/Lights.hpp
  src/Bloom/Scene/Components/MeshRenderer.cpp
  src/Bloom/Scene/Components/MeshRenderer.hpp
  src/Bloom/Scene/Components/Script.cpp
  src/Bloom/Scene/Components/Script.hpp
  src/Bloom/Scene/Components/Tag.hpp
  src/Bloom/Scene/Components/Transform.hpp

  src/Bloom/Scene/Entity.cpp
  src/Bloom/Scene/Entity.hpp
  src/Bloom/Scene/Scene.cpp
  src/Bloom/Scene/Scene.hpp
  src/Bloom/Scene/SceneCamera.cpp
  src/Bloom/Scene/SceneCamera.hpp
)

set(bloom_test_sources
  test/RuntimeSystem.t.cpp
)
