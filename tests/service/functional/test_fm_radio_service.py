import os
import re
import subprocess
import sys

from gi.repository import GLib
from time import sleep

import dbus
import pytest

import FMRadio


class TestMediaHub:

    def test_no_dbus(self):
        environment = os.environ.copy()
        environment["DBUS_SESSION_BUS_ADDRESS"] = ""
        environment["DBUS_SYSTEM_BUS_ADDRESS"] = ""
        service = subprocess.Popen(
            os.environ["SERVICE_BINARY"],
            stdout=sys.stdout,
            stderr=sys.stderr,
            env=environment)
        service.wait(5)
        assert service.returncode == 1

    @pytest.mark.parametrize('service_name', [
        (FMRadio.ServiceName),
    ])
    def test_steal_name(self, bus_obj, service_name, request_name):
        service = subprocess.Popen(
            os.environ["SERVICE_BINARY"],
            stderr=subprocess.PIPE,)
        service.wait(5)
        output = service.stderr.read()
        assert b'Failed to register' in output
        assert service.returncode == 1

    def test_no_backend(self, bus_obj, fm_radio_service):
        service = FMRadio.Service(bus_obj)

        with pytest.raises(dbus.exceptions.DBusException) as exception:
            service.open_tuner()
        assert exception.value.get_dbus_name() == \
            FMRadio.Error.ResourceError

    def test_unique_session(self, bus_obj, fm_radio_service_full,
                            other_client):
        _, fm_bridge = fm_radio_service_full
        client = other_client

        service = FMRadio.Service(bus_obj)

        tuner_path = service.open_tuner()
        (socket, client_address) = fm_bridge.get_request()
        tuner = FMRadio.Tuner(bus_obj, tuner_path)

        # Open the tuner

        reply = tuner.start(call_async=True)
        commands = socket.recv(4096).splitlines()
        socket.sendall(b'fm_seek_cmpl_cb\n')
        socket.sendall(b'fm_tune_cb 88000\n')
        tuner.loop.run()
        assert reply.finished

        # Open the same tuner from a different process
        client.writeline('start')
        errors = client.stderr.read()
        assert b'The tuner is being used by another application' in errors

    def test_cherokee_backend_without_bridge(self, bus_obj, cherokee,
                                             fm_radio_service):
        service = FMRadio.Service(bus_obj)
        with pytest.raises(dbus.exceptions.DBusException) as exception:
            service.open_tuner()
        assert exception.value.get_dbus_name() == \
            FMRadio.Error.ResourceError

    def test_cherokee_backend(self, bus_obj, pulseaudio, cherokee, fm_bridge, fm_radio_service):
        service = FMRadio.Service(bus_obj)

        tuner_path = service.open_tuner()
        (socket, client_address) = fm_bridge.get_request()
        socket.sendall(b'still ok\n')
        tuner = FMRadio.Tuner(bus_obj, tuner_path)

        # We use asynchronous calls so that while the service is processing
        # them we can control the fm-bridge

        tuner.set_prop('Frequency', dbus.Int32(93900, variant_level=1))
        assert tuner.wait_for_prop('Frequency',
                                   dbus.Int32(93900, variant_level=1))

        reply = tuner.start(call_async=True)
        commands = socket.recv(4096).splitlines()
        print('Got commands: {}'.format(commands))
        assert commands[-1] == b'setFreq 93900'

        # send the "chip" replies
        # - for the initialization
        socket.sendall(b'fm_seek_cmpl_cb\n')
        # - for the frequency change; send a slightly different frequency, to
        #   verify if the backend correctly handles it
        socket.sendall(b'fm_tune_cb 94000\n')
        tuner.loop.run()
        assert reply.finished
        # TODO assert not reply.error (now it's DBusException(dbus.String('FM source port not found'),)

        assert tuner.wait_for_prop('Frequency', dbus.Int32(94000, variant_level=1))
        tuner.stop()
        commands = socket.recv(4096).splitlines()
        assert commands[-1] == b'enableSlimbus 0'

    def test_cherokee_backend_search(self, bus_obj, pulseaudio, cherokee, fm_bridge, fm_radio_service):
        service = FMRadio.Service(bus_obj)

        tuner_path = service.open_tuner()
        (socket, client_address) = fm_bridge.get_request()
        tuner = FMRadio.Tuner(bus_obj, tuner_path)

        reply = tuner.start(call_async=True)
        commands = socket.recv(4096).splitlines()
        socket.sendall(b'fm_seek_cmpl_cb\n')
        socket.sendall(b'fm_tune_cb 88000\n')
        tuner.loop.run()
        assert reply.finished

        # Search forward
        reply = tuner.search_forward(call_async=True)
        commands = socket.recv(4096).splitlines()
        assert commands[-1] == b'startSearch 1'
        socket.sendall(b'fm_seek_cmpl_cb\nfm_tune_cb 88500\n')
        tuner.loop.run()
        assert reply.finished and not reply.error
        assert tuner.wait_for_prop('Frequency',
                                   dbus.Int32(88500, variant_level=1))

        # Search backward
        reply = tuner.search_backward(call_async=True)
        commands = socket.recv(4096).splitlines()
        assert commands[-1] == b'startSearch 0'
        socket.sendall(b'fm_seek_cmpl_cb\nfm_tune_cb 87500\n')
        tuner.loop.run()
        assert reply.finished and not reply.error
        assert tuner.wait_for_prop('Frequency',
                                   dbus.Int32(87500, variant_level=1))

        tuner.close()
