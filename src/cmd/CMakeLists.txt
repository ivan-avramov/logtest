add_executable (main main.cpp)

target_link_libraries (main PRIVATE logging ${CMAKE_THREAD_LIBS_INIT} )

install (TARGETS main DESTINATION bin)
