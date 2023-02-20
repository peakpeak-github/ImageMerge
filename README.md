# ImageMerge
Distribute your ESP-* project as one binary file.
https://arduino-esp8266.readthedocs.io/en/latest/

If you have built a solution you want to share among friends/customers but do not want to :
1.	Share the source code
or
2.	Need your friends/customers to install a complete VS Code and platform.io

Then one possible solution is to distribute the memory image of both the program partition and the filesystem partition as one single binary file which can be uploaded by one single program to the board.

Meet ImageMerge and ESPTool.exe! 

I will not take any credit for ESPTool, it’s a python program which I converted into a an .exe with pyinstall to be self-contained without any dependencies. 

ImageMerge combines both the program partition and the filesystem partition as one single binary file.
This file can then be uploaded to the board with ESPTool. 

Your friends/customers then only need three files without any installation requirements.
1.	ESPTool.exe.
2.	The resulting file produced by ImageMerge.
3.	A batch file starting ESPTool with the required arguments.

Usage:

ImageMerge -prog firmware.bin -fs littlefs.bin -image everything.bin -offset 1024  -v  
*** Note: Find correct offset value for your board:  
https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html

This will produce everything bin from firmware.bin and littlefs.bin.  
Distribute this file with Esptool.exe and upload.bat

On customer side:  
Copy Esptool.exe. upload.bat and everything.bin to a folder. Open a CMD window in this folder.  
Execute upload.bat with a connected ESP-board.  
*** Note that flash_mode in batch file must match the board_build.flash_mode in platformio.ini
