target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

file(GLOB_RECURSE SOURCES *.cpp)

target_sources(${PROJECT_NAME} PRIVATE ${SOURCES})