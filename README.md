# Domoticz Sensor

## Project goals
This project will use the [MXCHIP Iot DevKit](https://aka.ms/iot-devkit) to create 4 sensors:
- [ ] Pressure
- [ ] Humidity
- [ ] Temperature
- [ ] Presence

### Pressure, Humidity, Temperature
The *MXChip* has these sensors onborad, so I'll use them directly.

### Presence
I want to be able to start some scenario if someone is present in the house.

I'll use the microphone from the *MXChip* board to detect activity in the house.

## Environement setup
You'll need a [MXCHIP Iot DevKit](https://aka.ms/iot-devkit)

### Steps to start

1. Setup development environment by following [Get Started](https://microsoft.github.io/azure-iot-developer-kit/docs/get-started/)
2. Open VS Code
3. Clone the [Domoticz project](https://github.com/amennelet/DomoticzSensor)

### Upload Arduino Code to DevKit

1. Connect your DevKit to your machine.
2. Press **F1** or **Ctrl + Shift + P** in Visual Studio Code - **IoT Workbench:Device** and click **Device Upload**.
3. Wait for Arduino code uploading.