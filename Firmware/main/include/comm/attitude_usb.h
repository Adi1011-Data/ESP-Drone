/**
 * Non-blocking USB-C stream of the fused MPU6050 quaternion.
 * The viewer in ESP-FLY/simulation/viewer reads lines of the form:
 *   ATT,<w>,<x>,<y>,<z>
 */
#ifndef ATTITUDE_USB_H_
#define ATTITUDE_USB_H_

void attitudeUsbStart(void);
void attitudeUsbSendQuaternion(float x, float y, float z, float w);

#endif
