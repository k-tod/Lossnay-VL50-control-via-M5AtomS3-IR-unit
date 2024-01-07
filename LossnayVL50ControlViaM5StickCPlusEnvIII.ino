#include <Arduino.h>
#include <M5StickCPlus.h>
#include "M5_ENV.h"
#include <HeatpumpIR.h>
#include "CountDown.h"
#include <SparkFun_FS3000_Arduino_Library.h> //Click here to get the library: http://librarymanager/All#SparkFun_FS3000
#include "AXP192.h"

// This sketch is heavily based on the followingsketches: 
// - https://github.com/ToniA/arduino-heatpumpir/blob/master/examples/rawsender/rawsender.ino
// - https://github.com/sparkfun/SparkFun_FS3000_Arduino_Library/tree/main/examples/Example01_BasicReadings
//   - you can also use another flowSensor from Renesas, like FS1015, 
//     - to do that , you would have to change the FS3000_DEVICE_ADDRESS to 0x50 in "SparkFun_FS3000_Arduino_Library.h"



IRSenderESP32 irSender(32, 0);     // IR led on M5Stack IR Unit()SKU:U002, which is connected via a grove connector to ESP22 digital pin 32, 0 - ESP32 LEDC channel.
                                   // more info at: https://docs.m5stack.switch-science.com/en/unit/ir 


// This sketch can be used to send 'raw' IR signals, like this:
// Hh001101011010111100000100101001010100000000000111000000001111111011010100000001000100101000
//
// Adjust these timings based on the heatpump / A/C model
#define IR_PAUSE_SPACE  0
#define IR_HEADER_MARK  3220
#define IR_HEADER_SPACE 1584
#define IR_BIT_MARK     411
#define IR_ZERO_SPACE   378
#define IR_ONE_SPACE    1180

SHT3X sht30;
QMP6988 qmp6988;
FS3000 flowsensor;


float tmp      = 0.0;
float hum      = 0.0;
float pressure = 0.0;

CountDown CD(CountDown::SECONDS);

void setup()
{
  Serial.begin(115200);
  delay(500);
  M5.begin();
  // M5.Lcd.setRotation(1); landscape
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 10);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);
  Wire.begin(0, 26);
  qmp6988.init();
  sht30.init();
  CD.start(3);
  if (flowsensor.begin() == false) //Begin communication over I2C
  {
    Serial.println("The Flow sensor did not respond. Please check wiring.");
    delay(5000);
  }
  // Set the range to match which version of the sensor you are using.
  flowsensor.setRange(AIRFLOW_RANGE_7_MPS);
  M5.Axp.EnableCoulombcounter();  // Enable Coulomb counter.
  Serial.println(F("Starting..."));
}

void loop()
{ 
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 10);
  M5.Lcd.printf("ReadRaw: %d  \r\n", flowsensor.readRaw());
  M5.Lcd.printf("m/s: %.2f  \r\n", flowsensor.readMetersPerSecond());
  M5.Lcd.printf("mph: %.2f  \r\n", flowsensor.readMilesPerHour());
  M5.Lcd.printf("Bat: %.3fV I: %.2fmA\r\n", M5.Axp.GetBatVoltage(), M5.Axp.GetBatCurrent());
  delay(250);
  char fan_high[1024] = "Hh1100010011010011011010000010001010000000100100010101000000001101";
  char fan_low[1024] = "Hh1100010011010011011010000010001010000000100100011101000000000101";
  pressure = qmp6988.calcPressure();
    if (sht30.get() == 0) {  // Obtain the data of shT30.
        tmp = sht30.cTemp;   // Store the temperature obtained from shT30.
        hum = sht30.humidity;  // Store the humidity obtained from the
    } else {
        tmp = 0, hum = 0;
    }
    M5.lcd.fillRect(0, 20, 100, 60, BLACK);  // Fill the screen with black (to clear the screen).
    M5.lcd.setCursor(0, 20);
    M5.Lcd.printf("Temp: %2.1f  \r\nHum:%2.0f%%", tmp, hum);
    
    if (hum > 65) {
      if ((flowsensor.readRaw() < 1200) && (flowsensor.readRaw() > 600)){
        sendRaw(fan_high);
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setCursor(0, 10);
        M5.Lcd.printf("Temp: %2.1f  \r\nHum:%2.0f%%\r\nSet Fan +  \r\nFlow: %d  \r\n", tmp, hum, flowsensor.readRaw());
        delay(5000);
      }else{
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setCursor(0, 10);
        M5.Lcd.printf("Temp: %2.1f  \r\nHum:%2.0f%%\r\nFan ~  \r\nFlow: %d \r\n", tmp, hum, flowsensor.readRaw());
        delay(5000);
      }
      CD.start(600); // 10 minutes
      while (CD.remaining() > 0 ){
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setCursor(0, 10);
        M5.Lcd.printf("Temp: %2.1f  \r\nHum:%2.0f%%\r\nFlow: %d  \r\nBat: %.3fV\r\nI: %.2fmA\r\n", tmp, hum, flowsensor.readRaw(), M5.Axp.GetBatVoltage(), M5.Axp.GetBatCurrent());
        M5.Lcd.printf("Wait: %u \r\n", CD.remaining());
        delay(1000);
        }

    } else {
      if (flowsensor.readRaw() > 1200){
      sendRaw(fan_low);
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setCursor(0, 10);
      M5.Lcd.printf("Temp: %2.1f  \r\nHum:%2.0f%%\r\nSet Fan -  \r\nFlow: %d  \r\n", tmp, hum, flowsensor.readRaw());
      delay(5000);
      }else{
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setCursor(0, 10);
        M5.Lcd.printf("Temp: %2.1f  \r\nHum:%2.0f%%\r\nFan ~ \r\nFlow: %d  \r\n", tmp, hum, flowsensor.readRaw());
        delay(5000);
      }
      CD.start(300); // 5 minutes
      while (CD.remaining() > 0 ){
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setCursor(0, 10);
        M5.Lcd.printf("Temp: %2.1f  \r\nHum:%2.0f%%\r\nFlow: %d  \r\nBat: %.3fV\r\nI: %.2fmA\r\n", tmp, hum, flowsensor.readRaw(), M5.Axp.GetBatVoltage(), M5.Axp.GetBatCurrent());
        M5.Lcd.printf("Wait: %u \r\n", CD.remaining());
        delay(1000);
        }
  }

  M5.update(); // refresh button states etc
}

void sendRaw(char *symbols)
{
  irSender.space(0);
  irSender.setFrequency(38);

  while (char symbol = *symbols++)
  {
    switch (symbol)
    {
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
