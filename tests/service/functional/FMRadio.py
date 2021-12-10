import dbus

from gi.repository import GLib

ServiceName = 'com.lomiri.FMRadioService'


class Error:
    ResourceError = 'com.lomiri.FMRadioService.Error.ResourceError'


class AsyncReply:
    def __init__(self, loop):
        self.loop = loop
        self.params = []
        self.error = None
        self.finished = False
    def on_reply(self, *params):
        self.params = params
        self.finished = True
        self.loop.quit()
    def on_error(self, error):
        self.error = error
        self.finished = True
        self.loop.quit()


def dbus_method(method):
    def run_async(*args, **kwargs):
        proxy = args[0]
        if kwargs.get('call_async', False):
            reply = AsyncReply(proxy.loop)
            method(*args,
                   reply_handler=reply.on_reply,
                   error_handler=reply.on_error)
            return reply
        return method(*args, **kwargs)
    return run_async


class Service(object):
    """ Client of the dbus interface of the FM radio service """

    def __init__(self, bus_obj):
        service_name = ServiceName
        object_path = '/com/lomiri/FMRadioService'
        self.interface_name = 'com.lomiri.FMRadioService'
        main_object = bus_obj.get_object(service_name, object_path,
                                         introspect=False)
        self.__service = dbus.Interface(main_object, self.interface_name)

    def open_tuner(self):
        return self.__service.OpenTuner()


class Tuner(object):
    """ Client of the dbus interface of the Tuner object """

    def __init__(self, bus_obj, object_path):
        service_name = ServiceName
        self.interface_name = 'com.lomiri.FMRadioService.Tuner'
        main_object = bus_obj.get_object(service_name, object_path,
                                         introspect=False)
        self.__tuner = dbus.Interface(main_object, self.interface_name)

        self.__properties = dbus.Interface(
            main_object,
            'org.freedesktop.DBus.Properties')
        self.props = {}
        self.__prop_callbacks = []
        self.signals = []
        self.__signal_callbacks = []

        self.loop = GLib.MainLoop()
        self.__properties.connect_to_signal('PropertiesChanged',
                                            self.__on_properties_changed)

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
        print('Signal emitted: ' + signal_name)
        self.signals.append([signal_name, *args])
        for cb in self.__signal_callbacks:
            cb(signal_name, *args)

    def clear_signals(self):
        self.signals = []

    def wait_for_signal(self, name, *args, timeout=3000):
        match = [name, *args]
        if match in self.signals:
            return True

        timer_id = GLib.timeout_add(timeout, self.loop.quit)
        def check_value(signal_name, *signal_args):
            if [name, *signal_args] == match:
                GLib.source_remove(timer_id)
                self.loop.quit()
        self.on_signal(check_value)
        self.loop.run()
        self.unsubscribe_signal(check_value)
        return True if match in self.signals else False

    def wait_for_prop(self, name, value, timeout=3000):
        if self.get_prop(name) == value:
            return True
        timer_id = GLib.timeout_add(timeout, self.loop.quit)
        def check_value(interface, changed, invalidated):
            if self.props.get(name) == value:
                GLib.source_remove(timer_id)
                self.loop.quit()
        self.on_properties_changed(check_value)
        self.loop.run()
        self.unsubscribe_properties_changed(check_value)
        return True if self.props.get(name) == value else False

    def set_prop(self, name, value, **kwargs):
        return self.__properties.Set(self.interface_name, name, value, **kwargs)

    def get_prop(self, name):
        return self.__properties.Get(self.interface_name, name)

    @dbus_method
    def search_forward(self, **kwargs):
        return self.__tuner.SearchForward(**kwargs)

    @dbus_method
    def search_backward(self, **kwargs):
        return self.__tuner.SearchBackward(**kwargs)

    @dbus_method
    def start(self, **kwargs):
        return self.__tuner.Start(**kwargs)

    @dbus_method
    def stop(self, **kwargs):
        return self.__tuner.Stop(**kwargs)

    @dbus_method
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
