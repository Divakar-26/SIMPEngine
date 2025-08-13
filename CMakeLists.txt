cmake_minimum_required(VERSION 3.16)
project(MyGameEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build SDL3 from vendor folder
add_subdirectory(vendor/SDL EXCLUDE_FROM_ALL)
add_subdirectory(vendor/spdlog EXCLUDE_FROM_ALL)
add_subdirectory(vendor/glm EXCLUDE_FROM_ALL)
add_subdirectory(vendor/SDL_image EXCLUDE_FROM_ALL)
add_subdirectory(vendor/entt EXCLUDE_FROM_ALL)

# Add engine and game subprojects
add_subdirectory(engine)
add_subdirectory(game)
