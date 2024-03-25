cmake_minimum_required(VERSION 3.23)

project(
  Bloom
  VERSION 0.0.1
  DESCRIPTION "Toy Game Engine"
)

include(cmake/BuildOptions.cmake)
include(cmake/CPM.cmake)
include(cmake/CompilerOptions.cmake)

set(CMAKE_CXX_STANDARD 20)

# Dependencies

# To make range-v3 happy
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
CPMAddPackage("gh:ericniebler/range-v3#0.12.0")
set(ASSIMP_INSTALL OFF)
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_WARNINGS_AS_ERRORS OFF)
CPMAddPackage("gh:assimp/assimp@5.3.1")
CPMAddPackage("gh:skypjack/entt@3.13.1")
set(RAPIDJSON_BUILD_DOC OFF)
set(RAPIDJSON_BUILD_EXAMPLES OFF)
set(RAPIDJSON_BUILD_TESTS OFF)
CPMAddPackage("gh:Tencent/rapidjson#master")
set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)
CPMAddPackage("gh:glfw/glfw#master")
set(IMGUIZMO_IMGUI_INCLUDEDIR ${PROJECT_SOURCE_DIR}/imgui)
CPMAddPackage("gh:chrysante/imguizmo#main")
set(SCATHA_BUILD_EXECUTABLES OFF)
set(SCATHA_ENABLE_INSTALL OFF)
CPMAddPackage("gh:chrysante/scatha#dev")
CPMAddPackage("gh:chrysante/utility#main")
CPMAddPackage("gh:chrysante/vml#main")
set(YAML_CPP_BUILD_CONTRIB OFF)
set(YAML_CPP_BUILD_TOOLS OFF)
set(YAML_BUILD_SHARED_LIBS ON)
set(YAML_CPP_BUILD_TESTS OFF)
CPMAddPackage("gh:jbeder/yaml-cpp#master")
set(termfmt_SHARED ON)
CPMAddPackage("gh:chrysante/termfmt#main")
if(BLOOM_BUILD_TESTS)
  CPMAddPackage("gh:catchorg/Catch2@3.4.0")
endif()

set(IMGUI_BACKEND_METAL ON)
set(IMGUI_BACKEND_OSX ON)
add_subdirectory(imgui)

# Subprojects
add_subdirectory(bloom)
add_subdirectory(poppy)
