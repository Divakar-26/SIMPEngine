add_executable(SIMPTests
    main.cpp
    CoreTests.cpp
    SerializerTests.cpp
    GameplayTests.cpp
)
target_link_libraries(SIMPTests PRIVATE Engine)
set_target_properties(SIMPTests PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
add_test(NAME SIMPTests COMMAND SIMPTests)
if(WIN32)
    add_custom_command(TARGET SIMPTests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:SIMPTests> $<TARGET_FILE_DIR:SIMPTests>
        COMMAND_EXPAND_LISTS)
endif()
