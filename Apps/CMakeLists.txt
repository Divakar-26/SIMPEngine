set(SIMP_APP_DIR "${CMAKE_BINARY_DIR}/bin")

add_executable(SIMPPlayer SIMPPlayer/main.cpp)
target_link_libraries(SIMPPlayer PRIVATE Engine imgui)

set(SIMP_APPS SIMPPlayer)
if(NOT NO_EDITOR)
    add_executable(SIMPEditor SIMPEditor/main.cpp)
    target_link_libraries(SIMPEditor PRIVATE Engine Editor imgui)
    add_dependencies(SIMPEditor SIMPPlayer)
    list(APPEND SIMP_APPS SIMPEditor)
endif()

foreach(app IN LISTS SIMP_APPS)
    set_target_properties(${app} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${SIMP_APP_DIR}"
        BUILD_RPATH "\$ORIGIN/lib"
        INSTALL_RPATH "\$ORIGIN/lib")
    if(WIN32)
        add_custom_command(TARGET ${app} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:${app}> $<TARGET_FILE_DIR:${app}>
            COMMAND_EXPAND_LISTS)
    elseif(UNIX AND NOT APPLE)
        add_custom_command(TARGET ${app} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -DEXECUTABLE=$<TARGET_FILE:${app}> -DDESTINATION=${SIMP_APP_DIR}/lib
                    -P ${CMAKE_SOURCE_DIR}/cmake/CopyRuntimeLibs.cmake)
    endif()
endforeach()

install(TARGETS ${SIMP_APPS} RUNTIME DESTINATION . COMPONENT Editor)
if(UNIX AND NOT APPLE)
    install(DIRECTORY "${SIMP_APP_DIR}/lib/" DESTINATION lib COMPONENT Editor)
elseif(WIN32)
    install(CODE "file(GLOB SIMP_DLLS \"${SIMP_APP_DIR}/*.dll\")
                  file(INSTALL \${SIMP_DLLS} DESTINATION \"\${CMAKE_INSTALL_PREFIX}\")" COMPONENT Editor)
endif()
install(DIRECTORY "${CMAKE_SOURCE_DIR}/assets/fonts/" DESTINATION engine/fonts COMPONENT Editor
    FILES_MATCHING PATTERN "Inter-*.ttf" PATTERN "JetBrainsMonoNerdFont-*.ttf" PATTERN "lucide.ttf" PATTERN "LICENSE-*.txt")
install(DIRECTORY "${CMAKE_SOURCE_DIR}/assets/editor/" DESTINATION engine/editor COMPONENT Editor)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/docs/" DESTINATION engine/docs COMPONENT Editor)
install(FILES "${CMAKE_SOURCE_DIR}/README.md" "${CMAKE_SOURCE_DIR}/CHANGELOG.md" "${CMAKE_SOURCE_DIR}/LICENSE" DESTINATION . COMPONENT Editor OPTIONAL)

set(CPACK_PACKAGE_NAME "SIMPEngine")
set(CPACK_PACKAGE_VENDOR "SIMPEngine")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "2D game engine and editor")
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_ALL Editor)
set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)
set(CPACK_PACKAGE_FILE_NAME "SIMPEngine-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY ON)
set(CPACK_COMPONENT_INCLUDE_TOPLEVEL_DIRECTORY ON)
if(WIN32)
    set(CPACK_GENERATOR ZIP)
else()
    set(CPACK_GENERATOR TGZ)
endif()
include(CPack)
