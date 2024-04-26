include(SourceFiles.cmake)

# poppy
add_executable(poppy)
set_target_properties(poppy PROPERTIES LINKER_LANGUAGE CXX)
BLSetCompilerOptions(poppy)

target_include_directories(poppy
    PUBLIC
      src
    PRIVATE
      src/poppy
      ${rapidjson_SOURCE_DIR}/include
)

target_link_libraries(poppy
  PRIVATE
    bloom
    utility
    vml
    yaml-cpp
    ImGui
    ImGuizmo
    scatha
    "-framework AppKit"
    "-framework Metal"
    "-framework GameController"
    "-framework CoreImage"
    "-framework QuartzCore"
)

target_sources(poppy
  PRIVATE
    ${poppy_sources}
    ${poppy_shader_sources}
)
source_group(TREE ${PROJECT_SOURCE_DIR}/poppy/src/Poppy FILES ${poppy_sources})

set_source_files_properties(${poppy_shader_sources} PROPERTIES LANGUAGE METAL)

set_target_properties(poppy
  PROPERTIES
    XCODE_ATTRIBUTE_MTL_HEADER_SEARCH_PATHS
      "${PROJECT_SOURCE_DIR}/Poppy/src ${PROJECT_SOURCE_DIR}/Bloom/src/ ${utility_SOURCE_DIR}/include"
)

add_custom_target(touch_timestamp
    touch ${CMAKE_CURRENT_SOURCE_DIR}/src/Poppy/Core/Timestamp.cpp
)
add_dependencies(poppy touch_timestamp)
