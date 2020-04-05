/*
 * Door sensor using IR LED + IR phototransistor.
 */

int doorLastTimeMillis = 0;
bool irLedState = false;
long accumulator = 0;
int numValues = 0;
long highAvg;
int doorSensorState;
int doorSensorFilter;

#define DOOR_SENSOR_SAMPLE_PERIOD 50

#define DOOR_SENSOR_FILTER_NUM 80
#define DOOR_SENSOR_FILTER_DENOM 100

#define DOOR_SENSOR_INACTIVE 0
#define DOOR_SENSOR_CLOSED 1
#define DOOR_SENSOR_OPEN 2

void doorSensorInit() {
  pinMode(0, OUTPUT);
  digitalWrite(0, LOW);
  irLedState = false;
  accumulator = 0;
  numValues = 0;
  if (doorSensorEnabled) {
    doorSensorFilter = analogRead(A0);
    doorSensorState = (doorSensorFilter > doorClosedThres ? DOOR_SENSOR_CLOSED : DOOR_SENSOR_OPEN);
  } else {
    doorSensorFilter = 0;
    doorSensorState = DOOR_SENSOR_INACTIVE;
  }
}

void doorSensorLoop() {
  if (!doorSensorEnabled) {
    return;
  }

  int now = millis();
  if (now - doorLastTimeMillis < DOOR_SENSOR_SAMPLE_PERIOD) {
    return;
  }

  doorLastTimeMillis = now;

  int sensorValue = analogRead(A0);

  accumulator += sensorValue;
  numValues++;
  if (irLedState) {
    digitalWrite(0, LOW);
    irLedState = false;
    highAvg = accumulator / numValues;
    accumulator = 0;
    numValues = 0;
  } else if (!irLedState) {
    digitalWrite(0, HIGH);
    irLedState = true;

    long lowAvg = accumulator / numValues;
    accumulator = 0;
    numValues = 0;

    doorSensorFilter =
      doorSensorFilter * DOOR_SENSOR_FILTER_NUM / DOOR_SENSOR_FILTER_DENOM +
      abs(highAvg - lowAvg) * (DOOR_SENSOR_FILTER_DENOM - DOOR_SENSOR_FILTER_NUM) / DOOR_SENSOR_FILTER_DENOM;
    // Serial.print("DS ");
    // Serial.print(doorSensorFilter);
    // Serial.print(" LA ");
    // Serial.print(lowAvg);
    // Serial.print(" HA ");
    // Serial.print(highAvg);
    // Serial.println();

    switch (doorSensorState) {
    case DOOR_SENSOR_CLOSED:
      if (doorSensorFilter < doorClosedThres) {
        Serial.println("DOOR OPEN!");
        effectCurrent = FX_MODE_STATIC;
        for (int i = 0; i < sizeof(col); i++) { col[i] = doorOpenCol[i]; }
        bri = doorOpenBri;
        colorUpdated(NOTIFIER_CALL_MODE_DIRECT_CHANGE);
        doorSensorState = DOOR_SENSOR_OPEN;
      }
      break;
    case DOOR_SENSOR_OPEN:
      if (doorSensorFilter > doorClosedThres) {
        Serial.println("DOOR CLOSED!");
        effectCurrent = FX_MODE_STATIC;
        for (int i = 0; i < sizeof(col); i++) { col[i] = doorClosedCol[i]; }
        bri = doorClosedBri;
        colorUpdated(NOTIFIER_CALL_MODE_DIRECT_CHANGE);
        doorSensorState = DOOR_SENSOR_CLOSED;
      }
      break;
    default:
      Serial.println("DOOR SENSOR STATE UNKNOWN; flipping to INACTIVE :(");
      doorSensorState = DOOR_SENSOR_INACTIVE;
      break;
    }
  }
}
