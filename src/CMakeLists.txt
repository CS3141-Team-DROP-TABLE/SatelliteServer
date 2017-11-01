link_directories(/usr/local/lib)

set(LIBS ${LIBS} gnutls pthread linkedList ping serverConnect)

add_executable(client client.c)
target_link_libraries(client ${LIBS})


add_subdirectory(lib)
