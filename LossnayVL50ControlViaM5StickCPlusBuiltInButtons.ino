#include <Arduino.h>
#include <M5StickCPlus.h>
#include <HeatpumpIR.h>

// This sketch is heavily based on : https://github.com/ToniA/arduino-heatpumpir/blob/master/examples/rawsender/rawsender.ino


// This sketch can be used to send 'raw' IR signals, like this:
// Hh001101011010111100000100101001010100000000000111000000001111111011010100000001000100101000


IRSenderESP32 irSender(32, 0);     // IR led on M5Stack IR Unit()SKU:U002, which is connected via a grove connector to ESP22 digital pin 32, 0 - ESP32 LEDC channel.
                                   // more info at: https://docs.m5stack.switch-science.com/en/unit/ir

// Adjust these timings based on the heatpump / A/C model
#define IR_PAUSE_SPACE  0
#define IR_HEADER_MARK  3220
#define IR_HEADER_SPACE 1584
#define IR_BIT_MARK     411
#define IR_ZERO_SPACE   378
#define IR_ONE_SPACE    1180
bool fanState = false; // Initialize fan state to low


void setup()
{
  Serial.begin(115200);
  delay(500);
  M5.begin();
  // M5.Lcd.setRotation(1); // landscape mode
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 10);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);
}

void loop()
{ 
  if (M5.BtnA.wasPressed()) {
    char power_on_off[1024] = "Hh1100010011010011011010000010001010000000100100010000000000001000";
    sendRaw(power_on_off);
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 10);
    M5.Lcd.printf("power_on_off");
    Serial.println(F("power_on_off"));
  }

  delay(100);

  if (M5.BtnB.wasPressed()) {
    // Toggle the fan state
    fanState = !fanState;
    char fan_high[1024] = "Hh1100010011010011011010000010001010000000100100010101000000001101";
    char fan_low[1024] = "Hh1100010011010011011010000010001010000000100100011101000000000101";
    if (fanState) {
      sendRaw(fan_high);
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setCursor(0, 10);
      M5.Lcd.printf("fan_high");
      Serial.println(F("fan_high"));
    } else {
      sendRaw(fan_low);
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setCursor(0, 10);
      M5.Lcd.printf("fan_low");
      Serial.println(F("fan_low"));
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
