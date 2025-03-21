# esp-idf stand alone framework (arduino as a component)
0) Install ticcam-idf (TicWave's IOT Development Framework)
git clone --recurse-submodules git@github.com:UllrichPfeiffer/ticcam-idf.git

0.1) Added submodule with
git submodule add git@github.com:UllrichPfeiffer/TicWave-Python-API.git 
under components: git submodule add git@github.com:UllrichPfeiffer/tw-camera.git

1) Install eps-idf manually or use visual studio code extensions (see below) 
cd ~/src/esp
git clone https://github.com/espressif/esp-idf.git
cd esp-idf
git fetch
git checkout v4.4.4
git submodule update --init --recursive
cd ..
mv esp-idf esp-idf-4_4_4
rm -r ~/.espressif
./install.sh 
. ./export.sh

2) Create a project from template or pull from a git  
create tempate: idf.py create-project M5StackSarrayLib
cd M5StackSarrayLib

3) Install wanted components (other libs)
mkdir -p components
cd components
git clone https://github.com/espressif/arduino-esp32.git arduino 
cd arduino 
git submodule update --init --recursive 
cd ../.. 
repeat this for other libs, e.g. M5Core2, lvgl, arduinoFFT
git clone https://github.com/m5stack/M5Core2.git
git clone https://github.com/lvgl/lvgl.git
git clone https://github.com/kosme/arduinoFFT.git

4) Include compulents in top CMakeLists.txt
set(APPEND COMPONENT_REQUIRES "M5Core2")
set(APPEND COMPONENT_REQUIRES "arduinoFFT")
set(APPEND COMPONENT_REQUIRES "lvgl")

4.1) -> you may need to create a CMakeLists.txt file in the other libs to integrate them into this build
5) Create a template sdkconfig file
list possible targets: idf.py set-target
creates the sdkconfig file: idf.py set-target esp32 
replace sdkconfig with orig. core2 file: cp ~/src/esp/m5stack-core2-wasm3-as/sdkconfig . 
kompare sdkconfig sdkconfig.esp32
6) modify sdkconfig 
use: idf.py menuconfig
or: modify manually, e.g. set: CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="large_spiffs_16MB.csv"
- copy other stuff referenced in sdkconfig: cp /home/ullrich/DATEN/ticwave/tw_chips/sources/tw_id01/M5Stack/M5StackSarray_v3/large_spiffs_16MB.csv .
7) build
* idf.py build
- subl CMakeLists.txt and add at end of file: idf_build_set_property(COMPILE_OPTIONS "-Wno-error=address" APPEND)
- fix compilation errors, e.g. in ...
8) flash
idf.py -p /dev/ttyUSB2 flash
monitor: idf.py monitor

# Issues

1) Pin #4 is used for SD card and PSRAM CS
-> reboots all the time if not fixed
- disable sd card init with false: M5.begin(true, false, true, true, kMBusModeInput);
- and/or? set #define TFCARD_CS_PIN -1 to disable this pin, or 16 (or any other pin) in: components/M5Core2/src/utility/Config.h 

2) lvgl: Always use idf.py menuconfig for lvgl settings
- fonts
```
[*] Use a custom tick source in (Top) → Component config → LVGL configuration → HAL Settings 
[*] Enable float in built-in (v)snprintf functions in (Top) → Component config → LVGL configuration → Feature configuration → Others) → Component 
(4) PSRAM CS IO number in config → ESP32-specific → Support for external, SPI-connected RAM → SPI RAM config → PSRAM clock and cs IO for ESP32-DOWD
[*] Enable esp_timer profiling features in (Top) → Component config → High resolution timer (esp_timer)
Tick handler not working?
```

3) board variance (fix pins_arduino.h)
* esp-idf only supports the esp32 variant (no boards selectable)
cd  components/arduino/variants/esp32
mv pins_arduino.h pins_arduino.h.esp32
cp ../m5stack_core2/pins_arduino.h pins_arduino.h

4) lvgl fonts not recocnized, e.g. lv_conf.h not recongnized
Add:
add_compile_definitions( LV_CONF_INCLUDE_SIMPLE=1 )  # allow custom lv_conf.h path
idf_build_set_property(COMPILE_OPTIONS "-I../main" APPEND)  # directory with lv_conf.h
to the root CMakeFile. It should really be -I../main and not -Imain even though the former is confusing since the main folder is in the same folder as the cmake file.
Uncheck [ ] Uncheck this to use custom lv_conf.h in the menuconfig. (I hope you guys follow my suggestion and hide LVGL configuration if the field is unchecked - this is how every ESP built-in component is managed in the menuconfig.)
    Remove include "lv_conf.h" from the main.c file.
    Update the release/v8.3 LVGL branch with esp_timer fix finally included ecafa88
    Change the following lines in the lv_conf.h to be
```
#define LV_TICKpy.exe -m pip install _CUSTOM 1
#if LV_TICK_CUSTOM
    #define LV_TICK_CUSTOM_INCLUDE "esp_timer.h"         /*Header for the system time function*/
    #define LV_TICK_CUSTOM_SYS_TIME_EXPR (esp_timer_get_time() / 1000LL)    /*Expression evaluating to current system time in ms*/
#endif   /*LV_TICK_CUSTOM*/
```
# Visual studio code
https://github.com/espressif/vscode-esp-idf-extension/blob/master/docs/tutorial/install.md

# Modifications on PCBs
## LDO and Core2 Board modifications
1) ADC gnd pin was not connected in layout 
    -> on LDO PCB, solder ADC pin
2) Pin 17 is the clk on PSRAM and is also used as ADC CS signal on LDO board 
    -> on Core2 PCB, remove 47R resistor to disconnect GPIO17 from ADC CS
    -> on LDO PCB, solder a wire from ADC CS connector pad to another pad of connector (lower right corner on LDO board, former GPIO16=ASIC_VSYNC on Core2)
    -> on Core2 PCB, remove 47R resistor of GPIO16 to disconnect GPIO16 from connector
    -> on Core2 PCB, solder wire for ADC CS from removed 47R to unused GPIO0 
    ( now #define GPIO_ADC_SS 0 )
3) I also want to use SD CARD!
    -> on Core2 PCB, solder wire for now available GPIO16 to solderbridg for TFCARD_CS_PIN 
    ( now #define TFCARD_CS_PIN 16 in components/M5Core2/src/utility/Config.h)
4) GPIO_PELTIER_PWM can not generate PWM signal!!! (GPIOs 34 to 39 can't generate PWM!!!)
    -> swap #define for GPIO_PELTIER_PWM with GPIO_PUMP_RPM
    #define GPIO_PELTIER_PWM 33 // 37 
    #define GPIO_PUMP_RPM    37 // 33 
5) Diode for FAN overvoltage protection?


# Start CAMctrl.py python script on windows
configure serial port in VirtualBox befor you start VirtualBox
go to Device->USB and check cp2104 serial UART bridge
start gerätemanager, check COM1 is available
start app powershell (in suchen)
cd Microsoft.PowerShell.Core\FileSystem::\\Vboxsvr\daten\ticwave\tw_chips\sources\tw_id01\M5Stack\src\idf\M5StackSarray_idf_v1\tools
install missing libs: py.exe -m pip install pyserial
.\SActrp.bat
python3.10 -m pip install libusb1
py.exe -m pip install libusb1
for plotting images: sudo zypper in gtk3-devel
python3.10 -m pip install PyQt5
python3.10 -m pip install opencv-python
sudo zypper install opencv-devel ffmpeg-devel

# Acess to USB device under linux 
. Im Ordner /etc/udev/rules.d eine Textdatei z.B. 99-local.rules 
anlegen und folgende Regel hineinschreiben:
ACTION=="add", SUBSYSTEM=="usb", ENV{DEVTYPE}=="usb_device", ATTR{idVendor}=="173a", ATTR{idProduct}=="2198", RUN{builtin}+="uaccess"
2. Konfiguration vom udev-Dienst neu lesen lassen:
 udevadm control -R
 aus.
3. Das USB-Gerät trennen und wieder verbinden.
Dann sollte dein Script auch ohne root-Rechte laufen.
Falls nicht, kannst du mit
 udevadm monitor -u
 die Aktionen anzeigen lassen, während das Gerät wieder verbunden wird. 
Es sollte dann sichtbar sein, dass "uaccess" gesetzt wird.

# Windows CP210x software (part of the USBXpressHostSDK-Win.zip under extern)

1) USBXpress Device Customization Utility (AN721) App
- use to write own Product description. One time programmable

2) Custom USB Drive Installer Wizard
- create custom driver installer 
- a custom drive will throw an error during installation. You need to do on windows root eingabeaufforderung:
bcdedit /set nointegritychecks off
or
bcdedit /set testsigning on
and restart, try agin
=> rechts unten steht TestModus
Step 1: Click the Start button, choose the Power button, press Shift and click Restart.
Step 2: After Windows enters WinRE, go to Troubleshoot > Advanced options > Startup Settings >Restart.

# PC simulator
git clone --recursive https://github.com/lvgl/lv_sim_vscode_sdl
cd lv_sim_vscode_sdl/
make
cd build/bin
./demo

# Or get it included in my existing code
## Install lv_driver
cd components
git clone https://github.com/lvgl/lv_drivers.git
cd lv_drivers
git checkout --track origin/release/v8.3

# create a single windows exe file:
Start WindoesPower shell
cd \\VBoxSvr\daten\ticwave\tw_chips\sources\tw_id01\M5Stack\src\idf\M5StackSarray_idf_v1\tools\
python3.10 -m pip install pyinstaller
pyinstaller --onefile CAMctrl.py
or to keep console open (used for raysecure stick):
pyinstaller --onefile --console CAMctrl.py
install inno installer on windows and open tools/installer.iss

# Rename fat USB stick
sudo zypper in mtools
sudo mlabel -i /dev/sdb1 -s ::"TWS_ID02"

# Rename extFAT USB stick
sudo zypper install exfat-utils
sudo exfatlabel /dev/sdb1 "TW_ID01"
sudo exfatlabel /dev/sdb1 "TWS_ID02"

# Format extFAT stick (always format the stick maybe there is a virus Trojan:Win32/Wacatac.B!ml on this Chinise stick)
lsblk
sudo mkfs.exfat -n 'TWS_ID02' /dev/sdb1

# Format USB stick
lsblk
umount /dev/sda1
mkfs.vfat /dev/sda1

# Programm CP210x on Linux

There are two versions of a CP210x programmer under linux, one from the silabs website and one on gihub.
1) SimplicityStudio_v5
get it from www.silabs.com
cd extern/SimplicityStudio_v5
./studio

2) git clone https://github.com/VCTLabs/cp210x-program
cd cp210x-program
pip install -r requirements.txt
alias python=python3
python3 setup.py install
cp210x-program.py --read-cp210x -f eeprom-content.hex

3) Wanted Product Description:
Source: TicWave Source TW_ID01

# PWM Module pinout (seen from screw side)
(left side)
#1 
#2
#3
#4
#5 - white CH1
#6 - yellow CH2
#7 - black gnd
#8 - 
#9 - red 5V (not used)
(right side)

# Measurements with Toptica setup
## Configure network, located at: /etc/NetworkManager/system-connections
[connection]
id=TopticaSetup
uuid=cc49da62-d5c7-4bba-a1e2-92da8cf77afa
type=ethernet
autoconnect=false
permissions=user:ullrich:;

[ethernet]
auto-negotiate=true
mac-address-blacklist=

[ipv4]
address1=192.168.0.100/24
dns-search=
method=manual

[ipv6]
addr-gen-mode=stable-privacy
dns-search=
method=auto

[proxy]

# # create USB stick content
* Linux part
    mkdir RaySecurUSBstick/
    mkdir RaySecurUSBstick/camera_id02_controller_v2
    pyinstaller --onefile CAMctrl.py
    cp dist/CAMctrl RaySecurUSBstick/camera_id02_controller_v2/linux/.
    rm CAMctrl.json
    ./CAMctrl.py -u
    cp ./CAMctrl.json RaySecurUSBstick/camera_id02_controller_v2/linux/.
* Test it: 
    cd RaySecurUSBstick/camera_id02_controller_v2/linux 
    ./CAMctrl
* Windows part
    Start WindowsPower shell
    cd \\VBoxSvr\daten\ticwave\tw_chips\cameras\tw_id02\M5Stack\src\idf\M5StackCamera_idf_v2\tools
    if needed: python3.10 -m pip install pyinstaller
    pyinstaller --onefile CAMctrl.py
    install inno installer on windows and open tools/installer.iss
    update revision numbers in installer.iss
    run it
    cp Output/Setup.exe RaySecurUSBstick/camera_id02_controller_v2/windows/.
* Test it
    Install software on windows: exe Setup.exe
    Start WindowsPower shell
    cd 'C:\Program Files (x86)\CAMctrl'
    On VirtualBox add USB device
    .\CAMctrl.exe
* Prepare Documentation
    cp README.md from previous release to RaySecurUSBstick/camera_id02_controller_v2/. and modify content
    cd .\doc
    cp Usermanual_TW_ID02_Rev1_v3.docx Usermanual_TW_ID02_Rev2_v1.docx
    make changes/updates to documentation
    cp ../../doc/Usermanual_TW_ID01_Rev2_v1.pdf RaySecurUSBstick/camera_id02_controller_v2/.
* Update Packaging list depending on export configuration
    cd ..\..\doc
    modify PackagingList_TW_ID01_Rev2.docx
    cp ../../doc/PackagingList_TW_ID01_Rev2.pdf RaySecurUSBstick/camera_id02_controller_v2/.
* copy to USB stick
    cp -r RaySecurUSBstick/camera_id02_controller_v2/ /run/media/ullrich/TW_ID0102/.
* Sort content of stick if needed

# Unzip compressed index.html data using https://gchq.github.io/CyberChef/
* See: https://www.youtube.com/watch?v=bIJoVyjTf7g&list=PLbDvGAjwWTEymhiLuoQwheQZLmE78iM8b&index=6
## Recipe for uncompress
Find_/_Replace({'option':'Regex','string':','},'',true,false,false,false)
Remove_whitespace(true,false,true,false,false,false)
From_Hex('0x')
Gunzip()
## Recipe for compress
Gzip('Dynamic Huffman Coding','','index.html.gz',false)
To_Hex('0x',16)
Split('0x',',0x')
## figure out the length !!!
disable To_Hex and Split and read from lower left of output window
## Copy to camera_index.h

# Enable CONFIG_HTTPD_WS_SUPPORT (WebSocket support)
idf.py menuconfig
and make sure CONFIG_HTTPD_WS_SUPPORT=y

# Create a single image
* make sure the project is correctly called in CMakeLists.txt, e.g. project(TW_ID02_rev2)
* make sure you exported the esp environment for the following: 
    . ~/src/esp/esp-idf-4_4_4/export.sh
* idf.py build
* cd build
* esptool.py --chip ESP32 merge_bin -o TW_ID02_rev2_image.bin @flash_args
* back it up: cp TW_ID02_rev2_image.bin ../release_v2/.
* cd ../release_v2/.
* test this file diretcly with: esptool.py write_flash 0x0 TW_ID01_rev1b_image.bin

# Create Linux executable flash script
1) test python script to set the port: ./flashme.py TW_ID02_rev2_image.bin
2) create stand alone exe of the esptool.py script:
    pyinstaller --onefile --console esptool.py 
3) create stand alone exe of the flashme.py script and add the esptool binary:
    pyinstaller --onefile --add-binary "dist/esptool:." --console flashme.py

# Creata Windows executable flash script
* execute this also on Windows, open powershell
    cd \\VBoxSvr\daten\ticwave\tw_chips\sources\tw_id01\M5Stack\src\idf\M5StackSarray_idf_v1b\release_v1b
    pyinstaller --onefile --console .\esptool.py 
    pyinstaller --onefile --add-binary ".\dist\esptool.exe;." --console flashme.py

# create USB stick content
    mkdir USBstick
    mkdir USBstick/linux
    mkdir USBstick/windows
    cp TW_ID02_rev2_image.bin USBstick/linux/.
    cp TW_ID02_rev2_image.bin USBstick/windows/.
    cp dist/flashme USBstick/linux/.
    cp dist/flashme.exe USBstick/windows/.

# Copy old images from previous releases
    cp ../release_v1b/USBstick/linux/TW_ID01_rev1_image.bin USBstick/linux/.
    cp ../release_v1b/USBstick/linux/TW_ID01_rev1b_image.bin USBstick/linux/.
    cp ../release_v1b/USBstick/linux/TW_ID01_rev1_image.bin USBstick/windows/.
    cp ../release_v1b/USBstick/linux/TW_ID01_rev1b_image.bin USBstick/windows/.

# Edit/Copy README_4USBstick.md 

* Add change log to readme
* cp README_4USBstick.md ./USBstick/README.md
## test it

# Format SD card for Core2
* sudo mkfs.vfat -F32 -s 64 /dev/mmcblk0p1
- it requires a -s 64: Sets 64 sectors per cluster, which, given a sector size of 512 bytes, results in a cluster size of 32KB.

# How about streaming mjpeg?

pip install opencv-python-headless

# Create a requirements.txt file
pip install pipreqs
cd tools
pipreqs .
pip install -r requirements.txt

# Compress exe on windows and linux
* get it from https://github.com/upx/upx/releases/tag/v4.2.3
* on linux there are compatibility problems, so it is not used and disabled: 
pyinstaller --onefile --upx-dir=../extern/upx-4.2.3-amd64_linux CAMctrl.py
* on Windows it works, but starts slowr:
pyinstaller --onefile --upx-dir=../extern/upx-4.2.3-win64 CAMctrl.py

# Create a word doc from the README.md
* pandoc README.md -o README.docx

# add new lvgl symbols
* go to https://github.com/lvgl/lv_font_conv and clone into ~/src
* sudo zypper install npm
* sudo npm install -g lv_font_conv
* cd /home/ullrich/DATEN/ticwave/tw_chips/sources/tw_id01/M5Stack/src/idf/M5StackSarrayFan_idf_v2b/components/lvgl/scripts/built_in_font
* subl built_in_font_gen.py and add the Built in symbols with syms = ...
* ./generate_all.py 
* also add a define for, e.g. LV_SYMBOL_HISTO in /home/ullrich/DATEN/ticwave/tw_chips/sources/tw_id01/M5Stack/src/idf/M5StackSarrayFan_idf_v2b/components/components/lvgl/src/font/lv_symbol_def.h 
* find the decimal: inspect the ttf font file to find unicode hex value, then convert hex to decimal: fontforge FontAwesome5-Solid+Brands+Regular_windows.ttf
* find the unicode string: python3.11 and the type: chr(63550).encode('utf-8')
* idf.py build flash monitor /dev/ttyUSB2
* to include a greak symbol, which is part of Monserrat, change the default font gange to explicit include that hex unicode in build_in_font_gen.py at line: default=['0x20-0x7F,0xB0,0x2022,0xB5'] then run generate_all.py 