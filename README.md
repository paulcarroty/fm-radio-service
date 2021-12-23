FM radio service
================

Enable the FM radio on your Ubuntu Touch device. Supported hardware:

- Some Mediatek devices
- Some Qualcomm devices:
  - Redmi Note 7 Pro

Please continue reading to learn how to find out if your device is supported.


#### Mediatek devices

Mediatek devices capable of FM radio receiving should expose a device named
`/dev/fd`. In order for the tuner to work, you also need to have the proper
kernel drivers and firmware; but the presence of this device file should be
enough to get you started.

In order to hear the radio stream, your device must also expose a pulseaudio
source for the FM tuner. If the command

    pactl list sources | grep tuner

shows that a port named `input-fm_tuner` is available, then you are ready to
go. If not, it's likely that enabling this source will require some more work
for the device porter.


#### Qualcomm devices

At the moment, only the `cherokee` radio chips are supported: if the command

    getprop vendor.bluetooth.soc

prints `cherokee`, then we are on the good path. However, there are several
other checks that we need to perform:

    ls -l /vendor/lib64/libfm-hci.so
    ls -l /vendor/lib64/fm_helium.so
    ls -l /vendor/bin/fm-bridge

All these files need to exist. If they don't, then the device porter needs to
modify the vendor image to include them. While one could modify the binary
vendor image and dump the three binaries in there (by mounting the image over a
loopback device), a cleaner solution seems to be rebuilding the vendor image
and enabling these packages. For an example of this approach, see the [changes
that were made for the Redmi Note 7
Pro](https://gitlab.com/ubuntu-touch-xiaomi-violet/android_vendor_xiaomi_violet/-/commit/f6af0fe8576864d1f393ae14b1b6aeea4fdf0f7f)
and the [notes on how to build the vendor
image](https://gitlab.com/ubports/community-ports/android9/xiaomi-redmi-note-7-pro/xiaomi-violet#building-the-vendor-image).


## Checking if FM radio works

Install the `fm-radio-service` and `fm-radio-tools` packages from [this Jenkins
page](https://ci.ubports.com/job/UBportsCore/job/FM%20radio%20service/view/change-requests/job/MR-2/lastSuccessfulBuild/artifact/)
(make sure you pick the correct architecture: in most cases you'll want the
`arm64` variant) on your device, and then open three shells:

1. One one shell, run `tail -f /var/log/syslog` (this is especially important
   to find if there are issues with the kernel driver or if the firmware is not
   found);
2. On the second one, run
   ```
   FM_RADIO_SERVICE_TIMEOUT=300 fm-radio-service
   ```
3. On the third one, run
   ```
   fm-radio-client.py
   ```
   then type the number corresponding to the "Open tuner" command, and then
   "Start playback". You can then activate the commands to change frequency
   (pick numbers in the range from 87500 to 108000); the commands to search
   forward and backward might or might not work.
