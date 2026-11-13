cmake_minimum_required(VERSION 3.21)

project(TrainSchedulingEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SFML 3.1 COMPONENTS Graphics Window System REQUIRED)

add_executable(TrainScheduling
    src/main.cpp
    src/FileManager.cpp
    src/GraphicalSimulationEngine.cpp
    src/GuiMenu.cpp
    src/MapData.cpp
    src/StationList.cpp
    src/TrainList.cpp
    src/UiKit.cpp
)

target_include_directories(TrainScheduling PRIVATE include)
target_link_libraries(TrainScheduling PRIVATE SFML::Graphics SFML::Window SFML::System)

set_target_properties(TrainScheduling PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

if(WIN32)
    add_custom_command(TARGET TrainScheduling POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:TrainScheduling>
                $<TARGET_FILE_DIR:TrainScheduling>
        COMMAND_EXPAND_LISTS
    )
endif()

if(MSVC)
    target_compile_options(TrainScheduling PRIVATE /utf-8)
endif()
