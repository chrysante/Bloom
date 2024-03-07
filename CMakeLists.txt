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

set(ASSIMP_INSTALL OFF)
set( ASSIMP_WARNINGS_AS_ERRORS OFF)
CPMAddPackage("gh:assimp/assimp@5.3.1")
CPMAddPackage("gh:skypjack/entt@3.13.1")
cmake_policy(SET CMP0048 NEW)
CPMAddPackage("gh:Tencent/rapidjson#master")
set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)
CPMAddPackage("gh:glfw/glfw#master")
set(SCATHA_BUILD_EXECUTABLES OFF)
set(SCATHA_ENABLE_INSTALL OFF)
CPMAddPackage("gh:chrysante/scatha#dev")
CPMAddPackage("gh:chrysante/utility#main")
CPMAddPackage("gh:jbeder/yaml-cpp#master")
if(BLOOM_BUILD_TESTS)
  CPMAddPackage("gh:catchorg/Catch2@3.4.0")
endif()

# Subprojects
add_subdirectory(bloom)
add_subdirectory(poppy)
