#include <Arduino.h>
#include <M5AtomS3.h>
#include "M5_ENV.h"
#include <HeatpumpIR.h>
#include "CountDown.h"
#include <SparkFun_FS3000_Arduino_Library.h>

// The following projects were used as inspiration for creating this particular sketch:
// - https://github.com/ToniA/arduino-heatpumpir/blob/master/examples/rawsender/rawsender.ino
// - https://github.com/sparkfun/SparkFun_FS3000_Arduino_Library/tree/main/examples/Example01_BasicReadings

//   - you can also use another flowSensor from Renesas, like FS1015,
//     - to do that , you would have to change the FS3000_DEVICE_ADDRESS to 0x50 in "SparkFun_FS3000_Arduino_Library.h"

SHT3X sht30;
FS3000 flowsensor;
IRSenderESP32 irSender(39, 0);  // IR led on M5Stack IR Unit()SKU:U002, which is connected via a grove connector to ESP22 digital pin 39, 0 - ESP32 LEDC channel.
                                // more info at: https://docs.m5stack.switch-science.com/en/unit/ir

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
const unsigned long debounceDuration = 5000; // 5 seconds debounce durationv
bool manualOverride = false;
unsigned long manualOverrideStartTime = 0;
const unsigned long manualOverrideDuration = 5 * 60 * 1000; // 5 minutes in milliseconds

void setup() {
  M5.begin();
  Serial.begin(115200);
  Wire.begin(2, 1);
  AtomS3.Lcd.setRotation(2);
  AtomS3.Lcd.fillScreen(BLACK);
  AtomS3.Lcd.setCursor(0, 10);
  AtomS3.Lcd.setTextColor(WHITE);
  AtomS3.Lcd.setTextSize(2);
  Wire.begin(0, 26);
  flowsensor.begin();
  flowsensor.setRange(AIRFLOW_RANGE_7_MPS);
  sht30.init();
}

void loop() {
  M5.update();
  unsigned long currentMillis = millis();

  if (AtomS3.BtnA.wasPressed()) {
    handleButtonPress();
  }

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    if (sht30.get() == 0) {
      tmp = sht30.cTemp;
      hum = sht30.humidity;
    } else {
      tmp = 0, hum = 0;
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
  if (hum > highThreshold) {
    if (fanState != FAN_HIGH) {
      setFanState(FAN_HIGH);
    }
  } else if (hum > lowThreshold) {
    if (fanState != FAN_LOW) {
      setFanState(FAN_LOW);
    }
  } else {
    if (fanState != FAN_OFF) {
      setFanState(FAN_OFF);
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

  if (currentFanState != fanState) {
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
    thresholdInfo += " Man";
  }

  AtomS3.Lcd.fillRect(3, 100, 135, 25, BLACK);
  AtomS3.Lcd.drawString(thresholdInfo, 3, 100);
}
