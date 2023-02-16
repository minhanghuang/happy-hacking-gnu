# happy-hacking-gnu
A free, open-source alternative to the HHKB Keymap Tool provided by PFU.

| Features                    | Supported | Planned |
|-----------------------------|:---------:|--------:|
| Remap individual keys       |  Yes      |  -      |
| Read keyboard information   |  Yes      |  -      |
| Reset factory defaults      |  Yes      |  -      |
| Dump firmware               |  Yes      |  -      |
| Update firmware             |  Yes      |  -      |

## Building
The only build dependencies are `cmake` and `libudev`, all other dependencies are included in the source tree. 

Install dependencies on Fedora-based distros:
```
# dnf install gcc cmake systemd-devel
```

Install dependencies on Debian-based distros:
```
# apt install gcc cmake libudev-dev
```

Building is as simple as:
```bash
$ git clone --recursive https://gitlab.com/dom/happy-hacking-gnu.git
$ cd happy-hacking-gnu/bin && cmake ..
$ make
```

## Usage
Due to udev rules, this program will fail with a `Permission denied` error on most distributions by default.

There are two options to fix this:
* Run `hhg` as root
* Create a udev rule to allow hidraw access to local users. You can do this by running the following command as root: 

```
echo 'KERNEL=="hidraw*", ATTRS{idVendor}=="04fe", TAG+="uaccess"' > /etc/udev/rules.d/01-hhkb-hid.rules && udevadm control --reload-rules && udevadm trigger
```

## Options
```
Usage: hhg [options] [[--] args]

    -h, --help                show this help message and exit
    -v, --verbose             show debug messages

Basic options
    -i, --info                print keyboard information
    -d, --dip                 print dipswitch state
    -m, --mode                print keyboard mode
    -k, --keymap              print current keymap
    -f, --factory-reset       reset to factory defaults

Keymapping options
    --remap-key=<int>         key number to remap
    --scancode=<int>          hid scancode to map
    --fn                      operate on function layer

Firmware options
    --flash-firmware=<str>    flash firmware from file
    --dump-firmware           save current firmware to file
```
## Remapping guide

**Note:** It is possible to use scancodes that are not in the official software, but there are a few limitations. Most notably, unsupported media keys will act up if assigned to most keys, since the HHKB usually uses a separate USB interface to send these.

To see available keys and their numbers, run `hhg --keymap`:
```
----------------------------------------------------------------------------
| 60 | 59 | 58 | 57 | 56 | 55 | 54 | 53 | 52 | 51 | 50 | 49 | 48 | 47 | 46 |
| 29 | 1e | 1f | 20 | 21 | 22 | 23 | 24 | 25 | 26 | 27 | 2d | 2e | 31 | 35 |
----------------------------------------------------------------------------
|  45  | 44 | 43 | 42 | 41 | 40 | 39 | 38 | 37 | 36 | 35 | 34 | 33 |  32   |
|  2b  | 14 | 1a | 08 | 15 | 17 | 1c | 18 | 0c | 12 | 13 | 2f | 30 |  2a   |
----------------------------------------------------------------------------
|  31   | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 20 |    19     |
|  e0   | 04 | 16 | 07 | 09 | 0a | 0b | 0d | 0e | 0f | 33 | 34 |    28     |
----------------------------------------------------------------------------
|   18     | 17 | 16 | 15 | 14 | 13 | 12 | 11 | 10 | 09 | 08 |   07   | 06 |
|   e1     | 1d | 1b | 06 | 19 | 05 | 11 | 10 | 36 | 37 | 38 |   e5   | 01 |
----------------------------------------------------------------------------
        | 05 |  04   |               03               |  02   | 01 |
        | e3 |  e2   |               2c               |  e6   | e7 |
        ------------------------------------------------------------
```
The first number on each key is what you should pass to `--remap-key`.

You can find all [USB HID scan codes here](https://gist.github.com/MightyPork/6da26e382a7ad91b5496ee55fdc73db2), which should be passed to `--scancode`.

For example, to assign Print Screen to the Z key on your function layer, you would run:
```
hhg --remap-key 17 --scancode 0x46 --fn
```

## License

[The Unlicense](https://unlicense.org/)
