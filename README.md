Focuser

A Raspberry Pi Pico–based focuser that emulates the MoonLite focuser protocol.

⚠️ Status: Work in progress in terms of release polish, but fully functional and currently in use on my own telescopes.

Build Environment

IDE: Arduino IDE

Target: Raspberry Pi Pico

Overview

The MoonLite focuser uses a simple ASCII-based serial protocol, commonly referred to as the MoonLite protocol.
Commands are typically sent over a virtual COM port (RS232) at 9600 baud.

Protocol Notes

All commands start with : and end with #

Numeric values are usually hexadecimal

Hex values are typically fixed-width (e.g. 0000–FFFF)

Supported MoonLite Commands
Movement & Status
Command	Description
:FG#	Go to target position (moves motor to value set by :SNxxxx#)
:FQ#	Stop movement
:GI#	Get moving status (01 = moving, 00 = stopped)
:GN#	Get target position (returns 4-character hex)
:GP#	Get current position (returns 4-character hex)
Position Control
Command	Description
:SNxxxx#	Set new target position (4-character hex)
:SPxxxx#	Set current position (sync/set position to xxxx)
Temperature & Compensation
Command	Description
:CG#	Start temperature conversion
:GT#	Get current temperature
:GC#	Get temperature coefficient
:SCxx#	Set new temperature coefficient
:PLUS#	Enable temperature compensation
Stepper Configuration
Command	Description
:GD#	Get stepping delay (returns 2-character hex)
:SDxx#	Set stepping delay (02, 04, 08, 10, 20)
:GH#	Get half-step mode (FF = half, 00 = full)
:SF#	Set full-step mode
:SH#	Set half-step mode
Firmware Information
Command	Description
:GV#	Get firmware version
Notes

All responses conform to the standard MoonLite ASCII protocol

Designed for compatibility with existing MoonLite-aware astronomy software

Tested in real telescope setups
