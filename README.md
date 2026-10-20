# ESP-Drone (XIAO ESP32-C3)

A 1S brushed micro quad. The Seeed XIAO ESP32-C3 is the flight controller. A custom ESC board holds the MPU6050 and four MOSFET motor channels. You fly from the ESP-Drone phone app over the XIAO’s Wi-Fi. This tree is [Circuit Digest ESP-Drone](https://github.com/Circuit-Digest/ESP-Drone) / Espressif ESP-Drone, retargeted from the ESP32-S3 to the ESP32-C3.

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

3. **Phone app** — [ESP-Drone 0.7.3 for Android](https://www.pgyer.com/a27L). Do not change drone IP `192.168.43.42` or UDP port `2390`.

4. **Browser** — Chrome or Edge for the attitude viewer (Web Serial / Web Bluetooth).

5. **Python 3** — only needed for `simulation\serve.ps1` (`python -m http.server`). The IDF venv is enough if it is on PATH.

6. **KiCad 10** — only if you will edit the PCB.

You do not need Node, Docker, or a second copy of ESP-IDF.

## Build and flash

From the repo root, in an ESP-IDF 5.0.7 shell (see the installer notes above):

```powershell
cd Firmware
idf.py set-target esp32c3
idf.py build
```

The image is `Firmware\build\ESPDrone.bin`. `idf.py flash` does not work on this XIAO. Flash with esptool and `usb_reset`, as in [Firmware/FLASHING.md](Firmware/FLASHING.md). Unplug the battery if props are on.

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

Upstream video: [youtu.be/V_mZsiZcy7s](https://youtu.be/V_mZsiZcy7s). This board is C3, not S3.
