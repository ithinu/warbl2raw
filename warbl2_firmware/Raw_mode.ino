// switches to the raw mode
void rawEnable() {
    bool gyro = false;
    for(int s = RAW_SI_IMU + 3; s < RAW_SI_END; ++s)
        if(rawSensorEnabled(s)) {
            gyro = true;
            break;
        }
    if(gyro)
        sox.setGyroDataRate(LSM6DS_RATE_208_HZ);
    else
        sox.setGyroDataRate(LSM6DS_RATE_SHUTDOWN);
    noteMode = 0;
}

// switches to the note mode
void rawDisable() {
    sox.setGyroDataRate(LSM6DS_RATE_SHUTDOWN);
    loadPrefs();
    noteMode = 1;
}


bool rawSensorEnabled(int index) {
    return (rawMask&(1L << index)) != 0;
}


// Empties the raw queue
void clearRawQueue(void) {
    rawQueueSize = 0;
}


// If a sensor is enabled and changed, adds a value to rawQueue as is
void toRawQueue(byte index, short value) {
    if (rawSensorEnabled(index) && rawPrevious[index] != value) {
        raw_message m = {index, value};
        rawQueue[rawQueueSize++] = m;
        // Serial.print("[");
        // Serial.print(m.index);
        // Serial.print("]=");
        // Serial.println(m.value);
    }
}


// If a sensor is enabled and changed, adds a float measurements to rawQueue;
// the value should be inside -1 ... 1, it is then scaled to
// -RAW_MAX_VALUE ... RAW_MAX_VALUE
void floatToRawQueue(byte index, float value) {
    if(value < -1 || value > 1) {
      Serial.print("!!! ");
      Serial.print(index);
      Serial.print("=");
      Serial.println(value);
    }
    short v = (short)round(value*RAW_MAX_VALUE);
    toRawQueue(index, v);
}


// tests if breath pressure changed since last update, if yes adds
// a respective raw message to the queue
void rawUpdatePressure(void) {
#if NUM_PRESSURE != 1
    #error "exactly 1 pressure sensor supported"
#endif
    double value = smoothed_pressure - (sensorThreshold[0] << 2);
    floatToRawQueue(RAW_SI_PRESSURE, value/(float)RAW_PRESSURE_DIV);
}


// if tonehole readings changed since last update, adds
// raw messages of toneholes which changed to the queue
void rawUpdateToneholes(void) {
    for (int h = 0; h < NUM_TONEHOLES; ++h) {
        int p = RAW_SI_TONEHOLE + h;
        int r = toneholeRead[h] - senseDistance;
        if (r >= 0)
            floatToRawQueue(p, (r - (int)toneholeCovered[h])/(float)RAW_TONEHOLE_DIV);
    }
}


// if any of button states (pressed, just pressed, just released, long press) changed,
// adds a respective raw message to the queue
void rawUpdateButtons(void) {
    for (int b = 0; b < NUM_BUTTONS; ++b) {
        byte state = 0;
        if(pressed[b])
            state += 1 << RAW_BUTTON_PRESSED;
        if(justPressed[b])
            state += 1 << RAW_BUTTON_TOGGLE_ON;
        if(releasedRaw[b])
            state += 1 << RAW_BUTTON_TOGGLE_OFF;
        if(longPress[b])
            state += 1 << RAW_BUTTON_LONG_PRESS;
        toRawQueue(RAW_SI_BUTTON + b, state);
    }
}


// if an IMU degree changed since last update, adds
// a respective raw message to the queue
void rawUpdateImuDeg(int index, float value) {
    int p = RAW_SI_IMU + index;
    floatToRawQueue(p, value);
}


// if IMU data changed since last update, adds
// a respective raw message to the queue
void rawUpdateIMU(void) {
#if NUM_IMU != 6
    #error "exactly 6 IMU degrees supported"
#endif
    rawUpdateImuDeg(0, accelX/RAW_ACCEL_DIV);
    rawUpdateImuDeg(1, accelY/RAW_ACCEL_DIV);
    rawUpdateImuDeg(2, accelZ/RAW_ACCEL_DIV);
    rawUpdateImuDeg(3, roll/RAW_GYRO_DIV);
    rawUpdateImuDeg(4, pitch/RAW_GYRO_DIV);
    rawUpdateImuDeg(5, yaw/RAW_GYRO_DIV);
}


// Sends a raw message, updates rawPrevious
void sendRawQueueElement(raw_message* m) {
    int v = m->value;
    byte lsb = v & 0x7F;
    byte msb = (v >> 7) & 0x7F;
#if RAW_SI_END >= 32
    #error "index must not exceed range 0 ... 31"
#endif
    sendMIDI(0x80 + (m->index & 0x10), (m->index & 0x0f) + 1, lsb, msb);
    rawPrevious[m->index] = v;
//    Serial.print("c[");
//    Serial.print(m->index);
//    Serial.print("]=");
//    Serial.println(m->value);
}


int compareMessages(const void *cmp1, const void *cmp2)
{
  int a = ((raw_message*)cmp1)->priority;
  int b = ((raw_message*)cmp2)->priority;
  return b - a;
}


// Sends throttled queued raw/sensor messages. If their number exceedes rawThrottle, only these of highest
// priority and sent. Priority is based on sensor type and absolute difference compared to the most
// recent value sent
void consumeRawQueue(void) {
//    Serial.print("queue ");Serial.println(rawQueueSize);
    for(int q = 0; q < rawQueueSize; ++q) {
        raw_message* m = rawQueue + q;
        int diff = abs(m->value - rawPrevious[m->index]);
        switch(m->index) {
            case RAW_SI_PRESSURE ... RAW_SI_PRESSURE + NUM_PRESSURE - 1:
                m->priority = diff*RAW_PRI_PRESSURE;
                break;
            case RAW_SI_TONEHOLE ... RAW_SI_TONEHOLE + NUM_TONEHOLES - 1:
                m->priority = diff*RAW_PRI_TONEHOLE;
                break;
            case RAW_SI_BUTTON ... RAW_SI_BUTTON + NUM_BUTTONS - 1:
                m->priority = diff*RAW_PRI_BUTTON;
                break;
            case RAW_SI_IMU ... RAW_SI_IMU + 3 - 1:
                m->priority = diff*RAW_PRI_IMU_ACCEL;
                break;
            case RAW_SI_IMU + 3 ... RAW_SI_IMU + NUM_IMU - 1:
                m->priority = diff*RAW_PRI_IMU_GYRO;
                break;
            default:
                Serial.println("unknown type\n");
        };
        // Serial.print("\ti ");Serial.print(m->index);
        // Serial.print(" val ");Serial.print(rawPrevious[m->index]);Serial.print("=>");Serial.print(m->value);
        // Serial.print(" d ");Serial.print(diff);Serial.print(" pri ");Serial.println(m->priority);
    }
    qsort(rawQueue, rawQueueSize, sizeof(raw_message), compareMessages);
    for(int q = 0; q < min(rawQueueSize, rawThrottle); ++q) {
        raw_message* m = rawQueue + q;
        // Serial.print("\t>> i ");Serial.println(m->index);
        sendRawQueueElement(rawQueue + q);
    }
    clearRawQueue();
}
