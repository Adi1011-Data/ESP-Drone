#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

#include "attitude_ble.h"

#if CONFIG_BT_ENABLED && CONFIG_BT_NIMBLE_ENABLED

#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#define ATT_BLE_DEVICE_NAME "ESP-FLY-ATT"

static const char *TAG = "ATT_BLE";

static uint8_t own_addr_type;
static uint16_t conn_handle;
static uint16_t att_chr_handle;
static bool is_connected;
static bool notify_enabled;
static bool is_started;
static char last_packet[48] = "ATT,1.0000,0.0000,0.0000,0.0000\n";
static portMUX_TYPE packet_mux = portMUX_INITIALIZER_UNLOCKED;

static int attitude_ble_gap_event(struct ble_gap_event *event, void *arg);

/* Nordic UART Service UUIDs. Browser code uses the same values. */
static const ble_uuid128_t att_svc_uuid =
  BLE_UUID128_INIT(0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0,
                   0x93, 0xf3, 0xa3, 0xb5, 0x01, 0x00, 0x40, 0x6e);
static const ble_uuid128_t att_notify_uuid =
  BLE_UUID128_INIT(0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0,
                   0x93, 0xf3, 0xa3, 0xb5, 0x03, 0x00, 0x40, 0x6e);

static int attitude_ble_access(uint16_t conn, uint16_t attr,
                               struct ble_gatt_access_ctxt *ctxt, void *arg)
{
  char packet[sizeof(last_packet)];
  int rc;

  if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR) {
    return BLE_ATT_ERR_UNLIKELY;
  }

  taskENTER_CRITICAL(&packet_mux);
  memcpy(packet, last_packet, sizeof(packet));
  taskEXIT_CRITICAL(&packet_mux);

  rc = os_mbuf_append(ctxt->om, packet, strnlen(packet, sizeof(packet)));
  return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

static const struct ble_gatt_svc_def attitude_svcs[] = {
  {
    .type = BLE_GATT_SVC_TYPE_PRIMARY,
    .uuid = &att_svc_uuid.u,
    .characteristics = (struct ble_gatt_chr_def[]) {
      {
        .uuid = &att_notify_uuid.u,
        .access_cb = attitude_ble_access,
        .val_handle = &att_chr_handle,
        .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
      },
      { 0 }
    },
  },
  { 0 }
};

static void attitude_ble_advertise(void)
{
  struct ble_gap_adv_params adv_params;
  struct ble_hs_adv_fields fields;
  struct ble_hs_adv_fields rsp;
  int rc;

  /* Legacy advertising is 31 bytes. Flags + "ESP-FLY-ATT" fits; a 128-bit
   * UUID does not fit in the same packet, so it goes in the scan response.
   * Web Bluetooth namePrefix needs the complete name in the advertisement.
   */
  memset(&fields, 0, sizeof(fields));
  fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
  fields.name = (uint8_t *)ATT_BLE_DEVICE_NAME;
  fields.name_len = strlen(ATT_BLE_DEVICE_NAME);
  fields.name_is_complete = 1;

  rc = ble_gap_adv_set_fields(&fields);
  if (rc != 0) {
    ESP_LOGW(TAG, "advertising fields failed: %d", rc);
    return;
  }

  memset(&rsp, 0, sizeof(rsp));
  rsp.uuids128 = &att_svc_uuid;
  rsp.num_uuids128 = 1;
  rsp.uuids128_is_complete = 1;
  rc = ble_gap_adv_rsp_set_fields(&rsp);
  if (rc != 0) {
    ESP_LOGW(TAG, "scan response fields failed: %d", rc);
  }

  memset(&adv_params, 0, sizeof(adv_params));
  adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
  adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

  rc = ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER, &adv_params, attitude_ble_gap_event, NULL);
  if (rc != 0) {
    ESP_LOGW(TAG, "advertising start failed: %d", rc);
    return;
  }

  ESP_LOGI(TAG, "advertising as %s", ATT_BLE_DEVICE_NAME);
}

static int attitude_ble_gap_event(struct ble_gap_event *event, void *arg)
{
  switch (event->type) {
  case BLE_GAP_EVENT_CONNECT:
    if (event->connect.status == 0) {
      conn_handle = event->connect.conn_handle;
      is_connected = true;
    } else {
      attitude_ble_advertise();
    }
    break;

  case BLE_GAP_EVENT_DISCONNECT:
    is_connected = false;
    notify_enabled = false;
    attitude_ble_advertise();
    break;

  case BLE_GAP_EVENT_ADV_COMPLETE:
    attitude_ble_advertise();
    break;

  case BLE_GAP_EVENT_SUBSCRIBE:
    if (event->subscribe.attr_handle == att_chr_handle) {
      notify_enabled = event->subscribe.cur_notify;
    }
    break;

  default:
    break;
  }

  return 0;
}

static void attitude_ble_on_sync(void)
{
  int rc = ble_hs_id_infer_auto(0, &own_addr_type);
  if (rc == 0) {
    attitude_ble_advertise();
  }
}

static void attitude_ble_on_reset(int reason)
{
  is_connected = false;
  notify_enabled = false;
}

static void attitude_ble_host_task(void *param)
{
  nimble_port_run();
  nimble_port_freertos_deinit();
}

static int attitude_ble_gatt_init(void)
{
  int rc;

  ble_svc_gap_init();
  ble_svc_gatt_init();

  rc = ble_gatts_count_cfg(attitude_svcs);
  if (rc != 0) {
    return rc;
  }

  return ble_gatts_add_svcs(attitude_svcs);
}

void attitudeBleStart(void)
{
  if (is_started) {
    return;
  }
  is_started = true;

  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  if (ret != ESP_OK) {
    ESP_LOGW(TAG, "nvs init failed: %d", ret);
    return;
  }

  ret = nimble_port_init();
  if (ret != ESP_OK) {
    ESP_LOGW(TAG, "nimble init failed: %d", ret);
    return;
  }

  ble_hs_cfg.sync_cb = attitude_ble_on_sync;
  ble_hs_cfg.reset_cb = attitude_ble_on_reset;

  int rc = attitude_ble_gatt_init();
  if (rc != 0) {
    ESP_LOGW(TAG, "gatt init failed: %d", rc);
    return;
  }

  rc = ble_svc_gap_device_name_set(ATT_BLE_DEVICE_NAME);
  if (rc != 0) {
    ESP_LOGW(TAG, "device name failed: %d", rc);
    return;
  }

  nimble_port_freertos_init(attitude_ble_host_task);
}

void attitudeBleSend(const char *packet, int length)
{
  if (packet == NULL || length <= 0) {
    return;
  }

  if (length >= (int)sizeof(last_packet)) {
    length = sizeof(last_packet) - 1;
  }

  taskENTER_CRITICAL(&packet_mux);
  memcpy(last_packet, packet, length);
  last_packet[length] = '\0';
  taskEXIT_CRITICAL(&packet_mux);

  if (!is_connected || !notify_enabled) {
    return;
  }

  struct os_mbuf *om = ble_hs_mbuf_from_flat(packet, length);
  if (om == NULL) {
    return;
  }

  int rc = ble_gatts_notify_custom(conn_handle, att_chr_handle, om);
  if (rc != 0) {
    os_mbuf_free_chain(om);
  }
}

#else

void attitudeBleStart(void)
{
}

void attitudeBleSend(const char *packet, int length)
{
}

#endif
