/**
 * XIAO ESP32-C3 flight settings for this ESC board.
 *
 * Change values here, then rebuild and flash. This file replaces the
 * matching menuconfig values, so you do not have to open menuconfig
 * for pins, battery scale, axis signs, or the Wi-Fi password.
 * Attitude PID factory defaults live in pid_config.h. Live tuning
 * is the HTTP dashboard (simulation/pid); it does not need a reflash.
 *
 * Props off until each motor spins the right way on the bench.
 */
#ifndef DRONE_CONFIG_H_
#define DRONE_CONFIG_H_

/* -------------------------------------------------------------------------- */
/* MOSFET gates. Low-side AO3400A, one GPIO per motor.                       */
/* XIAO pin -> GPIO. These match the ESC schematic.                          */
/*                                                                            */
/*   MOT1  D0   GPIO2   mixer M1, front-left                                  */
/*   MOT2  D10  GPIO10  mixer M2, front-right                                 */
/*   MOT3  D1   GPIO3   mixer M3, rear-right                                  */
/*   MOT4  D3   GPIO5   mixer M4, rear-left                                   */
/*                                                                            */
/* D9 / GPIO9 is the XIAO BOOT strap. Leave it unconnected. Holding it low   */
/* at reset enters download mode. Do not use D8 / GPIO8 for a motor either.  */
/* If a corner climbs instead of holding level, swap that motor's GPIO       */
/* with the corner that should have received the correction.                 */
/* -------------------------------------------------------------------------- */
#define DRONE_MOTOR1_GPIO          2
#define DRONE_MOTOR2_GPIO          10
#define DRONE_MOTOR3_GPIO          3
#define DRONE_MOTOR4_GPIO          5

/* LEDC channels 0-3. ESP32-C3 has six channels; 6 and 7 do not exist. */
#define DRONE_MOTOR_PWM_HZ         20000

/* Minimum command while armed, 0..65535. 0 keeps the props stopped at idle. */
#define DRONE_IDLE_THRUST          0

/* -------------------------------------------------------------------------- */
/* MPU6050. AD0 is tied low, so the address is 0x68.                         */
/*   SDA  D4  GPIO6                                                          */
/*   SCL  D5  GPIO7                                                          */
/*   INT  D7  GPIO20                                                         */
/* Signs are +1 or -1. Flip a sign if that axis is reversed on the bench.   */
/* -------------------------------------------------------------------------- */
#define DRONE_I2C_SDA_GPIO         6
#define DRONE_I2C_SCL_GPIO         7
#define DRONE_MPU_INT_GPIO         20

#define DRONE_GYRO_X_SIGN          (-1)
#define DRONE_GYRO_Y_SIGN          (1)
#define DRONE_GYRO_Z_SIGN          (1)
#define DRONE_ACC_X_SIGN           (-1)
#define DRONE_ACC_Y_SIGN           (1)
#define DRONE_ACC_Z_SIGN           (1)

/* Attitude lines on the USB-C serial port, for simulation/viewer.
 * Format: ATT,<w>,<x>,<y>,<z>
 * 0 turns the stream off. Close idf.py monitor before the viewer
 * opens this same COM port. */
#define DRONE_USB_ATTITUDE_HZ      100

/* Level trim in centidegrees. 90 means 0.90 degree. */
#define DRONE_PITCH_CALIB_CENTIDEG 0
#define DRONE_ROLL_CALIB_CENTIDEG  0

/* -------------------------------------------------------------------------- */
/* Battery. R11 and R12 are both 100k, so the ADC sees half the cell.        */
/* Sense pin is D2 / GPIO4, which is ADC1 channel 4 on the ESP32-C3.         */
/* -------------------------------------------------------------------------- */
#define DRONE_VBAT_GPIO            4
#define DRONE_VBAT_ADC_CHANNEL     4
#define DRONE_BATTERY_DIVIDER      2.0f

/* 255 disables a LED. This board has no status LEDs on free GPIOs. */
#define DRONE_LED_RED_GPIO         255
#define DRONE_LED_GREEN_GPIO       255
#define DRONE_LED_BLUE_GPIO        255

/* No buzzer is fitted. Set to 1 only after two free GPIOs are wired. */
#define DRONE_BUZZER_ENABLE        0

/* No optical-flow deck. Negative pins keep SPI off the motor gates. */
#define DRONE_SPI_MISO             (-1)
#define DRONE_SPI_MOSI             (-1)
#define DRONE_SPI_CLK              (-1)
#define DRONE_SPI_CS               (-1)

/* -------------------------------------------------------------------------- */
/* ESP-Drone Android app 0.7.3 (https://www.pgyer.com/a27L).                  */
/* The phone joins "<prefix>_<MAC>" with the password below, then the app    */
/* sends CRTP to the address and UDP port. Those two must stay as written.   */
/* -------------------------------------------------------------------------- */
#define DRONE_WIFI_SSID_PREFIX     "ESP-DRONE"
#define DRONE_WIFI_PASSWORD        "12345678"
#define DRONE_WIFI_IP              "192.168.43.42"
#define DRONE_UDP_PORT             2390

/* PID dashboard HTTP. Must not share UDP 2390 with the phone app. */
#define DRONE_PID_HTTP_PORT        80

/* Apply the values above on top of menuconfig / sdkconfig.h. */
#ifdef CONFIG_MOTOR01_PIN
#undef CONFIG_MOTOR01_PIN
#endif
#define CONFIG_MOTOR01_PIN DRONE_MOTOR1_GPIO

#ifdef CONFIG_MOTOR02_PIN
#undef CONFIG_MOTOR02_PIN
#endif
#define CONFIG_MOTOR02_PIN DRONE_MOTOR2_GPIO

#ifdef CONFIG_MOTOR03_PIN
#undef CONFIG_MOTOR03_PIN
#endif
#define CONFIG_MOTOR03_PIN DRONE_MOTOR3_GPIO

#ifdef CONFIG_MOTOR04_PIN
#undef CONFIG_MOTOR04_PIN
#endif
#define CONFIG_MOTOR04_PIN DRONE_MOTOR4_GPIO

#ifdef CONFIG_I2C0_PIN_SDA
#undef CONFIG_I2C0_PIN_SDA
#endif
#define CONFIG_I2C0_PIN_SDA DRONE_I2C_SDA_GPIO

#ifdef CONFIG_I2C0_PIN_SCL
#undef CONFIG_I2C0_PIN_SCL
#endif
#define CONFIG_I2C0_PIN_SCL DRONE_I2C_SCL_GPIO

#ifdef CONFIG_MPU_PIN_INT
#undef CONFIG_MPU_PIN_INT
#endif
#define CONFIG_MPU_PIN_INT DRONE_MPU_INT_GPIO

#ifdef CONFIG_ADC1_PIN
#undef CONFIG_ADC1_PIN
#endif
#define CONFIG_ADC1_PIN DRONE_VBAT_GPIO

#ifdef CONFIG_LED_PIN_RED
#undef CONFIG_LED_PIN_RED
#endif
#define CONFIG_LED_PIN_RED DRONE_LED_RED_GPIO

#ifdef CONFIG_LED_PIN_GREEN
#undef CONFIG_LED_PIN_GREEN
#endif
#define CONFIG_LED_PIN_GREEN DRONE_LED_GREEN_GPIO

#ifdef CONFIG_LED_PIN_BLUE
#undef CONFIG_LED_PIN_BLUE
#endif
#define CONFIG_LED_PIN_BLUE DRONE_LED_BLUE_GPIO

#ifdef CONFIG_PITCH_CALIB
#undef CONFIG_PITCH_CALIB
#endif
#define CONFIG_PITCH_CALIB DRONE_PITCH_CALIB_CENTIDEG

#ifdef CONFIG_ROLL_CALIB
#undef CONFIG_ROLL_CALIB
#endif
#define CONFIG_ROLL_CALIB DRONE_ROLL_CALIB_CENTIDEG

#ifdef CONFIG_SPI_PIN_MISO
#undef CONFIG_SPI_PIN_MISO
#endif
#define CONFIG_SPI_PIN_MISO DRONE_SPI_MISO

#ifdef CONFIG_SPI_PIN_MOSI
#undef CONFIG_SPI_PIN_MOSI
#endif
#define CONFIG_SPI_PIN_MOSI DRONE_SPI_MOSI

#ifdef CONFIG_SPI_PIN_CLK
#undef CONFIG_SPI_PIN_CLK
#endif
#define CONFIG_SPI_PIN_CLK DRONE_SPI_CLK

#ifdef CONFIG_SPI_PIN_CS0
#undef CONFIG_SPI_PIN_CS0
#endif
#define CONFIG_SPI_PIN_CS0 DRONE_SPI_CS

#if !DRONE_BUZZER_ENABLE
#undef CONFIG_BUZZER_ON
#endif

#ifndef DEFAULT_IDLE_THRUST
#define DEFAULT_IDLE_THRUST DRONE_IDLE_THRUST
#endif

#endif /* DRONE_CONFIG_H_ */
