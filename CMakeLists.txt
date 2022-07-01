cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

project(SSAO_SSDO)

link_directories(${CMAKE_SOURCE_DIR}/lib)

file(GLOB_RECURSE SOURCE_FILES
    ${CMAKE_SOURCE_DIR}/src/includes/*.c
    ${CMAKE_SOURCE_DIR}/src/includes/*.cpp
	${CMAKE_SOURCE_DIR}/src/imgui/*.c
    ${CMAKE_SOURCE_DIR}/src/imgui/*.cpp
)
file(GLOB_RECURSE HEADER_FILES
    ${CMAKE_SOURCE_DIR}/src/includes/*.h
    ${CMAKE_SOURCE_DIR}/src/includes/*.hpp
	${CMAKE_SOURCE_DIR}/src/imgui/*.h
    ${CMAKE_SOURCE_DIR}/src/imgui/*.hpp  
)

configure_file(src/helpers/RootDir.h.in src/helpers/RootDir.h)
include_directories(${CMAKE_BINARY_DIR}/src)

add_executable(SSAO ${HEADER_FILES} ${SOURCE_FILES} ${CMAKE_SOURCE_DIR}/src/SSAO.cpp)
add_executable(SSDO ${HEADER_FILES} ${SOURCE_FILES} ${CMAKE_SOURCE_DIR}/src/SSDO.cpp)
add_executable(SSAOplusSSDO ${HEADER_FILES} ${SOURCE_FILES} ${CMAKE_SOURCE_DIR}/src/SSAOplusSSDO.cpp)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)

find_package(GLM REQUIRED)
message(STATUS "GLM included at ${GLM_INCLUDE_DIR}")
find_package(GLFW3 REQUIRED)
message(STATUS "Found GLFW3 in ${GLFW3_INCLUDE_DIR}")
find_package(ASSIMP REQUIRED)
message(STATUS "Found ASSIMP in ${ASSIMP_INCLUDE_DIR}")

add_library(STB_IMAGE "third_party/stb_image.cpp")
add_library(GLAD "third_party/glad.c")
set(LIBS ${GLFW3_LIBRARY} ${OPENGL_LIBRARY} GLAD ${CMAKE_DL_LIBS} ${ASSIMP_LIBRARY} STB_IMAGE)

include_directories(
    "${CMAKE_SOURCE_DIR}/src"
	"${CMAKE_SOURCE_DIR}/src/imgui"
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/include/glad"
    "${CMAKE_SOURCE_DIR}/include/glm"
)
target_link_libraries(SSAO ${LIBS})
target_link_libraries(SSDO ${LIBS})
target_link_libraries(SSAOplusSSDO ${LIBS})
if(MSVC_IDE)
	# Macro to preserve source files hierarchy in the IDE
	macro(GroupSources curdir)
		file(GLOB children RELATIVE ${PROJECT_SOURCE_DIR}/${curdir} ${PROJECT_SOURCE_DIR}/${curdir}/*)

		foreach(child ${children})
			if(IS_DIRECTORY ${PROJECT_SOURCE_DIR}/${curdir}/${child})
				GroupSources(${curdir}/${child})
			else()
				string(REPLACE "/" "\\" groupname ${curdir})
				string(REPLACE "src" "Sources" groupname ${groupname})
				source_group(${groupname} FILES ${PROJECT_SOURCE_DIR}/${curdir}/${child})
			endif()
		endforeach()
	endmacro()

	# Run macro
	GroupSources(src)
endif()

# Copy dlls
if(WIN32)
	add_custom_command(TARGET SSAO POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${PROJECT_SOURCE_DIR}/dlls"
		$<TARGET_FILE_DIR:SSAO>)
endif()

if(WIN32)
	add_custom_command(TARGET SSDO POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${PROJECT_SOURCE_DIR}/dlls"
		$<TARGET_FILE_DIR:SSDO>)
endif()

if(WIN32)
	add_custom_command(TARGET SSAOplusSSDO POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${PROJECT_SOURCE_DIR}/dlls"
		$<TARGET_FILE_DIR:SSAOplusSSDO>)
endif()