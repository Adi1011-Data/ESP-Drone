/**
 * Optional BLE attitude stream for the simulation viewer.
 * Uses a Nordic UART-style service:
 *   Service:        6e400001-b5a3-f393-e0a9-e50e24dcca9e
 *   Notify char:    6e400003-b5a3-f393-e0a9-e50e24dcca9e
 */
#ifndef ATTITUDE_BLE_H_
#define ATTITUDE_BLE_H_

void attitudeBleStart(void);
void attitudeBleSend(const char *packet, int length);

#endif
