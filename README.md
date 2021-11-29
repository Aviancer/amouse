# amouse

```
  __ _   _ __  ___ _  _ ___ ___ 
 / _` | | '  \/ _ \ || (_-</ -_)
 \__,_| |_|_|_\___/\_,_/__/\___====____
```
Anachro Mouse, a usb to serial mouse adapter.

This software will convert USB mouse inputs to Microsoft serial mouse protocol for retro hardware. This way you can use a Raspberry Pico, a Raspberry Pi or an existing modern PC/laptop to mouse on your retro system.

If you do feel a bit more adventurous and would like to build a stand-alone adapter device, I have included the code to do this using a Raspberry Pico microcontroller. A circuit diagram is provided under the `diagrams` folder.

The adapter has been tested to work against DOS and Windows 95 serial mouse drivers so far.

# Controls

## Sensitivity control

Once the mouse is initialized - hold down both left and right mouse buttons. Then additionally:
- Scroll mouse wheel up to add sensitivity
- Scroll mouse wheel down to reduce sensitivity
One mouse wheel click adds/reduces sensitivity by a factor of 0.2, you can adjust sensitivity between 0.2 and 2.0. 

# Linux version

Tested to work also on a Raspberry Pi.

The Linux version can be found under the 'linux' subdirectory. I wrote this as a prototype before moving on to writing a Raspberry Pico implementation of the same.

The mouse is by default captured in exclusive mode so you can use it without worrying about sending inputs to the system running it. Non-exclusive mode is also supported.

## Requirements
- A working C building environment.
- libevdev (tested with 1.11.0-1, probably works with other versions)
- Superuser privileges (simple) or access to raw mouse and serial devices (requires changing perms, exercise left up to reader).
- A serial port on the computer running amouse. You can get away with a cheap USB to Serial adapter.
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
sudo amouse -m /dev/input/by-id/usb-<yourdevice>-event-mouse -s /dev/ttyUSB0 
```

Where ttyUSB0 in this case is for example a USB serial port adapter, naturally you can use an actual serial port as well if your computer features one.
Note that evdev requires access to a -event- generating device, just picking any `/dev/input` mouse device will not work.

You can try the following for finding the device file for your mouse:
`ls /dev/input/by-id/*event-mouse*`

If you are unable to find your mouse under there, you may have to look try out the various `/dev/input/event*` files instead.
The following may also provide some pointers for figuring out a `/dev/input/event*` number: `grep -H '' /sys/class/input/*/name`

If your serial cable/adapter isn't fully pinned (missing a CTS pin), you may use the `-i` (immediate ident) option bypass the automatic handling. In this case you will need to manually time it and launch the mouse driver and amouse at the same time. The timing can be pretty tight and require multiple attempts.

`amouse -h` will also print help and list of flags available. 

# Raspberry Pico (RP2040) version

Provides a stand-alone adapter running on a Raspberry Pico microcontroller for converting from USB to Serial Mouse.

Currently supports emulating a wheeled Microsoft Mouse, with all three buttons and wheel working.

## Requirements
- Basic soldering skills
- Working C building environment
- cmake version > 3.12
- Raspberry Pico SDK (Should auto-pull from git on cmake)

### Components
- A Raspberry Pico microcontroller
- One DIP MAX3232 chip or compatible, with 1x TX + 2x RX pins available.
- 5x 0.1uF capacitors (charge pumps for MAX3232, cbypass, etc)
- Serial header with RX, TX and CTS available
- USB-A header to solder on, or alternatively a USB micro to USB-A host adapter for connecting a USB mouse to the Pico USB port
- Few bits of wire/board to make connections
- A suitable serial cable for connecting from the adapter to the computers serial port

## Build & install
```
cd pico/build
cmake ..
make
```

This will build `amouse.uf2` which can be flashed onto a Raspberry Pico.

To enter flashing mode with Raspberry Pico by holding down the small white button while connecting it to a USB port. Then simply copy `amouse.uf2` onto the Pico USB drive.

See `diagrams` directory for how to wire the Pico correctly to talk to a serial port.

## Usage example

- Connect your USB mouse to the adapter
- Connect the adapters serial port to the computers serial port
- Provide the Pico with 5v power (and appropriate grounding, etc)
- Run a mouse driver/start a serial mouse using OS on the computer

Provided that everything is connected up correctly, the adapter will auto-detect any mouse driver initialization from the PC and introduce itself as a Microsoft mouse. You can then use your USB mouse as a serial mouse.

# Planned future features

- Switching between at least wheeled and non-wheeled Microsoft mouse protocols (Already implemented in Linux version)
- Storing settings like sensitivity and used mouse protocol in a non-volatile manner.

# FAQ

Q: Why Anachro Mouse?
A: A serial mouse is pretty anachronistic these days, but there's plenty of people who enjoy retro hardware and might find this rather useful. It also makes for a nice wordplay with 'a mouse' and 'anachromous'.

Q: Why a serial mouse?
A: Lots of people enjoy playing with retro PC hardware and I wanted there to be something that's easy with minimal hardware to use as a serial mouse while the latter are becoming increasingly rare or are breaking.

Q: Anachromous isn't a word.
A: Neither is that a question. However it sounds like it should be a word.
