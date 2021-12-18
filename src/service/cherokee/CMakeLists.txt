set(TARGET BackendCherokee)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(${TARGET}
    radio_control.cpp
    radio_control.h
)
target_link_libraries(${TARGET} PRIVATE
    Audio
    RadioControl
    Qt5::Core)
