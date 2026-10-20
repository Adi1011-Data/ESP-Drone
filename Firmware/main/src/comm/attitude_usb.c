#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "drone_config.h"
#include "attitude_usb.h"
#include "attitude_ble.h"

#if DRONE_USB_ATTITUDE_HZ > 0

#include "esp_timer.h"
#include "hal/usb_serial_jtag_ll.h"

/* Writes only while the USB FIFO has room, and never waits, so a closed
 * serial port cannot stall the flight loop.
 */
static portMUX_TYPE s_att_mux = portMUX_INITIALIZER_UNLOCKED;

static void attitudeUsbWriteLine(const char *line, int length)
{
  int wrote;

  if (length <= 0) {
    return;
  }

  taskENTER_CRITICAL(&s_att_mux);
  wrote = usb_serial_jtag_ll_write_txfifo((const uint8_t *)line, (uint32_t)length);
  if (wrote > 0) {
    usb_serial_jtag_ll_txfifo_flush();
  }
  taskEXIT_CRITICAL(&s_att_mux);
}

void attitudeUsbSendQuaternion(float x, float y, float z, float w)
{
  static int64_t lastSendUs;
  char line[48];
  const int64_t nowUs = esp_timer_get_time();
  const int64_t periodUs = 1000000 / DRONE_USB_ATTITUDE_HZ;

  if ((nowUs - lastSendUs) < periodUs) {
    return;
  }
  lastSendUs = nowUs;

  int n = snprintf(line, sizeof(line), "ATT,%.4f,%.4f,%.4f,%.4f\n", w, x, y, z);
  if (n <= 0 || n >= (int)sizeof(line)) {
    return;
  }

  attitudeUsbWriteLine(line, n);
  attitudeBleSend(line, n);
}

void attitudeUsbStart(void)
{
  attitudeBleStart();
}

#else

void attitudeUsbSendQuaternion(float x, float y, float z, float w)
{
}

void attitudeUsbStart(void)
{
  attitudeBleStart();
}

#endif
