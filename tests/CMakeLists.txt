add_executable(SIMPTests
    main.cpp
    CoreTests.cpp
    SerializerTests.cpp
    GameplayTests.cpp
    LuaHelperTests.cpp
    EffectTests.cpp
    MotionTests.cpp
    RenderingTests.cpp
    PhysicsTests.cpp
    InputTests.cpp
    DisplayTests.cpp
    UITests.cpp
    PrefabTests.cpp
    ShaderTests.cpp
    ScriptEditorTests.cpp
    MarkdownTests.cpp
    ../Editor/Docs/Markdown.cpp
    ../Editor/ScriptEditor/LuaLexer.cpp
    ../Editor/ScriptEditor/LuaContext.cpp
    ../Editor/ScriptEditor/LuaNavigation.cpp
    ../Editor/ScriptEditor/LuaCompletion.cpp
)
target_include_directories(SIMPTests PRIVATE ${CMAKE_SOURCE_DIR}/Editor/ScriptEditor ${CMAKE_SOURCE_DIR}/Editor)
target_link_libraries(SIMPTests PRIVATE Engine)
target_compile_definitions(SIMPTests PRIVATE SIMP_TEST_ASSETS="${CMAKE_SOURCE_DIR}/assets")
set_target_properties(SIMPTests PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
add_test(NAME SIMPTests COMMAND SIMPTests)
if(WIN32)
    add_custom_command(TARGET SIMPTests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:SIMPTests> $<TARGET_FILE_DIR:SIMPTests>
        COMMAND_EXPAND_LISTS)
endif()
