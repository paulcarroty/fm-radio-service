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
