# ESP32-C3 firmware

Flight firmware for the XIAO ESP32-C3 ESC board. Full build, parts, and first-flight notes: [../README.md](../README.md). Flash method: [FLASHING.md](FLASHING.md).

It is the [ESP-FLY](https://github.com/Seeed-Projects/Co-Create_ESP-FLY) tree (itself a tested modification of [Circuit Digest ESP-Drone](https://github.com/Circuit-Digest/ESP-Drone) and Espressif ESP-Drone), retargeted from the ESP32-S3 in [Max Imagination's build video](https://youtu.be/V_mZsiZcy7s) to the ESP32-C3.

## Simulation links

Laptop on the drone Wi-Fi, or `..\simulation\serve.ps1` on this PC.

| Tool | Local | On the drone |
| --- | --- | --- |
| Attitude viewer | http://127.0.0.1:8765/viewer/ | USB/BLE from that page |
| PID tuner | http://127.0.0.1:8765/pid/ | http://192.168.43.42/ |

Phone keeps UDP **2390** for sticks. The PID page is HTTP **80**. Do not put a second CRTP client on 2390.

## Fly

The phone app is [ESP-Drone 0.7.3 for Android](https://www.pgyer.com/a27L).

1. Power the drone and wait until it is sitting level.
2. On the phone, join Wi-Fi `ESP-DRONE_<MAC>` with password `12345678`.
3. Open the app and tap connect. The app sends stick data to `192.168.43.42` port `2390`.

Do not change that address or port. The 0.7.3 app expects them. The password and SSID prefix are in `drone_config.h`.

## PID

Factory attitude gains are in `main/include/board/pid_config.h`. Live tuning does not need a reflash.

1. Phone joins `ESP-DRONE_<MAC>` and flies with the app (UDP 2390).
2. Laptop joins the same access point.
3. Open http://192.168.43.42/ or http://127.0.0.1:8765/pid/ .
4. Drag a gain. The next control loop uses it. **Save** writes flash so it survives reboot. **Restore defaults** reloads `pid_config.h`.

Tune Rate (inner) first, then Angle (outer).

## What to edit

Pins, battery scale, axis signs, and the Wi-Fi password:

`main/include/board/drone_config.h`

Attitude PID factory defaults:

`main/include/board/pid_config.h`

Change those files and rebuild only when the factory set should change. Menuconfig is not required for those values.

## Motor gates

| Mixer | Board | XIAO | GPIO |
| --- | --- | --- | --- |
| M1 front-left | MOT1 | D0 | GPIO2 |
| M2 front-right | MOT2 | D10 | GPIO10 |
| M3 rear-right | MOT3 | D1 | GPIO3 |
| M4 rear-left | MOT4 | D3 | GPIO5 |

MPU6050: SDA GPIO6 (D4), SCL GPIO7 (D5), interrupt GPIO20 (D7), address 0x68.

Battery sense: GPIO4 (D2), ADC1 channel 4. R11 and R12 are 100k, so the firmware multiplies the ADC voltage by 2.

D9 (GPIO9) is the BOOT strap. Leave it unconnected so the button can put the chip into download mode. D8 (GPIO8) is also a strap pin and is not used.

## Build

ESP-IDF 5.0.7. Put the offline installer in `downloads/` and run `downloads\install-esp-idf-5.0.7.ps1` before the first build. Use the IDF Python 3.11 venv, not Anaconda. See [../README.md](../README.md).

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

Flash with `usb_reset` as in [FLASHING.md](FLASHING.md). `idf.py flash` fails on this XIAO (`No serial data received`).

A good boot log shows the MPU6050 connected, the `ESP-DRONE_...` access point, and `ready to fly`. Warnings for a barometer, magnetometer, or flow sensor are expected.

Keep the propellers off until each motor has been checked on the bench.
