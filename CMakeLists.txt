project(nowplaying-lite)
find_package(KDE4 REQUIRED)
include(KDE4Defaults)

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories (${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${KDE4_INCLUDES})

set(nowplaying_SRCS
    nowplaying.cpp
    albumart.cpp
    infopanel.cpp)

kde4_add_plugin(plasma_applet_nowplaying_lite ${nowplaying_SRCS})
target_link_libraries(plasma_applet_nowplaying_lite  ${KDE4_PLASMA_LIBS} ${KDE4_KDEUI_LIBS})

install(FILES nocover.svgz DESTINATION ${DATA_INSTALL_DIR}/desktoptheme/default/widgets/nowplaying-lite/)
install(TARGETS plasma_applet_nowplaying_lite DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES plasma-applet-nowplaying-lite.desktop DESTINATION ${SERVICES_INSTALL_DIR})
