pkg_check_modules(PULSE REQUIRED libpulse libpulse-mainloop-glib)

set(TARGET fm-radio-service)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# We'll link everything statically into a single executable, but this
# subdivision allows us to move the plugins into subdirectories
add_library(RadioControl
    abstract_radio_control.cpp
    abstract_radio_control.h
    error.cpp
    error.h
    types.h
)
target_include_directories(RadioControl PUBLIC .)
target_link_libraries(RadioControl PUBLIC Qt5::Core)

add_library(Audio
    audio/context.cpp
    audio/context.h
    audio/operation.h
    audio/output_observer.cpp
    audio/output_observer.h
    audio/playback.cpp
    audio/playback.h
)
target_include_directories(Audio
    PUBLIC .
    PRIVATE ${PULSE_INCLUDE_DIRS}
)
target_link_libraries(Audio PUBLIC
    ${PULSE_LIBRARIES}
    Qt5::Core
)

add_executable(${TARGET}
    dbus_error.cpp
    dbus_error.h
    dbus_property_notifier.cpp
    dbus_property_notifier.h
    inactivity_timer.cpp
    inactivity_timer.h
    main.cpp
    radio_control_factory.cpp
    radio_control_factory.h
    service.cpp
    service.h
    service_adaptor.cpp
    service_adaptor.h
    tuner.cpp
    tuner.h
    tuner_adaptor.cpp
    tuner_adaptor.h
)

target_link_libraries(${TARGET} PRIVATE
    FmRadioCommon
    RadioControl
    Qt5::Core
    Qt5::DBus
    ${PULSE_AUDIO_LIBRARIES}
)

target_include_directories(${TARGET} PRIVATE
    ${PULSE_AUDIO_INCLUDE_DIRS}
)

install(
    TARGETS ${TARGET}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)

set(SERVICE_FILE com.lomiri.FMRadioService.service)
configure_file(${SERVICE_FILE}.in ${SERVICE_FILE})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${SERVICE_FILE}
    DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/services
)
