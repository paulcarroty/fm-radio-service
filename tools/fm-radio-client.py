#! /usr/bin/env python3

import dbus
import dbus.mainloop.glib

from gi.repository import GLib

ServiceName = 'com.lomiri.FMRadioService'


class Error:
    ResourceError = 'com.lomiri.FMRadioService.Error.ResourceError'


class Service(object):
    """ Client of the dbus interface of the FM radio service """

    def __init__(self, bus_obj):
        service_name = ServiceName
        object_path = '/com/lomiri/FMRadioService'
        self.interface_name = 'com.lomiri.FMRadioService'
        main_object = bus_obj.get_object(service_name, object_path)
        self.__service = dbus.Interface(main_object, self.interface_name)

    def open_tuner(self):
        return self.__service.OpenTuner()


class Tuner(object):
    """ Client of the dbus interface of the Tuner object """

    class State:
        Stopped = 0
        Active = 1


    def __init__(self, bus_obj, object_path):
        service_name = ServiceName
        self.interface_name = 'com.lomiri.FMRadioService.Tuner'
        main_object = bus_obj.get_object(service_name, object_path,
                                         introspect=True)
        self.__tuner = dbus.Interface(main_object, self.interface_name)

        self.__properties = dbus.Interface(
            main_object,
            'org.freedesktop.DBus.Properties')
        self.props = {}
        self.__prop_callbacks = []
        self.__signal_callbacks = []

        self.__properties.connect_to_signal('PropertiesChanged',
                                            self.__on_properties_changed)
        self.props = self.__properties.GetAll(self.interface_name)

        self.__tuner.connect_to_signal(
                'StationFound',
                lambda f, s: self.__on_signal('StationFound', f, s))
        self.__tuner.connect_to_signal(
                'Error',
                lambda c, m: self.__on_signal('Error', c, m))

    def __on_properties_changed(self, interface, changed, invalidated):
        assert interface == self.interface_name
        self.props.update(changed)
        for cb in self.__prop_callbacks:
            cb(interface, changed, invalidated)

    def __on_signal(self, signal_name, *args):
        for cb in self.__signal_callbacks:
            cb(signal_name, *args)

    def set_prop(self, name, value, **kwargs):
        return self.__properties.Set(self.interface_name, name, value,
                                     **kwargs)

    def get_prop(self, name):
        return self.__properties.Get(self.interface_name, name)

    def search_forward(self):
        return self.__tuner.SearchForward()

    def search_backward(self):
        return self.__tuner.SearchBackward()

    def start(self):
        return self.__tuner.Start()

    def stop(self):
        return self.__tuner.Stop()

    def close(self):
        return self.__tuner.Close()

    def on_properties_changed(self, callback):
        self.__prop_callbacks.append(callback)

    def unsubscribe_properties_changed(self, callback):
        self.__prop_callbacks.remove(callback)

    def on_signal(self, callback):
        self.__signal_callbacks.append(callback)

    def unsubscribe_signal(self, callback):
        self.__signal_callbacks.remove(callback)


class State:
    Disconnected = 0
    Connected = 1
    Playing = 2


class Command(object):
    def __init__(self, text, action, states):
        self.text = text
        self.action = action
        self.states = states


class Client(object):
    def __init__(self):
        dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
        self.state = State.Disconnected
        self.loop = GLib.MainLoop()
        all_states = [State.Disconnected, State.Connected, State.Playing]
        self.all_commands = [
                Command('Quit', self.quit, all_states),
                Command('Close', self.close, [State.Connected, State.Playing]),
                Command('Open tuner', self.open_tuner, [State.Disconnected]),
                Command('Set frequency', self.set_frequency,
                        [State.Connected, State.Playing]),
                Command('Set volume', self.set_volume,
                        [State.Connected, State.Playing]),
                Command('Scan forward', self.scan_forward,
                        [State.Connected, State.Playing]),
                Command('Scan backward', self.scan_backward,
                        [State.Connected, State.Playing]),
                Command('Start playback', self.start, [State.Connected]),
                Command('Stop playback', self.stop, [State.Playing]),
        ]
        self.tuner = None

    def on_properties_changed(self, interface, changed, invalidated):
        print('\nProperties changed: {}\n'.format(changed))

    def quit(self):
        self.loop.quit()

    def open_tuner(self):
        bus = dbus.SessionBus()
        try:
            service = Service(bus)
        except dbus.exceptions.DBusException as e:
            print('Failed to connect to service:\n  {}'.format(e))
            return
        tuner_path = service.open_tuner()
        self.tuner = Tuner(bus, tuner_path)
        self.tuner.on_properties_changed(self.on_properties_changed)
        self.state = State.Connected

    def update_state(self):
        if self.tuner:
            tuner_state = self.tuner.get_prop('State')
            self.state = State.Playing if tuner_state == Tuner.State.Active \
                    else State.Connected

    def set_frequency(self):
        frequency = -1
        while frequency == -1:
            try:
                frequency = int(input('Specify desired frequency ' +
                                      '(0 to go back):'))
            except (EOFError, ValueError):
                frequency = -1
        if frequency != 0:
            self.tuner.set_prop('Frequency', frequency)

    def set_volume(self):
        volume = -2
        while volume == -2:
            try:
                volume = int(input('Specify desired volume ' +
                                   '(-1 to go back):'))
            except (EOFError, ValueError):
                volume = -2
        if volume >= 0:
            self.tuner.set_prop('Volume', volume)

    def scan_forward(self):
        self.tuner.search_forward()

    def scan_backward(self):
        self.tuner.search_backward()

    def start(self):
        self.tuner.start()

    def stop(self):
        self.tuner.stop()

    def close(self):
        self.tuner.close()
        self.tuner = None
        self.state = State.Disconnected

    def commands_for_state(self, state):
        return [c for c in self.all_commands if state in c.states]

    def pick_action(self):
        self.update_state()
        commands = self.commands_for_state(self.state)

        for i, command in enumerate(commands):
            print('{}) {}'.format(i + 1, command.text))

        action_index = 0
        while action_index <= 0 or action_index > len(commands):
            try:
                action_index = int(input('Choose an action: '))
            except EOFError:
                return self.quit()
            except ValueError:
                action_index = 0

        command = commands[action_index - 1]
        command.action()
        print('\n')
        return True

    def run(self):
        GLib.idle_add(self.pick_action)
        self.loop.run()


if __name__ == "__main__":
    client = Client()
    client.run()
