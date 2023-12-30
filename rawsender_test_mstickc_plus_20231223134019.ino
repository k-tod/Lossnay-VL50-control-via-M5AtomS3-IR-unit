#include <Arduino.h>
#include <M5StickCPlus.h>
#include "M5_ENV.h"
#include <HeatpumpIR.h>
#include "CountDown.h"
#include "PinDefinitionsAndMore.h"

#ifndef ESP8266
IRSenderESP32 irSender(32, 0);     // IR led on ESP22 digital pin 32, 0 - ESP32 LEDC channel. 
// IRSenderPWM irSender(9);       // IR led on Arduino digital pin 9, using Arduino PWM
// IRSenderBlaster irSender(3); // IR led on Arduino digital pin 3, using IR Blaster (generates the 38 kHz carrier)
#else
IRSenderBitBang irSender(D1);  // IR led on Wemos D1 mini pin 'D1'
#endif


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
bool fanState = false; // Initialize fan state to low

SHT3X sht30;
QMP6988 qmp6988;

float tmp      = 0.0;
float hum      = 0.0;
float pressure = 0.0;

CountDown CD(CountDown::SECONDS);

uint32_t start, stop;


void setup()
{
  Serial.begin(115200);
  delay(500);
  M5.begin();
  M5.Lcd.setRotation(1);
  // text print
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 10);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);
  Wire.begin(0, 26);
  qmp6988.init();
  sht30.init();
  start = millis();
  CD.start(3);
  Serial.println(start);

  Serial.println(F("Starting..."));
}

void loop()
{     
  char fan_high[1024] = "Hh1100010011010011011010000010001010000000100100010101000000001101";
  char fan_low[1024] = "Hh1100010011010011011010000010001010000000100100011101000000000101";
  pressure = qmp6988.calcPressure();
    if (sht30.get() == 0) {  // Obtain the data of shT30.  获取sht30的数据
        tmp = sht30.cTemp;   // Store the temperature obtained from shT30.
                             // 将sht30获取到的温度存储
        hum = sht30.humidity;  // Store the humidity obtained from the
                               // SHT30. 将sht30获取到的湿度存储
    } else {
        tmp = 0, hum = 0;
    }
    M5.lcd.fillRect(0, 20, 100, 60,
                    BLACK);  // Fill the screen with black (to clear the
                             // screen).  将屏幕填充黑色(用来清屏)
    M5.lcd.setCursor(0, 20);
    M5.Lcd.printf("Temp: %2.1f  \r\nHumi: %2.0f%%  \r\n", tmp, hum);
    
    if (hum > 65) {
      sendRaw(fan_high);
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setCursor(0, 10);
      M5.Lcd.printf("Temp: %2.1f  \r\nHumi: %2.0f%%  \r\nFanHigh  \r\n", tmp, hum);
      CD.start(600); // 10 minutes
      while (CD.remaining() > 0 ){
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setCursor(0, 10);
        M5.Lcd.printf("Temp: %2.1f  \r\nHumi: %2.0f%%  \r\nFanHigh  \r\n", tmp, hum);
        M5.Lcd.printf("Wait: %u \r\n", CD.remaining());
        delay(1000);
        }

    } else {
      sendRaw(fan_low);
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setCursor(0, 10);
      M5.Lcd.printf("Temp: %2.1f  \r\nHumi: %2.0f%%  \r\nFanLow  \r\n", tmp, hum);
      CD.start(300); // 5 minutes
      while (CD.remaining() > 0 ){
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setCursor(0, 10);
        M5.Lcd.printf("Temp: %2.1f  \r\nHumi: %2.0f%%  \r\nFanLow  \r\n", tmp, hum);
        M5.Lcd.printf("Wait: %u \r\n", CD.remaining());
        delay(1000);
        }
  }

    // delay(60000); // wait 1 minute
    CD.start(60); // 5 minutes
    while (CD.remaining() > 0 ){
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setCursor(0, 10);
      M5.Lcd.printf("Temp: %2.1f  \r\nHumi: %2.0f%%  \r\nFanLow  \r\n", tmp, hum);
      M5.Lcd.printf("Wait between checks: %u \r\n", CD.remaining());
      delay(1000);
    }

  // ####################################################################################################


  // if (M5.BtnA.wasPressed()) {
  //   char power_on_off[1024] = "Hh1100010011010011011010000010001010000000100100010000000000001000";
  //   sendRaw(power_on_off);
  //   M5.Lcd.fillScreen(BLACK);
  //   M5.Lcd.setCursor(0, 10);
  //   M5.Lcd.printf("power_on_off");
  //   Serial.println(F("power_on_off"));
  // }

  // delay(100);

  // if (M5.BtnB.wasPressed()) {
  //   // Toggle the fan state
  //   fanState = !fanState;
  //   char fan_high[1024] = "Hh1100010011010011011010000010001010000000100100010101000000001101";
  //   char fan_low[1024] = "Hh1100010011010011011010000010001010000000100100011101000000000101";
  //   if (fanState) {
  //     sendRaw(fan_high);
  //     M5.Lcd.fillScreen(BLACK);
  //     M5.Lcd.setCursor(0, 10);
  //     M5.Lcd.printf("fan_high");
  //     Serial.println(F("fan_high"));
  //   } else {
  //     sendRaw(fan_low);
  //     M5.Lcd.fillScreen(BLACK);
  //     M5.Lcd.setCursor(0, 10);
  //     M5.Lcd.printf("fan_low");
  //     Serial.println(F("fan_low"));
  //   }
  // }
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
