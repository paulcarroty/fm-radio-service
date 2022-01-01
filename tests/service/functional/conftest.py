import os
import os.path
import sys
import subprocess

from pathlib import Path
from subprocess import Popen
from time import sleep
import shutil

import dbus
import dbus.mainloop.glib
import dbusmock
import pytest

dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)


@pytest.fixture(scope="function")
def fm_radio_service_full(request, bus_obj, pulseaudio, cherokee, fm_bridge,
                          fm_radio_service):
    """ Just a composition of fixtures which gives a working environment for
    the fm-radio-service """
    return (fm_radio_service, fm_bridge)


@pytest.fixture(scope="function")
def fm_radio_inactivity_timeout(request):
    return '1'  # seconds


@pytest.fixture(scope="function")
def fm_radio_service(request, fm_radio_inactivity_timeout):
    """ Manually spawn a new FM radio service instance
    """
    service_name = "com.lomiri.FMRadioService"
    bus = dbus.SessionBus()

    if 'SERVICE_BINARY' not in os.environ:
        print('"SERVICE_BINARY" environment variable must be set to full'
              'path of service executable.')

        sys.exit(1)

    environment = os.environ.copy()
    environment['FM_RADIO_SERVICE_TIMEOUT'] = fm_radio_inactivity_timeout
    environment['PATH'] = os.environ['PATH_FOR_TESTS']

    # Spawn the service, and wait for it to appear on the bus
    args = [os.environ['SERVICE_BINARY']]
    if 'WRAPPER' in os.environ:
        args = os.environ['WRAPPER'].split() + args
    service = Popen(
        args,
        stdout=sys.stdout,
        stderr=sys.stderr,
        env=environment)

    for i in range(0, 100):
        if service.poll() is not None or bus.name_has_owner(service_name):
            break
        sleep(0.1)

    assert service.poll() is None, "service is not running"
    assert bus.name_has_owner(service_name)

    def teardown():
        service.terminate()
        service.wait()
    request.addfinalizer(teardown)

    return service


@pytest.fixture(scope="function")
def other_client(request, bus_obj, current_path):
    script_path = current_path.joinpath("radio_client.py")
    client = subprocess.Popen([sys.executable, str(script_path)],
                              stdin=subprocess.PIPE,
                              stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE)

    def writeline(line):
        client.stdin.write(line.encode('utf-8') + b'\n')
        client.stdin.flush()

    def readline():
        return client.stdout.readline().strip().decode('utf-8')

    client.readline = readline
    client.writeline = writeline

    def teardown():
        client.terminate()
        client.wait()
    request.addfinalizer(teardown)

    return client


@pytest.fixture(scope="session")
def bin_path(request):
    path = os.environ['PATH_FOR_TESTS']
    os.makedirs(path, exist_ok=True)
    return Path(path)


@pytest.fixture(scope="function")
def cherokee(request, bin_path):
    getprop = bin_path / "getprop"
    print('Getprop: {}'.format(getprop))
    getprop.write_text('#! /bin/sh\necho cherokee\n')
    getprop.chmod(0o777)

    def teardown():
        print('Removing getprop')
        getprop.unlink()
    request.addfinalizer(teardown)


@pytest.fixture(scope="function")
def bridge_handler(request):
    return lambda handler: print('Got: {}'.format(handler.rfile.readline()))

@pytest.fixture(scope="function")
def fm_bridge(request, bin_path, bridge_handler):
    # Create a TCP server: the fake fm-bridge will connect to it and forward
    # all the communication. In this way we'll be able to send arbitrary
    # responses to the cherokee backend.
    import socketserver
    class TCPHandler(socketserver.StreamRequestHandler):
        def handle(self):
            pass

    server = socketserver.TCPServer(('localhost', 0), TCPHandler)

    bridge = bin_path / "fm-bridge"
    bridge.write_text('#! /bin/sh\nexec /bin/nc {} {}\n'.format(*server.server_address))
    bridge.chmod(0o777)

    def teardown():
        bridge.unlink()
        server.server_close()
    request.addfinalizer(teardown)

    return server


@pytest.fixture(scope="session")
def current_path():
    return Path(__file__).parent


@pytest.fixture(scope="session")
def data_path(request, current_path):
    return current_path.parent.joinpath('data')


@pytest.fixture(scope="function")
def service_name():
    return "core.ubuntu.media.Service"


@pytest.fixture(scope="function")
def request_name(request, service_name):
    """ Occupy the service bus name

    This is intended as a helper to test how the service behaves when its
    bus name is already taken.
    """

    bus = dbus.SessionBus()

    def teardown():
        # Make sure the name is no longer there
        bus.release_name(service_name)
    request.addfinalizer(teardown)

    bus.request_name(service_name, dbus.bus.NAME_FLAG_REPLACE_EXISTING)


@pytest.fixture(scope="function")
def dbus_monitor(request, bus_obj):
    """Add this fixture to your test to debug it"""
    monitor = Popen(['/usr/bin/dbus-monitor', '--session'], stdout=sys.stdout)
    sleep(1)

    def teardown():
        monitor.terminate()
        monitor.wait()
    request.addfinalizer(teardown)


@pytest.fixture(scope="function")
def pulseaudio(request, bus_obj):
    """ Pulseaudio fake server """
    pulseaudio = Popen([
        'pulseaudio',
        '-n',
        '--load=module-dbus-protocol',
        '--load=module-null-sink',
        '--load=module-null-source source_name=FmTestSource',
        '--load=module-native-protocol-unix',
        '--load=module-cli-protocol-unix',
    ], stdout=sys.stdout)

    bus = dbus.SessionBus()
    for i in range(0, 100):
        if pulseaudio.poll() is not None or bus.name_has_owner('org.PulseAudio1'):
            break
        sleep(0.1)

    def teardown():
        pulseaudio.terminate()
        pulseaudio.wait()
    request.addfinalizer(teardown)


@pytest.fixture(scope="session")
def bus_obj(request):
    """ Create a new SessionBus

    The bus is returned, and can be used to interface with the service
    """
    dbus_conf = os.environ['DBUS_CONF']
    dbus_daemon = shutil.which('dbus-daemon')
    start_dbus_daemon_command = [
        dbus_daemon,
        "--config-file={}".format(dbus_conf),
        "--nofork",
        "--print-address",
    ]

    # Change the PATH to make sure no binaries from the host are executed
    environment = os.environ.copy()
    environment['PATH'] = os.environ['PATH_FOR_TESTS']

    dbus_daemon = Popen(
        start_dbus_daemon_command,
        shell=False,
        stdout=subprocess.PIPE,
        env=environment)

    # Read the path of the newly created socket from STDOUT of
    # dbus-daemon, and set up the environment
    line = dbus_daemon.stdout.readline().decode('utf8')
    dbus_address = line.strip().split(",")[0]
    os.environ["DBUS_SESSION_BUS_ADDRESS"] = dbus_address
    os.environ["DBUS_SYSTEM_BUS_ADDRESS"] = dbus_address
    bus = dbus.SessionBus()

    def teardown():
        bus.close()

        dbus_daemon.kill()
        dbus_daemon.wait()

    request.addfinalizer(teardown)
    return bus
