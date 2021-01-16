file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
find_package(fmt)

add_library(logging STATIC ${HEADERS} ${SOURCES})
target_link_libraries(logging PRIVATE fmt::fmt)
