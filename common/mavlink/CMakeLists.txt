target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/command_service
    ${CMAKE_CURRENT_SOURCE_DIR}/communicator
    ${CMAKE_CURRENT_SOURCE_DIR}/handlers
    ${CMAKE_CURRENT_SOURCE_DIR}/links
    ${CMAKE_CURRENT_SOURCE_DIR}/vehicle_registry
    ${CMAKE_CURRENT_SOURCE_DIR}
)

file(GLOB_RECURSE SOURCES *.cpp)

target_sources(${PROJECT_NAME} PRIVATE ${SOURCES})