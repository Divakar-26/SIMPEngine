add_library(main SHARED ${CMAKE_SOURCE_DIR}/Apps/SIMPPlayer/main.cpp)
target_link_libraries(main PRIVATE Engine imgui SDL3::SDL3 log android)
set_target_properties(main PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
