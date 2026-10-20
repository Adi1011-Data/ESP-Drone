# ESP-FLY (XIAO ESP32-C3)

A 1S brushed micro quad. The Seeed XIAO ESP32-C3 is the flight controller. A custom ESC board holds the MPU6050 and four MOSFET motor channels. You fly from the ESP-Drone phone app over the XIAO’s Wi-Fi. This tree is [ESP-FLY](https://github.com/Seeed-Projects/Co-Create_ESP-FLY) / [Circuit Digest ESP-Drone](https://github.com/Circuit-Digest/ESP-Drone) / Espressif ESP-Drone, retargeted from the ESP32-S3 to the ESP32-C3.

Read this page before soldering, installing tools, or flashing.

## What you are building

```text
1S LiPo  -->  ESC board  -->  four 720 coreless motors
                 |  MPU6050 (gyro + accel)
                 |  four AO3400A low-side FETs
                 |
           XIAO ESP32-C3  -->  Wi-Fi AP  -->  phone app (sticks)
                              HTTP :80     -->  laptop (PID page)
                              USB-C        -->  attitude viewer
```

The phone never closes the attitude loop. The chip reads the MPU6050, runs cascaded PID at 500 Hz, and mixes the four motors. If stick packets stop, throttle goes to zero.

This board has no barometer, magnetometer, or optical flow. Stabilize / manual flight only. Height-hold and position-hold need extra sensors that are not fitted.

## Simulation links

The drone must already be flashed and powered. From `simulation/`:

```powershell
.\serve.ps1
```

| Tool | Local (after serve.ps1) | On the drone Wi-Fi |
| --- | --- | --- |
| Landing page | http://127.0.0.1:8765/ | — |
| Attitude viewer | http://127.0.0.1:8765/viewer/ | USB or BLE from that page |
| PID tuner | http://127.0.0.1:8765/pid/ | http://192.168.43.42/ |

- **Attitude viewer:** props off. Close `idf.py monitor`. Chrome or Edge. Connect USB (or BLE) and tilt the board. The on-screen quad copies the MPU6050.
- **PID tuner:** phone stays on `ESP-DRONE_…` for sticks (UDP **2390**). Laptop joins the same AP. Open the PID link (HTTP **80**). Do not point a second CRTP client at 2390.

More detail: [simulation/README.md](simulation/README.md).

## Architecture

```text
Phone app  --UDP 2390 CRTP-->  Wi-Fi AP (192.168.43.42)
Laptop     --HTTP 80------->  same AP, PID dashboard
USB-C      --ATT,w,x,y,z-->  attitude viewer

Stick angles --> Angle PID --> desired rates --> Rate PID --> mixer --> MOT1..4
                 (outer)                        (inner, gyro)
```

| Piece | What it does |
| --- | --- |
| XIAO ESP32-C3 | Wi-Fi AP, USB Serial/JTAG, PID, PWM to the FETs |
| MPU6050 | Gyro and accelerometer on I2C `0x68` |
| AO3400A × 4 | One low-side switch per brushed motor |
| SS14 × 4 | Flyback diode on each motor |
| ESP-Drone 0.7.3 | Stick input over CRTP |
| PID page | Live Kp/Ki/Kd; Save writes NVS |

Factory PID: [Firmware/main/include/board/pid_config.h](Firmware/main/include/board/pid_config.h). Pins and Wi-Fi: [Firmware/main/include/board/drone_config.h](Firmware/main/include/board/drone_config.h).

Tune **Rate** (inner) until it is calm, then **Angle** (outer). Rate is “don’t wobble.” Angle is “follow the stick.”

## What to buy / make

### Flight stack

| Item | Notes |
| --- | --- |
| Seeed XIAO ESP32-C3 | Plugs into J1 and J3. USB-C toward 5V / D0. |
| This ESC PCB | [hardware/PCB/ESC](hardware/PCB/ESC). Gerbers in `Gerber/`. KiCad 10. |
| MPU-6050 (U1) | Already on the ESC schematic |
| AO3400A × 4 | Motor FETs |
| SS14 × 4 (SOD-323) | Flyback diodes |
| 720 coreless motors × 4 | Two CW, two CCW if the props require it |
| 1S LiPo (about 250–300 mAh) | BAT pads. Do not reverse the pack. |
| Props matched to the motors | Stay off until motors are checked on the bench |
| Frame | 50–80 mm whoop / printed frame that holds the board |
| USB-C **data** cable | Charge-only cables will not flash |

Full passives are in [hardware/PCB/ESC/Gerber/ESC-BOM.csv](hardware/PCB/ESC/Gerber/ESC-BOM.csv). Board photos and assembly: [hardware/PCB/ESC/README.md](hardware/PCB/ESC/README.md).

### Firmware pinout (what this tree actually flies)

Seat the XIAO with USB toward 5V / D0. D9 (BOOT) is not a motor.

| Mixer | Board | XIAO | GPIO |
| --- | --- | --- | --- |
| M1 front-left | MOT1 | D0 | GPIO2 |
| M2 front-right | MOT2 | D10 | GPIO10 |
| M3 rear-right | MOT3 | D1 | GPIO3 |
| M4 rear-left | MOT4 | D3 | GPIO5 |

MPU6050: SDA D4 / GPIO6, SCL D5 / GPIO7, INT D7 / GPIO20. Battery sense: D2 / GPIO4 (100k / 100k, firmware ×2).

D8 and D9 are strap pins. Leave D9 free for the BOOT button.

## Downloads and tools (do this before flashing)

Work on Windows. Scripts and paths in this repo assume the tools live on **E:** under `Drone-ESC\downloads`. Anaconda Python must not be the `python` that ESP-IDF finds.

1. **ESP-IDF 5.0.7 offline installer** (~0.96 GB)  
   Put `esp-idf-tools-setup-offline-5.0.7.exe` in `downloads/`.  
   Get it from Espressif’s IDF installer releases: [idf-installer](https://github.com/espressif/idf-installer/releases) (offline 5.0.7).

2. **Install onto this drive** (not `C:\Espressif`):

   ```powershell
   cd E:\Drone-ESC\downloads
   .\install-esp-idf-5.0.7.ps1
   ```

   In the installer: keep PowerShell selected, select the **ESP32-C3** target. Tools land in `downloads\Espressif`. The IDF tree is `downloads\Espressif\frameworks\esp-idf-v5.0.7`. The Python venv is `downloads\Espressif\python_env\idf5.0_py3.11_env`.

3. **Phone app** — [ESP-Drone 0.7.3 for Android](https://www.pgyer.com/a27L). Same app as ESP-FLY / Circuit Digest. Do not change drone IP `192.168.43.42` or UDP port `2390`.

4. **Browser** — Chrome or Edge for the attitude viewer (Web Serial / Web Bluetooth).

5. **Python 3** — only needed for `simulation\serve.ps1` (`python -m http.server`). The IDF venv is enough if it is on PATH.

6. **KiCad 10** — only if you will edit the PCB.

You do not need Node, Docker, or a second copy of ESP-IDF.

## Build

In an ESP-IDF PowerShell (installer shortcut, or `export.ps1` after pointing PATH at the 3.11 venv):

```powershell
$env:IDF_TOOLS_PATH = "E:\Drone-ESC\downloads\Espressif"
$env:IDF_PATH = "E:\Drone-ESC\downloads\Espressif\frameworks\esp-idf-v5.0.7"
$venv = "E:\Drone-ESC\downloads\Espressif\python_env\idf5.0_py3.11_env\Scripts"
$env:PATH = "$venv;" + $env:PATH
. "$env:IDF_PATH\export.ps1"

cd E:\Drone-ESC\ESP-FLY\Firmware
idf.py set-target esp32c3
idf.py build
```

The image is `Firmware\build\ESPDrone.bin`. Edit `drone_config.h` / `pid_config.h` only when the factory set should change, then rebuild.

## Flash

`idf.py flash` does **not** work on this XIAO. The USB port is the chip’s own Serial/JTAG (`VID 303A`, `PID 1001`). Use esptool with `usb_reset` as in [Firmware/FLASHING.md](Firmware/FLASHING.md).

Unplug the battery if props are on. In download mode the motor gates can turn on.

Find the COM port:

```powershell
Get-PnpDevice -PresentOnly | Where-Object { $_.InstanceId -match 'VID_303A' } | Select-Object Status, FriendlyName, InstanceId
```

Then, from `Firmware\build`, with `COMx` replaced:

```powershell
$py = "E:\Drone-ESC\downloads\Espressif\python_env\idf5.0_py3.11_env\Scripts\python.exe"
$esptool = "E:\Drone-ESC\downloads\Espressif\frameworks\esp-idf-v5.0.7\components\esptool_py\esptool\esptool.py"
& $py $esptool --chip esp32c3 -p COMx -b 460800 --before usb_reset --after hard_reset --connect-attempts 5 --no-stub write_flash --flash_mode dio --flash_freq 80m --flash_size 4MB 0x0 bootloader\bootloader.bin 0x8000 partition_table\partition-table.bin 0x10000 ESPDrone.bin
```

A good boot log: MPU6050 connected, `ESP-DRONE_…` AP, `ready to fly`. Warnings for baro / mag / flow are expected.

Fallback if `usb_reset` fails: hold BOOT, tap RESET, flash with `--before no_reset`. Details in FLASHING.md.

## Bring-up order (props off)

1. 3.3 V present, XIAO seated the right way around.
2. Flash, then USB serial: MPU6050 OK, AP up.
3. Attitude viewer: tilt matches the on-screen quad. Invert an axis in the page if needed; flip `DRONE_GYRO_*_SIGN` / `DRONE_ACC_*_SIGN` in firmware if the board is mechanically reversed.
4. One motor at a time, no props: each corner spins the right way. Swap that motor’s two leads if it is backward. If a corner climbs in a hover, that mixer channel is wrong — see comments in `drone_config.h`.
5. Phone joins `ESP-DRONE_<MAC>`, password `12345678`, app Connect. Throttle at zero, then a blip. Killing the app or leaving Wi-Fi must stop the motors.
6. Props on only after that. First hover in a soft, open space.
7. PID: laptop on the same AP, open http://192.168.43.42/ . Tune Rate, then Angle. **Save** to keep the tune across reboot.

## Fly

1. Power the drone and wait until it is sitting level.
2. Phone: Wi-Fi `ESP-DRONE_<MAC>`, password `12345678`.
3. ESP-Drone app → Connect. Sticks go to `192.168.43.42:2390`.

SSID prefix and password are in `drone_config.h`. Do not change the IP or UDP port; the 0.7.3 app expects them.

## Repo map

| Path | Contents |
| --- | --- |
| [Firmware](Firmware) | ESP-IDF 5.0.7 app |
| [Firmware/FLASHING.md](Firmware/FLASHING.md) | How this XIAO is actually programmed |
| [simulation](simulation) | Attitude viewer and PID tuner |
| [hardware/PCB/ESC](hardware/PCB/ESC) | Schematic, PCB, Gerbers, BOM |

Upstream: [Max Imagination’s ESP-FLY video](https://youtu.be/V_mZsiZcy7s). This board is C3, not S3.
