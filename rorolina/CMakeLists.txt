cmake_minimum_required(VERSION 2.8)
find_package(GTK2 2.24 REQUIRED gtk)
include_directories(${GTK2_INCLUDE_DIRS})
add_executable(rorolina rorolina.c entrysearch.c)
target_link_libraries(rorolina ${GTK2_LIBRARIES})

install(PROGRAMS rorolina DESTINATION bin)
