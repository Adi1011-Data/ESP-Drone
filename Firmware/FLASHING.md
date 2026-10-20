# Flashing the XIAO ESP32-C3

This board is a Seeed XIAO ESP32-C3. Its USB port is the chip's own USB Serial/JTAG device (VID `303A`, PID `1001`), not a separate USB-UART chip. The flash method has to match that.

`idf.py flash` and esptool `--before default_reset` do **not** work here. They were tried many times and failed with `No serial data received`.

## What works

Use esptool directly with `--before usb_reset` and `--no-stub`. This entered download mode and completed a verified flash with no BOOT button held. A later flash of the motor hold-off image used the same command and also verified.

From `E:\Drone-ESC\ESP-FLY\Firmware\build`, after `idf.py build`:

```powershell
$py = "E:\Drone-ESC\downloads\Espressif\python_env\idf5.0_py3.11_env\Scripts\python.exe"
$esptool = "E:\Drone-ESC\downloads\Espressif\frameworks\esp-idf-v5.0.7\components\esptool_py\esptool\esptool.py"
& $py $esptool --chip esp32c3 -p COM17 -b 460800 --before usb_reset --after hard_reset --connect-attempts 5 --no-stub write_flash --flash_mode dio --flash_freq 80m --flash_size 4MB 0x0 bootloader\bootloader.bin 0x8000 partition_table\partition-table.bin 0x10000 ESPDrone.bin
```

Success looks like `Hash of data verified` for the bootloader (`0x0`), the partition table (`0x8000`), and the app (`0x10000`), then `Hard resetting via RTS pin...` and exit code 0.

Confirm the port first. Do not assume it is still COM17. The device is `USB\VID_303A&PID_1001`. The serial interface is `USB Serial Device (COMx)`. This board's USB serial number is `14:63:93:BE:FC:EC`.

```powershell
Get-PnpDevice -PresentOnly | Where-Object { $_.InstanceId -match 'VID_303A' } | Select-Object Status, FriendlyName, InstanceId
```

If that port name is not COM17, change `-p` to match it.

## Why the other resets fail

| Attempt | Result |
| --- | --- |
| `idf.py -p COM17 flash` | Opens the port, then `Failed to connect to ESP32-C3: No serial data received`. |
| esptool `--before default_reset` | Same failure. This is what `idf.py flash` uses. |
| Manual DTR/RTS "normal" reset | Chip prints `rst:0x15 (USB_UART_CHIP_RESET),boot:0x8 (SPI_FAST_FLASH_BOOT)` and starts the app. |
| Holding BOOT while the computer sends `default_reset` | Still `boot:0x8`. That reset does not sample the BOOT pin the way a real reset button does. |
| `--before no_reset` while the app is running | Write timeout or no sync. The ROM downloader is not running. |

`usb_reset` is esptool's `USBJTAGSerialReset`. On PID `0x1001` it asks the USB Serial/JTAG block for download mode. `default_reset` only restarts the app, and the USB port drops while the handle is still open, so the read comes back empty.

`--no-stub` is required for the full write. A stub upload happened to work for `chip_id`, but every successful full flash used `--no-stub`.

## Fallback that also worked

If `--before usb_reset` fails, the chip can be put in the ROM downloader by hand:

1. Hold the XIAO **BOOT** button (it is GPIO9 / D9, and it cannot be moved).
2. Tap **RESET** once.
3. Keep holding **BOOT** until the write starts, then let go.
4. Flash with `--before no_reset` so esptool does not reset the chip again.

Opening the COM port with DTR and RTS at their default (both asserted) resets the chip out of download mode. If a script must open the port first, set `dtr = False` and `rts = False` before `open()`.

This fallback succeeded when the chip was already sitting in the downloader (`LAST no-boot-line`, then `Connecting...`, then a verified write). It is the backup, not the normal path.

## Build, then flash

ESP-IDF 5.0.7 lives on E:. Anaconda Python must not be the `python` that `export.ps1` finds.

```powershell
$env:IDF_TOOLS_PATH = "E:\Drone-ESC\downloads\Espressif"
$env:IDF_PATH = "E:\Drone-ESC\downloads\Espressif\frameworks\esp-idf-v5.0.7"
$env:PYTHONNOUSERSITE = "1"
Remove-Item Env:PYTHONPATH -ErrorAction SilentlyContinue
Remove-Item Env:PYTHONHOME -ErrorAction SilentlyContinue
$venv = "E:\Drone-ESC\downloads\Espressif\python_env\idf5.0_py3.11_env\Scripts"
$env:PATH = "$venv;" + $env:PATH
Set-Location "E:\Drone-ESC\ESP-FLY\Firmware"
. "$env:IDF_PATH\export.ps1"
idf.py build
```

`idf.py` exists only in that session. The image is `build\ESPDrone.bin`. Do not flash an older bin after changing `drone_config.h`.

## While the chip is in download mode

The motor gates are pulled up. In the ROM downloader the firmware is not driving them, so every motor except the one on GPIO10 turns on. GPIO10 has an internal pull-down, so that motor stays off. `--after hard_reset` starts the app, which drives the gates low. Unplug the battery if the props are on.
