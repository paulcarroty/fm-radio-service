project(ServiceFunctionalTests)

set(FM_RADIO_BINARY "${CMAKE_BINARY_DIR}/src/service/fm-radio-service")
configure_file(data/testsession.conf.in data/testsession.conf)
configure_file(data/com.lomiri.FMRadioService.service.in
    data/com.lomiri.FMRadioService.service
)
configure_file(service_wrapper.sh.in service_wrapper.sh)

find_program(PYTEST_COMMAND NAMES py.test-3 py.test)
add_test(test_service ${PYTEST_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR}/ -vvv)

set(environment
    "SERVICE_BINARY=${FM_RADIO_BINARY}"
    "DBUS_CONF=${CMAKE_CURRENT_BINARY_DIR}/data/testsession.conf"
    "PATH_FOR_TESTS=${CMAKE_CURRENT_BINARY_DIR}/bin"
)
set_tests_properties(test_service PROPERTIES ENVIRONMENT "${environment}")
