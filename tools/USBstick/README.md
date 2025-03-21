# Windows Software for TicWave Solution Camera TWS-ID02

## Overview
This software is used for controlling the TicWave Solution Camera TWS-ID02 and includes:

- Firmware: TWS_ID02 Rev3a

- CLI (Command Line Interface) Tool

## Documentation
Before you start, please read the user manual: `Usermanual_TW_ID02_Rev2_v1.pdf`.

## Installation Instructions

### Windows (Recommended)
1. Execute `Setup.exe` from the USB stick (located in the windows sub-folder).
2. The default installation path is `C:\Program Files (x86)\TicWave\CAMctrl`.

### Linux
1. Copy the binary `CAMctrl` from the USB stick (located in the linux sub-folder) to your local disk.

## Connecting the Camera
1. Connect a USB-C cable for communication and power supply (note: battery not installed).
2. Connect the chopper X-link cable between the source and the camera. The connector receptors are located on the lens part of the Camera, not the rear part closer to the display.

## Usage

### Windows
1. Open Windows PowerShell (Run as Administrator - required to create `.mat` file).
2. Navigate to the installation directory:
    ```bash
    cd "C:\Program Files (x86)\CAMctrl\TicWave"
    ```
3. Generate a command configuration file:
    ```bash
    .\CAMctrl.exe -Update
    ```
4. Command help:
    ```bash
    .\CAMctrl.exe -h
    ```
5. Examples:
    - Stream for 10 seconds:
      ```bash
      .\CAMctrl.exe -Stream 10
      ```
    - Save a 100 frame video to a `.mat` file:
      ```bash
      .\CAMctrl.exe -GetMAT 100
      ```

### Linux
1. Open a command line terminal.
2. Execute:
    ```bash
    ./CAMctrl
    ```
3. Example:
    ```bash
    ./CAMctrl -h
    ```

## Troubleshooting Driver Installation

### Windows
1. Open Windows Device Manager.
2. Ensure the CP2104 USB to UART Bridge Controller is installed and active.

### Linux

- Check if the USB device is attached:
    ```bash
    lsusb | grep "Silicon Labs CP210x UART Bridge"
    ```

- Check if the serial port `/dev/ttyUSB0` is available:
    ```bash
    ls -l /dev/ttyUSB0
    ```
