set(TARGET qtmedia_fmradio)

find_package(Qt5Multimedia REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(${TARGET} MODULE
    error.cpp
    error.h
    plugin.cpp
    plugin.h
    radio_control.cpp
    radio_control.h
    radio_service.cpp
    radio_service.h
    types.cpp
    types.h
)

target_link_libraries(${TARGET} PRIVATE
    FmRadioCommon
    Qt5::Core
    Qt5::DBus
    Qt5::Multimedia
)

install(
    TARGETS ${TARGET}
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/qt5/plugins/mediaservice"
)
