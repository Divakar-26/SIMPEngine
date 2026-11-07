include(GNUInstallDirs)

set(SIMP_INSTALL_INCLUDE "${CMAKE_INSTALL_INCLUDEDIR}/simpengine")
set(SIMP_INSTALL_LIB "${CMAKE_INSTALL_LIBDIR}/simpengine")
set(SIMP_INSTALL_CMAKE "${CMAKE_INSTALL_LIBDIR}/cmake/SIMPEngine")
set(SIMP_INSTALL_SHARE "${CMAKE_INSTALL_DATADIR}/simpengine")

function(simp_real_target out target)
    get_target_property(aliased ${target} ALIASED_TARGET)
    if(aliased)
        set(${out} ${aliased} PARENT_SCOPE)
    else()
        set(${out} ${target} PARENT_SCOPE)
    endif()
endfunction()

simp_real_target(SIMP_SDL SDL3::SDL3)
simp_real_target(SIMP_SDL_IMAGE SDL3_image::SDL3_image)
simp_real_target(SIMP_YAML yaml-cpp)
simp_real_target(SIMP_SPDLOG spdlog)

set(SIMP_LINK_TARGETS Engine imgui glad AccelEngine lua ${SIMP_YAML} ${SIMP_SPDLOG} ${SIMP_SDL_IMAGE} ${SIMP_SDL})

install(TARGETS ${SIMP_LINK_TARGETS}
    ARCHIVE DESTINATION "${SIMP_INSTALL_LIB}"
    LIBRARY DESTINATION "${SIMP_INSTALL_LIB}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

install(DIRECTORY "${CMAKE_SOURCE_DIR}/Engine/Engine/"
    DESTINATION "${SIMP_INSTALL_INCLUDE}/Engine"
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")

install(DIRECTORY "${CMAKE_SOURCE_DIR}/vendor/entt/src/" DESTINATION "${SIMP_INSTALL_INCLUDE}"
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")
install(DIRECTORY "${CMAKE_SOURCE_DIR}/vendor/glm/glm" DESTINATION "${SIMP_INSTALL_INCLUDE}"
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp" PATTERN "*.inl")
install(DIRECTORY "${CMAKE_SOURCE_DIR}/vendor/glad/include/" DESTINATION "${SIMP_INSTALL_INCLUDE}")
install(DIRECTORY "${CMAKE_SOURCE_DIR}/vendor/yaml-cpp/include/" DESTINATION "${SIMP_INSTALL_INCLUDE}")
install(DIRECTORY "${CMAKE_SOURCE_DIR}/vendor/AccelEngine/include/" DESTINATION "${SIMP_INSTALL_INCLUDE}")
install(DIRECTORY "${CMAKE_SOURCE_DIR}/vendor/spdlog/include/" DESTINATION "${SIMP_INSTALL_INCLUDE}")
install(DIRECTORY "${CMAKE_SOURCE_DIR}/vendor/SDL/include/SDL3" DESTINATION "${SIMP_INSTALL_INCLUDE}")
install(DIRECTORY "${CMAKE_SOURCE_DIR}/vendor/SDL_image/include/SDL3_image" DESTINATION "${SIMP_INSTALL_INCLUDE}")
install(DIRECTORY "${CMAKE_SOURCE_DIR}/vendor/json/include/" DESTINATION "${SIMP_INSTALL_INCLUDE}")
install(DIRECTORY "${CMAKE_SOURCE_DIR}/vendor/nanosvg/src/" DESTINATION "${SIMP_INSTALL_INCLUDE}"
    FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${CMAKE_SOURCE_DIR}/vendor/stb/" DESTINATION "${SIMP_INSTALL_INCLUDE}"
    FILES_MATCHING PATTERN "stb_*.h"
    PATTERN "deprecated" EXCLUDE PATTERN "tests" EXCLUDE PATTERN "data" EXCLUDE PATTERN "docs" EXCLUDE)
install(FILES
    "${CMAKE_SOURCE_DIR}/vendor/imgui/imgui.h"
    "${CMAKE_SOURCE_DIR}/vendor/imgui/imconfig.h"
    "${CMAKE_SOURCE_DIR}/vendor/imgui/imgui_internal.h"
    "${CMAKE_SOURCE_DIR}/vendor/imgui/imstb_rectpack.h"
    "${CMAKE_SOURCE_DIR}/vendor/imgui/imstb_textedit.h"
    "${CMAKE_SOURCE_DIR}/vendor/imgui/imstb_truetype.h"
    "${CMAKE_SOURCE_DIR}/vendor/imgui/misc/cpp/imgui_stdlib.h"
    "${CMAKE_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_sdl3.h"
    "${CMAKE_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_opengl3.h"
    "${CMAKE_SOURCE_DIR}/vendor/ImGuizmo/ImGuizmo.h"
    DESTINATION "${SIMP_INSTALL_INCLUDE}")

install(DIRECTORY "${CMAKE_SOURCE_DIR}/assets/fonts" DESTINATION "${SIMP_INSTALL_SHARE}/assets")

set(SIMP_LIB_FILES "")
foreach(target ${SIMP_LINK_TARGETS})
    list(APPEND SIMP_LIB_FILES "$<TARGET_LINKER_FILE_NAME:${target}>")
endforeach()
set(SIMP_DEFINES "$<TARGET_PROPERTY:Engine,INTERFACE_COMPILE_DEFINITIONS>;$<TARGET_PROPERTY:${SIMP_SPDLOG},INTERFACE_COMPILE_DEFINITIONS>;$<TARGET_PROPERTY:${SIMP_YAML},INTERFACE_COMPILE_DEFINITIONS>")
file(RELATIVE_PATH SIMP_PREFIX_FROM_CONFIG "/simp/${SIMP_INSTALL_CMAKE}" "/simp")

configure_file("${CMAKE_SOURCE_DIR}/cmake/SIMPEngineConfig.cmake.in" "${CMAKE_BINARY_DIR}/SIMPEngineConfig.cmake.gen" @ONLY)
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/SIMPEngineConfig.cmake" INPUT "${CMAKE_BINARY_DIR}/SIMPEngineConfig.cmake.gen")
install(FILES "${CMAKE_BINARY_DIR}/SIMPEngineConfig.cmake" DESTINATION "${SIMP_INSTALL_CMAKE}")
