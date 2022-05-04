# Android Open Accessory proxy to connect Android devices to Linux based systems

Most Android devices support a USB protocol name [Android Open Accessory](https://source.android.com/devices/accessories/protocol).

This protocol allows Android devices to communicate via USB, while they are in USB accessory mode, instead of USB host mode. This allows you, to plug an Android device via a typical charging cable to the USB A port of a generic linux machine (Raspberry Pi, OpenWRT Router, arbitrary computer without its own screen) to advertise and open an App on the connected Android device.

# Goals

* Configure an embedded device such as a Raspberry Pi to configure its network and other services via an Android App
* Make embedded devices easier to configure
  * without HDMI cable + keyboard
  * without Serial/Debug cable
  * without a pre-configured SD card
  * just with the screen, you most likely carry around anyways: your (Android) phone or tablet
* Make this experience as beginner-friendly as possible
  * Easy to install, very small standard package
  * Eventually, make this package a default package in embedded distributions like
    * [Raspbian](https://www.raspbian.org/)
    * [OpenWRT](https://openwrt.org/)
  * Plug and Play
    * The device announces itself, when an Android Phone is connected
    * It advertises a URL, where to download a helpful App, directly via the System UI, before anything specific is installed.
      This makes it really easy to quickstart.
* Don't reinvent wheels. Use existing, safe and secure protocols
  * Connect to the SSH server
    * This is available virtually anywhere, especially when it also works with [DropBear](https://github.com/mkj/dropbear), or [tinysshd](https://github.com/janmojzis/tinyssh)
  * Alternatively to [Cockpit](https://cockpit-project.org/)
    * This might significantly reduce the amount of UI, I need to implement on the Android side, while being [very](https://cockpit-project.org/blog/creating-plugins-for-the-cockpit-user-interface.html) [extensible](https://cockpit-project.org/blog/cockpit-starter-kit.html) from the device side.
  * Maybe other OpenWRT [Web interfaces](https://openwrt.org/docs/guide-user/luci/webinterface.overview) like [LuCI](https://github.com/openwrt/luci/wiki/)
  * This significantly lowers the attack surface.
  * And the amount of code, I need to take care of.
* Work completely local. Don't require an additional internet connection (once the companion App is installed).


# Installation

The usual make, make install should suffice.
```
make
sudo make install
sudo systemctl daemon-reload
```

I plan to also provide packages (such as `deb`, `rpm` and OpenWRT `ipk`) to install via standard package managers.

## Deactivate automatic announcement

If you only want to have all the tools installed for manually starting things, but don't run it automatically, e.g. because you are developing on the machine, create a marker file.

```
sudo touch /etc/aoa-proxy_not_to_be_run
```

The udev rule checks for this file, and skips service instantiation, if it is present.


# Usage

Just plug an Android device. The system UI should display the manufacturer and model of the device, as well as its primary IP address.

## Manually start a service

Assuming you deactivated automatic announcement and your Android device is connected to USB bus 3, port 2, you can run:

```
systemctl start aoa-proxy-announce@3-2
systemctl start aoa-proxy-forward@3-2
```

For finding out, which port your device is connected to, use

```
lsusb -t
```

## Run the binary itself

All options are described in the help:

```
aoa-proxy --help
```

Bash completion is also available.

## Limitations

**The Android app is not yet ready**

However, the automatic announcement already works. Data sent via AOA will already be forwarded to SSH or Cockpit (depending on the first byte sent).

# QA
## What about Apple devices?

AOA is not supported by Apple. Maybe, someone could build something similar based on [PeerTalk](https://github.com/rsms/peertalk) or [libimobiledevice](https://libimobiledevice.org/).
