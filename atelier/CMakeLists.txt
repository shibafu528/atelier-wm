cmake_minimum_required(VERSION 2.8)
find_package(X11 REQUIRED)
include_directories(${X11_INCLUDE_DIR})
add_definitions(-DRESOURCE_DIR="${CMAKE_INSTALL_PREFIX}/share/atelier")
add_executable(atelier atelier.c window.c panel.c time.c resource.c event.c)
target_link_libraries(atelier ${X11_LIBRARIES})
target_link_libraries(atelier jansson)

install(PROGRAMS atelier DESTINATION bin)
file(GLOB files "${CMAKE_CURRENT_SOURCE_DIR}/*.xbm")
install(FILES ${files} atelierrc.default DESTINATION share/atelier)

