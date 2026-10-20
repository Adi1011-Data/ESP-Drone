# Simulation tools

Local pages for the XIAO. From this folder:

```powershell
.\serve.ps1
```

That opens the landing page. The two tools:

| Tool | Local | On the drone Wi-Fi |
| --- | --- | --- |
| Attitude viewer | http://127.0.0.1:8765/viewer/ | USB or BLE from that page |
| PID tuner | http://127.0.0.1:8765/pid/ | http://192.168.43.42/ |

Landing page: http://127.0.0.1:8765/

Project getting started: [../README.md](../README.md).

## PID tuner

Live attitude/rate gains while the phone app keeps flying.

1. Phone joins `ESP-DRONE_<MAC>` and connects in the ESP-Drone app (UDP 2390). Leave it connected.
2. Laptop joins the same Wi-Fi.
3. Open http://192.168.43.42/ on the drone, or http://127.0.0.1:8765/pid/ from `serve.ps1`.
4. Drag Kp/Ki/Kd. Apply is live; no reset. **Save** keeps the tune across reboot.

Do not send CRTP from the laptop. Port 2390 is the phone stick link. The dashboard uses HTTP port 80.

Factory defaults are `Firmware/main/include/board/pid_config.h`.

## Attitude viewer

Tilt the XIAO and the quad on screen copies the MPU6050.

The flight firmware sends one line over USB-C, 100 times a second:

```text
ATT,<w>,<x>,<y>,<z>
```

The page keeps the newest line and drops the rest, so boot text cannot pile up into lag.

## Run the viewer

1. Props off.
2. Close `idf.py monitor` if it is open. Windows allows one program on the COM port.
3. From this folder:

```powershell
.\serve.ps1
```

4. Open **Attitude viewer**. Microsoft Edge or Chrome is required (they can open a serial port).
5. Click **Connect USB** and choose the XIAO's COM port.

Until packets arrive, the quad plays a preview motion so you can see the window working. After a flash of the updated firmware, tilting the board takes over.

If one axis goes the opposite way, tick **Invert roll**, **Invert pitch**, or **Invert yaw**. The choice is remembered in the browser.

Drag the view to orbit. Scroll to zoom. Orange and yellow motors are the nose.

## Firmware

`DRONE_USB_ATTITUDE_HZ` in `Firmware/main/include/board/drone_config.h` sets the rate. `0` turns the stream off.

Rebuild and flash from `Firmware`:

```powershell
idf.py build
idf.py -p COMx flash
```

Yaw still drifts slowly. This MPU6050 has no magnetometer. Pitch and roll track the board.
