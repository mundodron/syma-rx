SYMA Gamepad Controller
==========

Thanks to

 * suxsem for the original code (https://github.com/Suxsem/symaxrx)
 * MHeironimus for the joystick code (https://github.com/MHeironimus/ArduinoJoystickLibrary)
 * execuc for the original code (https://github.com/execuc/v202-receiver)
 * Deviationtx team for hacking the symax protocol (https://bitbucket.org/deviationtx/deviation/src/1194044d116b9611a015837226729e26de7e8365/src/protocol/symax_nrf24l01.c)

 Goal
--------------------
This code decodes frames from the Syma X5C-1, X11, X11C, X12,... transmitter with an Arduino Leonardo or Arduino Micro (Maybe also works on Arduino UNO with UnoJoy) and a NRF24L01 module and it then sends the data back to the USB port as if ir was gamepad.
I've only tested it on Windows but should work on other Operting Systems as well.

