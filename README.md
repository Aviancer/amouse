# amouse

```
  __ _   _ __  ___ _  _ ___ ___ 
 / _` | | '  \/ _ \ || (_-</ -_)
 \__,_| |_|_|_\___/\_,_/__/\___====____
```
Anachro Mouse, a usb to serial mouse adaptor.

This program will convert USB mouse inputs to serial (Microsoft) mouse protocol for retro hardware. This way you can use an existing modern PC/laptop or perhaps even a Raspberry Pi to mouse on your retro system.

If you feel a bit more adventurous and would like to build a stand-alone adaptor device, I have also designed a board and written code to do this using a Raspberry Pico microcontroller (To be published).

The adaptor has been tested to work against DOS and Windows 95 serial mouse drivers so far.

# Linux version

A version usable under Linux can be found under the 'linux' subdirectory.

I wrote this as a prototype before moving on to writing a Raspberry Pico implementation of the same.

## Requirements
- A working C building environment.
- libevdev (tested with 1.11.0-1, probably works with other versions)
- Superuser privileges (simple) or access to raw mouse and serial devices (requires changing perms, exercise left up to reader).
- A serial port on the computer running amouse. You can get away with a cheap USB to Serial adaptor.
- A retro PC or alike for fun or productivity, I recommend using ctmouse.exe driver under DOS. amouse itself works fine for Windows 95, et al also.

## Preferred 
- The serial port (retropc)RTS -> CTC(amouse) pin is used by the mouse driver at the other end to signal initializing a mouse, amouse catches this for timing and responds appropriately. However this can also be manually timed.

## Build & install
```
cd linux
make
make install
```

make install will copy the binary to `/usr/local/bin` by default.

## Usage example

```
sudo amouse -m /dev/input/by-id/usb-<yourdevice>-event-mouse -s /dev/ttyUSB0 -d
```

Where ttyUSB0 in this case is for example a USB serial port adapter, naturally you can use an actual serial port as well if your computer features one.
Note that evdev requires access to a -event- generating device, just picking any `/dev/input` mouse device will not work.

You can try the following for finding the device file for your mouse:
`ls /dev/input/by-id/*event-mouse*`

If you are unable to find your mouse under there, you may have to look try out the various `/dev/input/event*` files instead.
The following may also provide some pointers for figuring out a `/dev/input/event*` number: `grep -H '' /sys/class/input/*/name`

If your serial cable/adaptor isn't fully pinned (missing a CTS pin), you may use the `-i` (immediate ident) option bypass the automatic handling. In this case you will need to manually time it and launch the mouse driver and amouse at the same time. The timing can be pretty tight and require multiple attempts.

`amouse -h` will also print help and list of flags available. 

# Raspberry Pico (RP2040) version

I've also written a version capable of running stand-alone on the RP2040 and it will be added later.

# FAQ

Q: Why Anachro Mouse?
A: A serial mouse is pretty anachronistic these days, but there's plenty of people who enjoy retro hardware and might find this rather useful. It also makes for a nice wordplay with 'a mouse' and 'anachromous'.

Q: Why a serial mouse?
A: Lots of people enjoy playing with retro PC hardware and I wanted there to be something that's easy with minimal hardware to use as a serial mouse while the latter are becoming increasingly rare or break.

Q: Anachromous isn't a word.
A: Neither is that a question. However it sounds like it should be a word.
