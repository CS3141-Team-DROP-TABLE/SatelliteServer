link_directories(/usr/local/lib)

set(LIBS ${LIBS} AVL stringMap configLoader gnutls pthread linkedList ping serverConnect)

add_executable(client client.c)
target_link_libraries(client ${LIBS})


add_subdirectory(lib)
