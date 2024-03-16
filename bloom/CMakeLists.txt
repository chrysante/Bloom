include(SourceFiles.cmake)

# bloom
add_library(bloom SHARED)
set_target_properties(bloom PROPERTIES LINKER_LANGUAGE CXX)
BLSetCompilerOptions(bloom)
target_compile_options(bloom PRIVATE "-fobjc-arc")

target_include_directories(bloom
    PUBLIC
      src
      ${EnTT_SOURCE_DIR}/src
    PRIVATE
      src/Bloom
)

target_link_libraries(bloom
  PUBLIC
    utility
  PRIVATE
    range-v3
    assimp
    glfw
    scatha
    libsvm
    utility
    yaml-cpp
    termfmt
    "-framework AppKit"
    "-framework Metal"
    "-framework IOKit"
    "-framework QuartzCore"
)

target_sources(bloom
  PRIVATE
    ${bloom_sources}
)
source_group(TREE ${PROJECT_SOURCE_DIR}/bloom/src/Bloom FILES ${bloom_sources})

if(NOT BLOOM_BUILD_TESTS)
  return()
endif()

# bloom-test
add_executable(bloom-test)
set_target_properties(bloom-test PROPERTIES LINKER_LANGUAGE CXX)
BLSetCompilerOptions(bloom-test)

target_include_directories(bloom-test
    PRIVATE
      src
      test
)

target_link_libraries(bloom-test
  PRIVATE
    bloom
    Catch2::Catch2WithMain
)

target_sources(bloom-test
  PRIVATE
    ${bloom_test_sources}
)
source_group(TREE ${PROJECT_SOURCE_DIR}/bloom/test FILES ${bloom_test_sources})
