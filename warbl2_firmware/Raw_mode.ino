// switches to the raw mode
void rawEnable() {
    rawGyroEnable = false;
    for(int s = RAW_TYPE_IMU + 3; s < RAW_TYPE_END; ++s)
        if(rawSensorEnabled(s)) {
            rawGyroEnable = true;
            break;
        }
    if(rawGyroEnable)
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


bool rawSensorEnabled(int type) {
    return (rawMask&(1L << type)) != 0;
}


// Empties the raw queue
void clearRawQueue(void) {
    rawQueueSize = 0;
}


// If a sensor is enabled and changed, adds a value to rawQueue as is
void toRawQueue(byte type, short value) {
    if (rawSensorEnabled(type) && rawPrevious[type] != value) {
        raw_message m = {type, value};
        rawQueue[rawQueueSize++] = m;
        // Serial.print("[");
        // Serial.print(m.type);
        // Serial.print("]=");
        // Serial.println(m.value);
    }
}


// If a sensor is enabled and changed, adds a float measurements to rawQueue;
// the value should be inside -1 ... 1, it is then scaled to
// -RAW_MAX_VALUE ... RAW_MAX_VALUE
void floatToRawQueue(byte type, float value) {
    short v = (short)round(value*RAW_MAX_VALUE);
    if(value < -1 || value > 1 /*|| type == 8*/) {
      Serial.print("!!! ");
      Serial.print(type);
      Serial.print("=");
      Serial.println(value, 4);
      Serial.print("=");
      Serial.println(v);
    }
    toRawQueue(type, v);
}


// tests if breath pressure changed since last update, if yes adds
// a respective raw message to the queue
void rawUpdatePressure(void) {
#if NUM_PRESSURE != 1
    #error "exactly 1 pressure sensor supported"
#endif
    double value = smoothed_pressure - (sensorThreshold[0] << 2);
    floatToRawQueue(RAW_TYPE_PRESSURE, value/(float)RAW_PRESSURE_DIV);
}


// if tonehole readings changed since last update, adds
// raw messages of toneholes which changed to the queue
void rawUpdateToneholes(void) {
    for (int h = 0; h < NUM_TONEHOLES; ++h) {
        int p = RAW_TYPE_TONEHOLE + h;
        int r = toneholeRead[h];// - senseDistance;
        //if (r >= 5 && h == 7) {
        //    Serial.println(r);
        //    Serial.println((int)toneholeCovered[h] - r);
        //}
        floatToRawQueue(p, ((int)toneholeCovered[h] - r)/(float)RAW_TONEHOLE_DIV);
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
        toRawQueue(RAW_TYPE_BUTTON + b, state);
    }
}


// if an IMU degree changed since last update, adds
// a respective raw message to the queue
void rawUpdateImuDeg(int type, float value) {
    int p = RAW_TYPE_IMU + type;
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
    if(rawGyroEnable) {
        rawUpdateImuDeg(3, roll/RAW_GYRO_DIV);
        rawUpdateImuDeg(4, pitch/RAW_GYRO_DIV);
        rawUpdateImuDeg(5, yaw/RAW_GYRO_DIV);
    }
}


// Sends a raw message, updates rawPrevious
void sendRawQueueElement(raw_message* m) {
    int v = m->value;
    byte lsb = v & 0x7F;
    byte msb = (v >> 7) & 0x7F;
#if RAW_TYPE_END >= 32
    #error "type must not exceed range 0 ... 31"
#endif
    sendMIDI(0x80 + (m->type & 0x10), (m->type & 0x0f) + 1, lsb, msb);
    rawPrevious[m->type] = v;
//    Serial.print("c[");
//    Serial.print(m->type);
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
        int diff = abs(m->value - rawPrevious[m->type]);
        switch(m->type) {
            case RAW_TYPE_PRESSURE ... RAW_TYPE_PRESSURE + NUM_PRESSURE - 1:
                m->priority = diff*RAW_PRI_PRESSURE;
                break;
            case RAW_TYPE_TONEHOLE ... RAW_TYPE_TONEHOLE + NUM_TONEHOLES - 1:
                m->priority = diff*RAW_PRI_TONEHOLE;
                break;
            case RAW_TYPE_BUTTON ... RAW_TYPE_BUTTON + NUM_BUTTONS - 1:
                m->priority = diff*RAW_PRI_BUTTON;
                break;
            case RAW_TYPE_IMU ... RAW_TYPE_IMU + 3 - 1:
                m->priority = diff*RAW_PRI_IMU_ACCEL;
                break;
            case RAW_TYPE_IMU + 3 ... RAW_TYPE_IMU + NUM_IMU - 1:
                m->priority = diff*RAW_PRI_IMU_GYRO;
                break;
            default:
                Serial.println("unknown type\n");
        };
        // Serial.print("\ti ");Serial.print(m->type);
        // Serial.print(" val ");Serial.print(rawPrevious[m->type]);Serial.print("=>");Serial.print(m->value);
        // Serial.print(" d ");Serial.print(diff);Serial.print(" pri ");Serial.println(m->priority);
    }
    qsort(rawQueue, rawQueueSize, sizeof(raw_message), compareMessages);
    for(int q = 0; q < min(rawQueueSize, rawThrottle); ++q) {
        raw_message* m = rawQueue + q;
        // Serial.print("\t>> i ");Serial.println(m->type);
        sendRawQueueElement(rawQueue + q);
    }
    clearRawQueue();
}
