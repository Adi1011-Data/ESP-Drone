/**
 * Attitude PID factory defaults for this ESC board.
 *
 * Live tuning does not require a rebuild. The dashboard at
 * http://192.168.43.42/ (or simulation/pid) writes RAM immediately
 * and can Save the values to NVS. These defines are the restore-
 * defaults set and the first-boot values when nothing is saved.
 *
 * Starting point is the ESP-Drone / ESP32-S2 micro brushed-quad tune.
 */
#ifndef PID_CONFIG_H_
#define PID_CONFIG_H_

#define DRONE_PID_ROLL_RATE_KP     190.0f
#define DRONE_PID_ROLL_RATE_KI     440.0f
#define DRONE_PID_ROLL_RATE_KD     2.6f
#define DRONE_PID_ROLL_RATE_ILIMIT 33.3f

#define DRONE_PID_PITCH_RATE_KP    190.0f
#define DRONE_PID_PITCH_RATE_KI    440.0f
#define DRONE_PID_PITCH_RATE_KD    2.6f
#define DRONE_PID_PITCH_RATE_ILIMIT 33.3f

#define DRONE_PID_YAW_RATE_KP      120.0f
#define DRONE_PID_YAW_RATE_KI      16.7f
#define DRONE_PID_YAW_RATE_KD      0.0f
#define DRONE_PID_YAW_RATE_ILIMIT  166.7f

#define DRONE_PID_ROLL_KP          5.3f
#define DRONE_PID_ROLL_KI          2.5f
#define DRONE_PID_ROLL_KD          0.0f
#define DRONE_PID_ROLL_ILIMIT      20.0f

#define DRONE_PID_PITCH_KP         5.3f
#define DRONE_PID_PITCH_KI         2.5f
#define DRONE_PID_PITCH_KD         0.0f
#define DRONE_PID_PITCH_ILIMIT     20.0f

#define DRONE_PID_YAW_KP           6.0f
#define DRONE_PID_YAW_KI           1.0f
#define DRONE_PID_YAW_KD           0.35f
#define DRONE_PID_YAW_ILIMIT       360.0f

#define DRONE_PID_INTEGRATION_LIMIT 5000.0f
#define DRONE_PID_OUTPUT_LIMIT     0.0f

/* Dashboard / HTTP setter clamps. Values outside these are clipped. */
#define PID_CLAMP_RATE_KP_MAX      500.0f
#define PID_CLAMP_RATE_KI_MAX      1000.0f
#define PID_CLAMP_RATE_KD_MAX      50.0f
#define PID_CLAMP_ANGLE_KP_MAX     30.0f
#define PID_CLAMP_ANGLE_KI_MAX     30.0f
#define PID_CLAMP_ANGLE_KD_MAX     10.0f

#endif /* PID_CONFIG_H_ */
