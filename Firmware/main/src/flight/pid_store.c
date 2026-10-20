#include "pid_store.h"

#include <stdint.h>

#include "nvs.h"
#include "stm32_legacy.h"
#include "attitude_controller.h"

#define DEBUG_MODULE "PIDSTORE"
#include "debug_cf.h"

#define PID_NVS_NS      "pidtune"
#define PID_NVS_KEY     "gains"
#define PID_NVS_MAGIC   0x50494431u
#define PID_NVS_VERSION 1u

typedef struct {
  uint32_t magic;
  uint32_t version;
  attitudePidGains_t gains;
} pidNvsBlob_t;

static bool s_saved;
static bool s_dirty;

void pidStoreInit(void)
{
  nvs_handle_t handle;
  pidNvsBlob_t blob;
  size_t length = sizeof(blob);
  esp_err_t err;

  s_saved = false;
  s_dirty = false;

  err = nvs_open(PID_NVS_NS, NVS_READONLY, &handle);
  if (err != ESP_OK) {
    DEBUG_PRINTI("no saved PID (nvs %s)", esp_err_to_name(err));
    return;
  }

  err = nvs_get_blob(handle, PID_NVS_KEY, &blob, &length);
  nvs_close(handle);

  if (err != ESP_OK || length != sizeof(blob) ||
      blob.magic != PID_NVS_MAGIC || blob.version != PID_NVS_VERSION) {
    DEBUG_PRINTI("using factory PID defaults");
    return;
  }

  attitudePidSetGains(&blob.gains);
  s_saved = true;
  DEBUG_PRINTI("loaded saved PID from NVS");
}

bool pidStoreHasSaved(void)
{
  return s_saved;
}

bool pidStoreIsDirty(void)
{
  return s_dirty;
}

void pidStoreMarkLiveChanged(void)
{
  s_dirty = true;
}

bool pidStoreSave(void)
{
  nvs_handle_t handle;
  pidNvsBlob_t blob = {
    .magic = PID_NVS_MAGIC,
    .version = PID_NVS_VERSION,
  };
  esp_err_t err;

  attitudePidGetGains(&blob.gains);

  err = nvs_open(PID_NVS_NS, NVS_READWRITE, &handle);
  if (err != ESP_OK) {
    DEBUG_PRINTW("PID save open failed %s", esp_err_to_name(err));
    return false;
  }

  err = nvs_set_blob(handle, PID_NVS_KEY, &blob, sizeof(blob));
  if (err == ESP_OK) {
    err = nvs_commit(handle);
  }
  nvs_close(handle);

  s_saved = (err == ESP_OK);
  if (s_saved) {
    s_dirty = false;
  } else {
    DEBUG_PRINTW("PID save failed %s", esp_err_to_name(err));
  }
  return s_saved;
}

bool pidStoreRestoreDefaults(void)
{
  nvs_handle_t handle;

  attitudePidLoadDefaults();

  if (nvs_open(PID_NVS_NS, NVS_READWRITE, &handle) == ESP_OK) {
    nvs_erase_key(handle, PID_NVS_KEY);
    nvs_commit(handle);
    nvs_close(handle);
  }

  s_saved = false;
  s_dirty = false;
  DEBUG_PRINTI("PID restored to factory defaults");
  return true;
}
