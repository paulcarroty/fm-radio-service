set(TARGET FmRadioCommon)

set(SOURCES
    dbus_constants.h
)

add_library(${TARGET} ${SOURCES})

target_include_directories(${TARGET} PUBLIC
    .
)
