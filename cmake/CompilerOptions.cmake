
function(BLSetCompilerOptions target)
  if(NOT BLOOM_DEV_MODE)
    return()
  endif()

  if(WIN32)
    if(MSVC)
      target_compile_options(${target} PRIVATE "/W3")
      target_compile_options(${target} PRIVATE "/WX")
      target_compile_options(${target} PRIVATE "/D_CRT_SECURE_NO_WARNINGS")
    endif()
  else()
    target_compile_options(${target} PRIVATE "-Wall")
    target_compile_options(${target} PRIVATE "-Wextra")
    target_compile_options(${target} PRIVATE "-pedantic")

    target_compile_options(${target} PRIVATE "-Wdeprecated")
    target_compile_options(${target} PRIVATE "-Wmissing-declarations")
    target_compile_options(${target} PRIVATE "-Wnon-virtual-dtor")
    target_compile_options(${target} PRIVATE "-Wnull-dereference")
    target_compile_options(${target} PRIVATE "-Woverloaded-virtual")
    target_compile_options(${target} PRIVATE "-Wunused")
  endif()

  target_compile_definitions(${target} PRIVATE BUILD_CONFIG=$<CONFIG>)

  if(APPLE)
    # Enable ARC in Objective-C
    set_target_properties(${target} PROPERTIES
      XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
    )
  endif()

endfunction()

if(BLOOM_DEV_MODE)
  if(NOT MSVC)
    add_compile_options("$<$<CONFIG:Debug>:-fsanitize=address>")
    add_link_options("$<$<CONFIG:Debug>:-fsanitize=address>")
    add_compile_options("$<$<CONFIG:Debug>:-fsanitize=undefined>")
    add_link_options("$<$<CONFIG:Debug>:-fsanitize=undefined>")
  endif()
endif()
