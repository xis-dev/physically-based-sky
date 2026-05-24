# CMakeList.txt : CMake project for procedural-sky, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.10)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project (physically-based-sky LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)

include(FetchContent)

FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.89.9
)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.3.8
)
FetchContent_MakeAvailable(glfw)

    
set (LOCAL_DEPENDENCIES_DIR ${PROJECT_SOURCE_DIR}/dependencies)

set(GLAD_DIR ${LOCAL_DEPENDENCIES_DIR}/glad)

set(GLM_DIR ${LOCAL_DEPENDENCIES_DIR}/glm)

set (STB_DIR ${LOCAL_DEPENDENCIES_DIR}/stb)

set(IMGUI_SOURCES
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

set(ALL_DEPENDENCIES_DIRS
    ${LOCAL_DEPENDENCIES_DIR}
    ${GLAD_DIR}
    ${GLM_DIR}
    ${STB_DIR}
    ${glfw_SOURCE_DIR}
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends)


# Header files with no source file included
 set (PROJECT_HEADERS  "includes/PathResolver.h")
 
 # All project source files
 set (PROJECT_SOURCES 
        src/main.cpp
        ${GLAD_DIR}/glad.c
        ${STB_DIR}/stb.cpp
  "src/shader.cpp")
# Add source to this project's executable.
add_executable (physically-based-sky ${PROJECT_SOURCES} ${PROJECT_HEADERS} ${IMGUI_SOURCES})

target_include_directories(physically-based-sky PRIVATE ${PROJECT_SOURCE_DIR} ${ALL_DEPENDENCIES_DIRS} includes)

target_link_libraries(physically-based-sky PRIVATE
        glfw
        ${OPENGL_LIBRARIES}
)


