# M5Stack Temperature and Humidity Control of Lossnay-VL50

This Arduino sketch is designed for an M5Stack device with temperature and humidity sensors. It allows you to control a fan based on the temperature and humidity readings. 
When the humidity is above a certain threshold, it turns the fan on to a high setting, and when the humidity is below that threshold, it switches to a low setting. 
The code also uses infrared (IR) signals to control the fan and a feedback loop implemented to check if the IR command was succesfull, via a Airflow sensor(Renesas FS1015)

## Hardware Used

- [M5Stack ATOMS3](https://docs.m5stack.com/en/core/AtomS3)
- [M5Stack Unit IR](https://docs.m5stack.com/en/unit/ir)
- [M5Stack Unit ENV-IV](https://docs.m5stack.com/en/unit/ENV%E2%85%A3%20Unit)
- [M5Stack ATOMIC PortABC Extension Base](https://docs.m5stack.com/en/unit/AtomPortABC)
  - port.B connections are interrupted and the connector is re-routed in parallel with port.A connections, so that we can connect both sensors(Airflow and ENV-IV)  on the same I2C line
- [Renesas FS1015](https://www.renesas.com/en/products/sensor-products/flow-sensors/fs1015-air-velocity-sensor-module)
- 5V Power Supply(~500mA)


## Usage

1. Upload this Arduino sketch to your M5AtomS3 device(making sure all required libraries are installed).
2. Connect the other hardware components to your M5Stack device.
3. Make sure the flowsensor is close enough to sense change in airflow of the Lossnay VL-50 unit.
4. Make sure the IR unit has direct line of sight towards the Lossnay VL-50 unit 
5. Power On the hardware setup.
6. From this point on, M5AtomS3 setup will monitor the temperature and humidity.
   - When the humidity exceeds a certain threshold (65% for example), it will activate the fan on a high setting via IR command. Otherwise, it will run the fan on a low setting.
   - When humidity is decreasing below a low threshold(55% for example), it will stop the fan and wait for humidity to increase to at least 5 units above the low threshold.
   - When the user is changing the fan state manually, via the original remote, the M5AtomS3 setup will suppress automatic control for x minutes.
   - User can increase the original threshold by pressing the M5AtomS3 Display button in range +0 .. +15% to compensate for natural humidity depending on the season.

7. The display will show temperature, humidity, raw airflow value, fan status and current threshold setting.

8. Adjust the thresholds and timings in the code as needed for your specific application.

## License

This code is provided under the [MIT License](LICENSE.md).
