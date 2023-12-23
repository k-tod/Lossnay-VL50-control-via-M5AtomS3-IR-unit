# M5Stack Temperature and Humidity Control of Lossnay-VL50

This Arduino sketch is designed for an M5Stack device with temperature and humidity sensors. It allows you to control a fan based on the temperature and humidity readings. 
When the humidity is above a certain threshold, it turns the fan on to a high setting, and when the humidity is below that threshold, it switches to a low setting. The code also uses infrared (IR) signals to control the fan.

## Hardware Requirements

- M5Stack device (specifically M5StickC+)
- Temperature and humidity sensor (SHT3X)
- Pressure sensor (QMP6988)
- Infrared (IR) sender

## Libraries Used

- [M5StickCPlus](https://github.com/m5stack/M5StickC-Plus)
- [HeatpumpIR](https://github.com/ToniA/arduino-heatpumpir)
- [CountDown](https://github.com/RobTillaart/CountDown)
- [PinDefinitionsAndMore](https://github.com/IRMP-org/IRMP/blob/master/examples/SimpleSender/PinDefinitionsAndMore.h)
  - Additionally, I adapted the ESP32 pins in "PinDefinitionsAndMore.h"  to match the pins of IR unit on my M5StickC-Plus like so:
    - IRMP_INPUT_PIN:   from 15 to 33
    - IRSND_OUTPUT_PIN: from 4 to 32

## Usage

1. Connect the required hardware components to your M5Stack device.

2. Upload this Arduino sketch to your M5Stack device.

3. The sketch will monitor the temperature and humidity. When the humidity exceeds a certain threshold (70% in this example), it will activate the fan on a high setting via IR command. Otherwise, it will run the fan on a low setting.

5. The display will show temperature, humidity, and fan status.

6. Adjust the thresholds and timings in the code as needed for your specific application.

## License

This code is provided under the [MIT License](LICENSE.md).
