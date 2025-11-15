#include <Arduino.h>
#include <M5AtomS3.h>
#include "M5UnitENV.h"
#include <HeatpumpIR.h>
#include "CountDown.h"
#include <SparkFun_FS3000_Arduino_Library.h>

// The following projects were used as inspiration for creating this particular sketch:
// - https://github.com/ToniA/arduino-heatpumpir/blob/master/examples/rawsender/rawsender.ino
// - https://github.com/sparkfun/SparkFun_FS3000_Arduino_Library/tree/main/examples/Example01_BasicReadings

//   - you can also use another flowSensor from Renesas, like FS1015,
//     - to do that , you would have to change the FS3000_DEVICE_ADDRESS to 0x50 in "SparkFun_FS3000_Arduino_Library.h"


SHT4X sht4;
FS3000 flowsensor;
IRSenderESP32 irSender(2, 1);  // IR led on M5Stack IR Unit(SKU:U002)

// IR communication timings specific to Lossnay VL-50
#define IR_PAUSE_SPACE 0
#define IR_HEADER_MARK 3220
#define IR_HEADER_SPACE 1584
#define IR_BIT_MARK 411
#define IR_ZERO_SPACE 378
#define IR_ONE_SPACE 1180

float tmp = 0.0;
float hum = 0.0;
char fan_high[1024] = "Hh1100010011010011011010000010001010000000100100010101000000001101";
char fan_low[1024] = "Hh1100010011010011011010000010001010000000100100011101000000000101";
char fan_off[1024] = "Hh1100010011010011011010000010001010000000100100010000000000001000";

unsigned long previousMillis = 0;
const long interval = 5000;

enum FanState {
  FAN_OFF,
  FAN_LOW,
  FAN_HIGH
};

FanState fanState = FAN_OFF;
// Variable to track if the fan was recently turned off
bool fanRecentlyTurnedOff = false;

const int BUTTON_PRESS_INTERVAL = 500; // Maximum time between button presses to count as a single press
unsigned long lastButtonPress = 0;
int buttonPressCount = 0;

// Default thresholds
const float DEFAULT_HIGH_THRESHOLD = 65.0;
const float DEFAULT_LOW_THRESHOLD = 45.0;

// Configurable thresholds
float highThreshold = DEFAULT_HIGH_THRESHOLD;
float lowThreshold = DEFAULT_LOW_THRESHOLD;

// Manual override management
bool autoHighState = false;
unsigned long lastAutoHighTime = 0; // To track when auto high was last triggered
const unsigned long debounceDuration = 10000; // 10 seconds debounce duration
bool manualOverride = false;
unsigned long manualOverrideStartTime = 0;
const unsigned long manualOverrideDuration = 5 * 60 * 1000; // 5 minutes in milliseconds

void setup() {
  M5.begin();
  Serial.begin(115200);
  AtomS3.Lcd.fillScreen(BLACK);
  AtomS3.Lcd.setCursor(0, 10);
  AtomS3.Lcd.setTextColor(WHITE);
  AtomS3.Lcd.setTextSize(2);
  Wire.begin(0, 26);
  flowsensor.begin();
  flowsensor.setRange(AIRFLOW_RANGE_7_MPS);
  if (!sht4.begin(&Wire, SHT40_I2C_ADDR_44, 38, 39, 400000U)) {
    Serial.println("Couldn't find SHT4x");
    while (1) delay(1);
  }
  // You can have 3 different precisions, higher precision takes longer
  sht4.setPrecision(SHT4X_HIGH_PRECISION);
  sht4.setHeater(SHT4X_NO_HEATER);
}

void loop() {
  M5.update();
  unsigned long currentMillis = millis();

  if (AtomS3.BtnA.wasPressed()) {
    handleButtonPress();
  }

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    if (sht4.update()) {
      tmp = sht4.cTemp;
      hum = sht4.humidity;
    }

    if (!manualOverride) {
      updateFanState();
    } else if (currentMillis - manualOverrideStartTime >= manualOverrideDuration) {
      manualOverride = false; // Disable manual override after the duration
      updateFanState(); // Update fan state after manual override ends
    }
    updateLCD(tmp, hum, flowsensor.readRaw(), getFanStatus());
  }
  checkFanState();
}

void handleButtonPress() {
  buttonPressCount = (buttonPressCount + 1) % 4;

  switch (buttonPressCount) {
    case 0:
      highThreshold = DEFAULT_HIGH_THRESHOLD;
      lowThreshold = DEFAULT_LOW_THRESHOLD;
      break;
    case 1:
      highThreshold = DEFAULT_HIGH_THRESHOLD + 5;
      lowThreshold = DEFAULT_LOW_THRESHOLD + 5;
      break;
    case 2:
      highThreshold = DEFAULT_HIGH_THRESHOLD + 10;
      lowThreshold = DEFAULT_LOW_THRESHOLD + 10;
      break;
    case 3:
      highThreshold = DEFAULT_HIGH_THRESHOLD + 15;
      lowThreshold = DEFAULT_LOW_THRESHOLD + 15;
      break;
  }

  Serial.print("Thresholds updated: High=");
  Serial.print(highThreshold);
  Serial.print(" Low=");
  Serial.println(lowThreshold);
}

void updateFanState() {
  const float hysteresisMargin = 5.0; // Hysteresis margin to prevent rapid toggling

  // If the fan is OFF, check if it should restart
  if (fanState == FAN_OFF) {
    if (fanRecentlyTurnedOff) {
      // Enforce hysteresis: Restart only if humidity exceeds lowThreshold + hysteresisMargin
      if (hum > lowThreshold + hysteresisMargin) {
        setFanState(FAN_LOW); // Restart fan in LOW mode
        fanRecentlyTurnedOff = false; // Reset hysteresis flag
      }
    } else {
      // No hysteresis flag? Directly check thresholds to start fan
      if (hum > lowThreshold) {
        setFanState(FAN_LOW); // Start fan in LOW mode
      }
    }
  } else if (fanState == FAN_LOW) {
    // If the fan is in LOW mode, check thresholds for HIGH or OFF
    if (hum > highThreshold + hysteresisMargin) {
      setFanState(FAN_HIGH); // Switch to HIGH mode
    } else if (hum < lowThreshold - hysteresisMargin) {
      setFanState(FAN_OFF); // Turn fan OFF
      fanRecentlyTurnedOff = true; // Mark the fan as recently turned off
    }
  } else if (fanState == FAN_HIGH) {
    // If the fan is in HIGH mode, check thresholds for LOW
    if (hum < highThreshold - hysteresisMargin) {
      setFanState(FAN_LOW); // Switch to LOW mode
    }
  }
}

void setFanState(FanState newState) {
  autoHighState = true;
  lastAutoHighTime = millis(); // Record the current time
  fanState = newState;
  switch (fanState) {
    case FAN_OFF:
      sendRaw(fan_off);
      break;
    case FAN_LOW:
      sendRaw(fan_low);
      break;
    case FAN_HIGH:
      sendRaw(fan_high);
      break;
  }
}

void checkFanState() {
  // Check if enough time has passed since the last auto high trigger
  if (autoHighState && (millis() - lastAutoHighTime < debounceDuration)) {
      // Still within debounce duration, skip manual override detection
      return;
  }
  int airflow = flowsensor.readRaw();
  Serial.print("Airflow: ");
  Serial.println(airflow);

  FanState currentFanState;
  if (airflow > 1200) {
    currentFanState = FAN_HIGH;
  } else if (airflow > 600) {
    currentFanState = FAN_LOW;
  } else {
    currentFanState = FAN_OFF;
  }

  if (currentFanState != fanState && !fanRecentlyTurnedOff) {
    manualOverride = true;
    manualOverrideStartTime = millis();
    fanState = currentFanState;
    autoHighState = false; // Only reset here after confirming the override

    Serial.println("Manual override detected");
  }
}

String getFanStatus() {
  switch (fanState) {
    case FAN_OFF:
      return "Fan OFF";
    case FAN_LOW:
      return "Fan LOW";
    case FAN_HIGH:
      return "Fan HIGH";
  }
  return "Fan ~";
}

void sendRaw(char *symbols) {
  irSender.space(0);
  irSender.setFrequency(38);

  while (char symbol = *symbols++) {
    switch (symbol) {
      case '1':
        irSender.space(IR_ONE_SPACE);
        irSender.mark(IR_BIT_MARK);
        break;
      case '0':
        irSender.space(IR_ZERO_SPACE);
        irSender.mark(IR_BIT_MARK);
        break;
      case 'W':
        irSender.space(IR_PAUSE_SPACE);
        irSender.mark(IR_BIT_MARK);
        break;
      case 'H':
        irSender.mark(IR_HEADER_MARK);
        break;
      case 'h':
        irSender.space(IR_HEADER_SPACE);
        irSender.mark(IR_BIT_MARK);
        break;
    }
  }

  irSender.space(0);
}

void updateLCD(float temperature, float humidity, int flowReading, String fanStatus) {
  AtomS3.Lcd.fillRect(3, 0, 135, 25, BLACK);
  AtomS3.Lcd.drawString(String(temperature) + " deg  ", 3, 0);

  AtomS3.Lcd.fillRect(3, 25, 135, 25, BLACK);
  AtomS3.Lcd.drawString(String(humidity) + " %  ", 3, 25);

  AtomS3.Lcd.fillRect(3, 50, 135, 25, BLACK);
  AtomS3.Lcd.drawString(String(flowReading) + " dec  ", 3, 50);

  AtomS3.Lcd.fillRect(3, 75, 135, 25, BLACK);
  AtomS3.Lcd.drawString(fanStatus, 3, 75);

  String thresholdInfo = "";
  switch (buttonPressCount) {
    case 0:
      thresholdInfo = "+0%";
      break;
    case 1:
      thresholdInfo = "+5%";
      break;
    case 2:
      thresholdInfo = "+10%";
      break;
    case 3:
      thresholdInfo = "+15%";
      break;
  }

  if (manualOverride) {
    thresholdInfo = "Man Override";
  }
  else if(fanRecentlyTurnedOff){
    thresholdInfo = thresholdInfo + "Hysteresis";
  }
  else{
    thresholdInfo = thresholdInfo;
  }


  AtomS3.Lcd.fillRect(3, 100, 135, 25, BLACK);
  AtomS3.Lcd.drawString(thresholdInfo, 3, 100);
}
