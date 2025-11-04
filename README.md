# LPMK ECE 362 Final Project
#### GPS Timekeeping and Alarm with Music on RP2350

### Parts List
- RP2350 Proton Board and Debugger
- <span style="color: #777">~~DS Series RTC (I<sup>2</sup>C)~~ (Rejected)</span>
- NEO-6M GPS Module (UART) for Satellite Timekeeping
- LCD TFT Display (SPI) with SD Card Reader (SPI) for Image Storage/Display + Audio Storage
- External DAC (I2C or Custom Build with Shift-Reg) for Audio Playback
- Passive Button Array Keypad for Navigation

### Functions
- Current Time
- GPS Map
- Alarm Settings
- Stopwatch
- Timer

### Project Tasks
1. Define Objectives
    - Get GTA approval
    - Agree on design
    - Assign roles
    - Who should keep the design?
    - Agree on meeting / build times
2. High-level planning
    - Hardware Diagram
    - Software Interface Diagram
3. Detailed Hardware Design
    - Determine pin allocation via GPIO needs
    - Determine required bus speeds via part datasheets
    - Design custom peripherals (ie. DAC)
4. Hardware Prototype
    - Breadboard Build
    - Electrical Verification
5. Software Build
    - Hardware Peripheral Backends
        - Create high-level functions for controlling peripherals easily (like in `display.c`)
        - A way to communicate with frontend
    - Application Frontend
        - Interface and Transition Logic
        - Graphics and UX Design
6. Iterate and Debug
7. PCB Build (if time available)
8. Report / Presentations