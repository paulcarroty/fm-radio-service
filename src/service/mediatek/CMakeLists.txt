set(TARGET BackendMediatek)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(${TARGET}
    fmr/common.cpp
    fmr/fmr.h
    radio_control.cpp
    radio_control.h
)
target_link_libraries(${TARGET} PRIVATE
    Audio
    RadioControl
    Qt5::Core)
target_include_directories(${TARGET} PRIVATE
    include  # all of Mediatek platform and kernel includes
)
