link_directories(/usr/local/lib)

set(LIBS ${LIBS} gnutls pthread linkedList)

add_executable(client ping.c client.c)
target_link_libraries(client ${LIBS})


add_subdirectory(lib)
