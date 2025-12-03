cmake_minimum_required(VERSION 3.28)
project(Roborun LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.1
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(SFML)

FetchContent_Declare(
  glaze
  GIT_REPOSITORY https://github.com/stephenberry/glaze.git
  GIT_TAG main
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glaze)

if (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -static-libgcc -static-libstdc++")

    add_executable(Roborun WIN32 src/main.cpp)
else()
    add_executable(Roborun src/main.cpp)
endif ()
target_compile_features(Roborun PRIVATE cxx_std_17)
target_link_libraries(Roborun PRIVATE SFML::Graphics SFML::Audio glaze::glaze )
FILE(COPY src/assets DESTINATION "${CMAKE_BINARY_DIR}/bin")
