#include "pid_http.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_http_server.h"

#include "stm32_legacy.h"
#include "attitude_controller.h"
#include "drone_config.h"
#include "pid_store.h"

#define DEBUG_MODULE "PIDHTTP"
#include "debug_cf.h"

extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");

static httpd_handle_t s_httpd;

static const char *const kGainKeys[] = {
  "rate_roll_kp", "rate_roll_ki", "rate_roll_kd",
  "rate_pitch_kp", "rate_pitch_ki", "rate_pitch_kd",
  "rate_yaw_kp", "rate_yaw_ki", "rate_yaw_kd",
  "angle_roll_kp", "angle_roll_ki", "angle_roll_kd",
  "angle_pitch_kp", "angle_pitch_ki", "angle_pitch_kd",
  "angle_yaw_kp", "angle_yaw_ki", "angle_yaw_kd",
};

static float *gainSlot(attitudePidGains_t *gains, int index)
{
  float *slots[] = {
    &gains->rate.roll.kp, &gains->rate.roll.ki, &gains->rate.roll.kd,
    &gains->rate.pitch.kp, &gains->rate.pitch.ki, &gains->rate.pitch.kd,
    &gains->rate.yaw.kp, &gains->rate.yaw.ki, &gains->rate.yaw.kd,
    &gains->angle.roll.kp, &gains->angle.roll.ki, &gains->angle.roll.kd,
    &gains->angle.pitch.kp, &gains->angle.pitch.ki, &gains->angle.pitch.kd,
    &gains->angle.yaw.kp, &gains->angle.yaw.ki, &gains->angle.yaw.kd,
  };

  if (index < 0 || index >= 18) {
    return NULL;
  }
  return slots[index];
}

static void setCors(httpd_req_t *req)
{
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
}

static esp_err_t sendJson(httpd_req_t *req)
{
  char body[768];
  attitudePidGains_t gains;
  int n;

  attitudePidGetGains(&gains);
  n = snprintf(body, sizeof(body),
    "{\"saved\":%s,\"dirty\":%s,"
    "\"rate_roll_kp\":%.5g,\"rate_roll_ki\":%.5g,\"rate_roll_kd\":%.5g,"
    "\"rate_pitch_kp\":%.5g,\"rate_pitch_ki\":%.5g,\"rate_pitch_kd\":%.5g,"
    "\"rate_yaw_kp\":%.5g,\"rate_yaw_ki\":%.5g,\"rate_yaw_kd\":%.5g,"
    "\"angle_roll_kp\":%.5g,\"angle_roll_ki\":%.5g,\"angle_roll_kd\":%.5g,"
    "\"angle_pitch_kp\":%.5g,\"angle_pitch_ki\":%.5g,\"angle_pitch_kd\":%.5g,"
    "\"angle_yaw_kp\":%.5g,\"angle_yaw_ki\":%.5g,\"angle_yaw_kd\":%.5g}",
    pidStoreHasSaved() ? "true" : "false",
    pidStoreIsDirty() ? "true" : "false",
    gains.rate.roll.kp, gains.rate.roll.ki, gains.rate.roll.kd,
    gains.rate.pitch.kp, gains.rate.pitch.ki, gains.rate.pitch.kd,
    gains.rate.yaw.kp, gains.rate.yaw.ki, gains.rate.yaw.kd,
    gains.angle.roll.kp, gains.angle.roll.ki, gains.angle.roll.kd,
    gains.angle.pitch.kp, gains.angle.pitch.ki, gains.angle.pitch.kd,
    gains.angle.yaw.kp, gains.angle.yaw.ki, gains.angle.yaw.kd);

  if (n <= 0 || n >= (int)sizeof(body)) {
    return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "json");
  }

  setCors(req);
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  return httpd_resp_send(req, body, n);
}

static bool jsonExtractFloat(const char *json, const char *key, float *out)
{
  char pattern[40];
  const char *found;
  const char *colon;
  char *end;
  float value;
  int n;

  n = snprintf(pattern, sizeof(pattern), "\"%s\"", key);
  if (n <= 0 || n >= (int)sizeof(pattern)) {
    return false;
  }

  found = strstr(json, pattern);
  if (found == NULL) {
    return false;
  }

  colon = strchr(found + n, ':');
  if (colon == NULL) {
    return false;
  }
  colon++;
  while (*colon == ' ' || *colon == '\t' || *colon == '\n' || *colon == '\r') {
    colon++;
  }

  value = strtof(colon, &end);
  if (end == colon) {
    return false;
  }

  *out = value;
  return true;
}

static void overlayGains(const char *json, attitudePidGains_t *gains)
{
  int i;

  for (i = 0; i < 18; i++) {
    float value;
    float *slot = gainSlot(gains, i);
    if (slot != NULL && jsonExtractFloat(json, kGainKeys[i], &value)) {
      *slot = value;
    }
  }
}

static esp_err_t recvBody(httpd_req_t *req, char *buf, size_t bufLen)
{
  int received = 0;

  if (req->content_len <= 0 || req->content_len >= (int)bufLen) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "body");
    return ESP_FAIL;
  }

  while (received < req->content_len) {
    int n = httpd_req_recv(req, buf + received, req->content_len - received);
    if (n <= 0) {
      return ESP_FAIL;
    }
    received += n;
  }
  buf[received] = '\0';
  return ESP_OK;
}

static esp_err_t optionsHandler(httpd_req_t *req)
{
  setCors(req);
  httpd_resp_set_status(req, "204 No Content");
  return httpd_resp_send(req, NULL, 0);
}

static esp_err_t rootGetHandler(httpd_req_t *req)
{
  const size_t length = (size_t)(index_html_end - index_html_start);

  setCors(req);
  httpd_resp_set_type(req, "text/html; charset=utf-8");
  return httpd_resp_send(req, (const char *)index_html_start, length);
}

static esp_err_t pidGetHandler(httpd_req_t *req)
{
  if (!attitudeControllerTest()) {
    setCors(req);
    return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "pid not ready");
  }
  return sendJson(req);
}

static esp_err_t pidPostHandler(httpd_req_t *req)
{
  char body[768];
  attitudePidGains_t gains;

  if (!attitudeControllerTest()) {
    setCors(req);
    return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "pid not ready");
  }
  if (recvBody(req, body, sizeof(body)) != ESP_OK) {
    return ESP_FAIL;
  }

  attitudePidGetGains(&gains);
  overlayGains(body, &gains);
  attitudePidSetGains(&gains);
  pidStoreMarkLiveChanged();
  return sendJson(req);
}

static esp_err_t pidSaveHandler(httpd_req_t *req)
{
  if (!pidStoreSave()) {
    setCors(req);
    return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "save failed");
  }
  return sendJson(req);
}

static esp_err_t pidDefaultsHandler(httpd_req_t *req)
{
  pidStoreRestoreDefaults();
  return sendJson(req);
}

static void registerUri(httpd_handle_t server, const char *uri,
                        httpd_method_t method, esp_err_t (*handler)(httpd_req_t *))
{
  httpd_uri_t route = {
    .uri = uri,
    .method = method,
    .handler = handler,
  };
  httpd_register_uri_handler(server, &route);
}

void pidHttpStart(void)
{
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  if (s_httpd != NULL) {
    return;
  }

  config.server_port = DRONE_PID_HTTP_PORT;
  config.lru_purge_enable = true;
  config.max_open_sockets = 4;
  config.max_uri_handlers = 10;
  config.stack_size = 6144;

  if (httpd_start(&s_httpd, &config) != ESP_OK) {
    DEBUG_PRINTW("PID HTTP start failed");
    s_httpd = NULL;
    return;
  }

  registerUri(s_httpd, "/", HTTP_GET, rootGetHandler);
  registerUri(s_httpd, "/api/pid", HTTP_GET, pidGetHandler);
  registerUri(s_httpd, "/api/pid", HTTP_POST, pidPostHandler);
  registerUri(s_httpd, "/api/pid", HTTP_OPTIONS, optionsHandler);
  registerUri(s_httpd, "/api/pid/save", HTTP_POST, pidSaveHandler);
  registerUri(s_httpd, "/api/pid/save", HTTP_OPTIONS, optionsHandler);
  registerUri(s_httpd, "/api/pid/defaults", HTTP_POST, pidDefaultsHandler);
  registerUri(s_httpd, "/api/pid/defaults", HTTP_OPTIONS, optionsHandler);

  DEBUG_PRINTI("PID dashboard http://%s:%d/", DRONE_WIFI_IP, DRONE_PID_HTTP_PORT);
}
