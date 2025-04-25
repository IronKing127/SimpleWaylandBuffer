cmake_minimum_required(VERSION 3.24)

project(simpleWaylandBuffer)

#C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#Export compile commands which allows clangd lsp to work
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#Get wayland package
find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

set(MY_WL_COMPONENTS "Client" "Cursor")
find_package(Wayland REQUIRED COMPONENTS ${MY_WL_COMPONENTS})
find_package(WaylandProtocols REQUIRED)

#Add executable
add_executable(simpleWaylandBuffer)

#Add include directories and sources
target_include_directories(simpleWaylandBuffer PUBLIC
${Wayland_INCLUDE_DIRS}
${CMAKE_SOURCE_DIR}/include)

target_sources(simpleWaylandBuffer PUBLIC ${CMAKE_SOURCE_DIR}/src/main.cpp
	${CMAKE_SOURCE_DIR}/src/PixelPool.cpp
	${CMAKE_SOURCE_DIR}/src/SharedMemory.cpp
	${CMAKE_SOURCE_DIR}/src/xdg-shell.c
)

target_link_libraries(simpleWaylandBuffer ${Wayland_LIBRARIES})
