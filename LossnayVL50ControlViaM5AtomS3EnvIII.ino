#include <Arduino.h>
#include <M5AtomS3.h>
#include "M5_ENV.h"
#include <HeatpumpIR.h>
#include "CountDown.h"
#include <SparkFun_FS3000_Arduino_Library.h>

// This sketch is heavily based on the followingsketches:
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

CountDown CD(CountDown::SECONDS);

void setup() {
  M5.begin();
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
  CD.start(3);
}

void loop() {
  if (sht30.get() == 0) {
    tmp = sht30.cTemp;
    hum = sht30.humidity;
  } else {
    tmp = 0, hum = 0;
  }

  String fanStatus = "Fan ~";
  if (hum > 65) {
    if (flowsensor.readRaw() < 1200) { // if it is on low
      sendRaw(fan_high);
      fanStatus = "Set Fan +";
      delay(5000);
    } else {
      updateLCD(tmp, hum, flowsensor.readRaw(), fanStatus);
      delay(5000);
    }
    CD.start(600);
  } else if ((hum < 65) && hum > 45) {
    if ((flowsensor.readRaw() < 600) || (flowsensor.readRaw() > 1200)) { // if it is on off or on high
      sendRaw(fan_low);
      fanStatus = "Set Fan -";
      delay(5000);
    } else {
      updateLCD(tmp, hum, flowsensor.readRaw(), fanStatus);
      delay(5000);
    }
    CD.start(60);
  } else if (hum < 45) { // Humidity below 45%, turn fan off
      if (flowsensor.readRaw() > 600) { // if it is on low
        sendRaw(fan_off);
        fanStatus = "Set Fan OFF";
        delay(5000);
      } else {
        updateLCD(tmp, hum, flowsensor.readRaw(), fanStatus);
        delay(5000);
    }
    CD.start(60);
  }

  while (CD.remaining() > 0) {
    updateLCD(tmp, hum, flowsensor.readRaw(), "Wait " + String(CD.remaining()) + "s");
    delay(1000);
  }

  updateLCD(tmp, hum, flowsensor.readRaw(), fanStatus);
  M5.update();
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

// Function to update LCD display
void updateLCD(float temperature, float humidity, int flowReading, String fanStatus) {
  AtomS3.Lcd.fillRect(3, 0, 135, 25, BLACK);
  AtomS3.Lcd.drawString(String(temperature) + " deg  ", 3, 0);

  AtomS3.Lcd.fillRect(3, 25, 135, 25, BLACK);
  AtomS3.Lcd.drawString(String(humidity) + " %  ", 3, 25);

  AtomS3.Lcd.fillRect(3, 50, 135, 25, BLACK);
  AtomS3.Lcd.drawString(String(flowReading) + " dec  ", 3, 50);

  AtomS3.Lcd.fillRect(3, 75, 135, 25, BLACK);
  AtomS3.Lcd.drawString(fanStatus, 3, 75);
}
