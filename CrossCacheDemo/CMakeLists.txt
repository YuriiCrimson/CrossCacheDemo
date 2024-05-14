cmake_minimum_required(VERSION 3.5)

project(CrossCacheDemo LANGUAGES C)

add_executable(CrossCacheDemo main.c)
target_link_libraries(CrossCacheDemo -static)

include(GNUInstallDirs)
install(TARGETS CrossCacheDemo
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
