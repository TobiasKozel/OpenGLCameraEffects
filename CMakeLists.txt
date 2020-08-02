cmake_minimum_required(VERSION 3.10)

project(dof_example)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB SRC_SHADERS "src/shaders/*.h")
file(GLOB SRC_MAIN "src/*.h" "src/*.cpp")
file(GLOB SRC_WRAP "src/wrapper/*.h")
file(GLOB SRC_UTIL "src/util/*.h")

file(GLOB SRC_IMGUI
	"external/imgui/*.cpp"
	"external/imgui/examples/imgui_impl_glfw.cpp"
	"external/imgui/examples/imgui_impl_opengl3.cpp"
)

file(GLOB GLAD_SRC "external/glad/src/glad.c")

add_executable(dof_example
	${SRC_MAIN} ${SRC_SHADERS} ${SRC_WRAP}
	${SRC_UTIL} ${SRC_IMGUI} ${GLAD_SRC}
)

source_group("shaders" FILES ${SRC_SHADERS})
source_group("main" FILES ${SRC_MAIN})
source_group("util" FILES ${SRC_UTIL})
source_group("wrapper" FILES ${SRC_WRAP})
source_group("imgui" FILES ${SRC_IMGUI})
source_group("glad" FILES ${GLAD_SRC})


if(MSVC)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY build/$<0:>)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY build/$<0:>)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY build/$<0:>)
	set(CMAKE_SUPPRESS_REGENERATION true) # gets rid of some of the cmake junk
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT dof_example)
	# set visual studio debugger working directory
	set_target_properties(dof_example PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "build/")

else()
	add_definitions(-DASSET_PATH="${CMAKE_CURRENT_SOURCE_DIR}/build/")
endif()

# Don't build glfw tests, docs and examples
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(external/glfw/)
target_link_libraries(dof_example glfw)

include_directories("external/glm/include")
include_directories("external/headeronly")
include_directories("external/imgui")
include_directories("external/imgui/examples")
include_directories("external/glad/include")

# preprocessor
add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GLAD)


find_package(OpenGL REQUIRED)
target_link_libraries(dof_example ${OPENGL_LIBRARIES})
