//
// Public constants of the raw mode.
//

#define NUM_PRESSURE 1
#define NUM_TONEHOLES 9
#define NUM_BUTTONS 3
#define NUM_IMU 6

#define RAW_SOURCE_NUM 19           // number of possible sources (measurements) producing sensor messages;
                                    // index of each defined in RAW_SI_*
#define RAW_SI_PRESSURE 0           // pressure sensor (var sensorValue 10 bit signed)
#define RAW_SI_TONEHOLE 1           // 1 to 9 for toneholes (var toneholeRead 8 bit unsigned)
#define RAW_SI_BUTTON 10            // 10 to 12 for buttons, bits as in RAW_BUTTON_*
#define RAW_SI_IMU 13               // 13 to 18 for 6 IMU measurements (accelX, accelY, accelZ, roll, pitch, yaw float)
#define RAW_SI_END 19
#if RAW_SI_TONEHOLE - RAW_SI_PRESSURE != NUM_PRESSURE
    #error "exactly 1 pressure sensor supported"
#endif
#if RAW_SI_BUTTON - RAW_SI_TONEHOLE != NUM_TONEHOLES
    #error "exactly 9 toneholes supported"
#endif
#if RAW_SI_IMU - RAW_SI_BUTTON != NUM_BUTTONS
    #error "exactly 3 buttons supported"
#endif
#if RAW_SOURCE_NUM != RAW_SI_END
    #error "source num mismatch"
#endif
#if RAW_SI_END > 31
    #error "raw mask supports up to 31 sensors"
#endif
#define RAW_PRESSURE_DIV 1024       // Divider normalizing pressure values
#define RAW_TONEHOLE_DIV 512        // Divider normalizing tonehole values
#define RAW_ACCEL_DIV 256           // Divider normalizing IMU accel* values
#define RAW_GYRO_DIV 180            // Divider normalizing IMU roll, pitch, yaw values
#define RAW_MAX_VALUE (1 << 13)     // Maximum range of integer representation of raw values
                                    // -RAW_MAX_VALUE ... RAW_MAX_VALUE
#define RAW_BUTTON_PRESSED 0        // Bit for currently pressed
#define RAW_BUTTON_TOGGLE_ON 1      // Bit for just pressed
#define RAW_BUTTON_TOGGLE_OFF 2     // Bit for just released
#define RAW_BUTTON_LONG_PRESS 3     // Bit for long press

#define RAW_PRI_PRESSURE 1          // Pressure priority, multiplies diffs of values in ~ -2000 ... 8000
#define RAW_PRI_TONEHOLE 1          // Tonehole priority, multiplies diffs of values in ~ -5000 ... 1200, >= 0 pressed
#define RAW_PRI_BUTTON 1000         // Button priority, multiplies diffs of values in 0 ... 15, see RAW_BUTTON_*
#define RAW_PRI_IMU_ACCEL 1         // Accel priority, multiplies diffs of values in -400 ... 400
#define RAW_PRI_IMU_GYRO 1          // Gyro priority, multiplies diffs of values in ?? ~ 1000, 1500
