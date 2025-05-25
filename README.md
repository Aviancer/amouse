# amouse

```
  __ _   _ __  ___ _  _ ___ ___ 
 / _` | | '  \/ _ \ || (_-</ -_)
 \__,_| |_|_|_\___/\_,_/__/\___====____
```

Anachro Mouse, a usb to serial mouse adapter.

![Picture of a black box with USB and serial connector](images/amouse-box.jpg?raw=true)

This software will convert USB mouse inputs to a selectable serial mouse protocol for retro hardware. This way you can use a Raspberry Pico, a Raspberry Pi or an existing modern PC/laptop to mouse on your retro system.

If you do feel a bit more adventurous and would like to build a stand-alone adapter device, I have included the code to do this using a Raspberry Pico microcontroller. A circuit diagram is provided under the `diagrams` folder.

The adapter has been tested to work with DOS, Windows 3.1, Windows 95 and 98 serial mouse drivers so far.

# New and changed

Notable changes for users, see more detailed changelogs at the releases.

Since v1.6.0:
- Console is now accessed with `backspace` rather than `enter` key to avoid Windows ATDT autodetect
- Saving settings to non-volatile memory, both for Pico an Linux builds

Since v1.5.0:
- Fixed a regression in initializing mouse with Windows environments
- Changed from FIFO to queue based multi-threading model for much improved reliability in serial transmission

# Features

Input aggregation from USB data rates to serial, for making the mouse response smooth and accurate.

On-the-fly sensitivity control and easy menu based configuration over serial terminal.
Emulated protocols:
- Wheeled Microsoft IntelliPoint mouse (Both DOS and Windows Plug and Play compatible).
- Generic two button Microsoft serial Mouse, without wheel.
- Logitech three button mouse.

All protocols work on DOS and Windows with the appropriate mouse driver or ctmouse.exe available from the FreeDOS project.

Since version 1.6.0 you are able to save your settings to non-volatily flash memory or disk so that they get automatically loaded when the adapter starts.

# Controls

## On-the-fly sensitivity control

Once the mouse is initialized - hold down both left and right mouse buttons. Then additionally:
- Scroll mouse wheel up to add sensitivity
- Scroll mouse wheel down to reduce sensitivity

One mouse wheel click adds/reduces sensitivity by a factor of 0.2, you can adjust sensitivity between 0.2 and 3.0.

## Serial console for configuration

<video src="https://user-images.githubusercontent.com/80006672/147396198-eb4ab52f-2a5a-4799-a7e4-8fb58865289f.mp4" controls="controls" style="max-width: 720px;">
</video>

A serial console feature is provided for more advanced configuration of the features of the mouse adapter, allowing for more than what is practical with just mouse button shortcuts. The serial console uses the same COM port used for acting as a serial adapter and you can toggle between the modes easily. This way the computer can be used to configure the adapter and no additional hardware is required, on Linux you can of course also use direct cli arguments.

Things you can configure (as of 1.6.0):
- Sensitivity
- Serial mouse protocol
- Swap left and right buttons
- Store settings in non-volatile memory (flash)

Open a serial terminal program (kermit, etc) to the same COM port you connected the mouse to, use the following settings:
```
1200 baud
7 data bits
No parity
1 stop bit
No hardware flow control
```

Press `backspace` (in some terminals `ctrl+backspace`) to bring up the serial console, the adapter will switch to configuration mode and a menu will open. If you get scrambled characters, check all your terminal settings.
For an example you can enter: `3 11<enter>` to change the mouse sensitivity to 11 (1.1 in the above sensitivity scale).

Enter the `exit command` to the serial console after you are done configuring amouse, returning it to adapter mode. If you changed protocols, you may need to re-initialize the OS mouse driver.

### Saving your options

You can save your preferred settings so that they will be automatically loaded when you power on the adapter. These settings will be written into the same non-volatile flash memory on the Pico as the program is written on, just at the end of the flash rather than the beginning where the program is written. Settings currently in use are not written to flash by before you tell the adapter to save them, instead safe defaults will be loaded. However once you have saved your settings, these settings will be used the next time the adapter is powered on

Navigate to `6) Read or write settings (Flash)` from the main menu. You can then:

1) Show help
2) Load settings from flash
3) Save settings to flash <--

If you make changes to your settings without saving, these will only remain in the memory until the adapter is power cycled. To update your settings you will need to save the settings to flash again. Using the on-the-fly sensitivity adjustment also changes the in-volatile-memory settings.

With the Linux build the settings will be written to the users home directory at `~/.amouse.conf` (if you run the program as root with sudo, this means `/root/.amouse.conf`), in the same binary format as the Pico build uses.

# Linux version

Tested to work also on a Raspberry Pi.

The Linux version can be found under the 'linux' subdirectory. I wrote this initially as a prototype before moving on to writing a Raspberry Pico implementation of the same, but have continued to keep it up to date also.

The mouse is by default captured in exclusive mode so you can use it without worrying about sending inputs to the system running it. Non-exclusive mode is also supported.

## Requirements
- A working C building environment.
- libevdev (tested with 1.11.0-1, probably works with other versions)
- Superuser privileges (simple) or access to raw mouse and serial devices (requires changing perms, exercise left up to reader).
- A serial port on the computer running amouse. You can get away with a cheap USB to Serial adapter.
- A retro PC or alike for fun or productivity, I recommend using ctmouse.exe driver under DOS. amouse works fine for Windows 3.1, 95 and 98 drivers also.

## Preferred 
- The serial port (retropc)RTS -> CTC(amouse) pin is used by the mouse driver at the other end to signal initializing a mouse, amouse catches this for timing and responds appropriately. However this can also be manually timed if there are no other options.

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

You can use the `-W` option to have the software write your current settings as the default settings when you run the software, the configuration will be written to `~/.amouse.conf` in the same binary format that is used to store the settings in flash for the stand-alone Pico adapter.

`amouse -h` will also print help and list of flags available.

# Raspberry Pico (RP2040) version

Provides a stand-alone adapter running on a Raspberry Pico microcontroller for converting from USB to Serial Mouse.

![Electronic schematic of the adapter](diagrams/amouse-schematic.png?raw=true)

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
- A suitable serial cable for connecting from the adapter to the computers serial port (see diagram)
- A way to provide 5v power to the Pico (a USB charger or battery could be ideal)

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

Provided that everything is connected up correctly, the adapter will auto-detect any mouse driver initialization from the PC and by default introduce itself as a Microsoft mouse. You can then use your USB mouse as a serial mouse.

# Planned future features

- [x] Storing settings like sensitivity and used mouse protocol in a non-volatile manner.
- [ ] Mouse Systems protocol.
- [ ] Graceful handling of reconnecting USB devices.
- [ ] DOS-native software for setting options without serial terminal program.

# FAQ

Q: Why Anachro Mouse?
A: A serial mouse is pretty anachronistic these days, but there's plenty of people who enjoy retro hardware and might find this rather useful. It also makes for a nice wordplay with 'a mouse' and 'anachromous'.

Q: Why a serial mouse?
A: Lots of people enjoy playing with retro PC hardware and I wanted there to be something that's easy with minimal hardware to use as a serial mouse while the latter are becoming increasingly rare or are breaking.
