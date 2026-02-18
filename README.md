# Focuser
 Pico based focuser emulating the moonlite focuser protocol, this is a work in progress as far a release is concerned, but is fully working and in use on my own telescopes.
 
 Use Arduino IDE to build.

The MoonLite focuser uses a simple ASCII-based serial protocol, commonly referred to as the MoonLite protocol. These commands are often sent over a virtual COM port (RS232) at 9600 baud. 
Note: All commands start with a colon : and end with a hash #. Hexadecimal values are usually represented as 4-character strings (e.g., 0000 to FFFF). 
Primary MoonLite Focuser Commands
:CG# - Temperature Conversion (Starts conversion)
:FG# - Go to Target Position (Moves motor to SN value)
:FQ# - Stop Movement
:GC# - Get Temperature Coefficient
:GD# - Get Stepping Delay (Returns 2-character hex value)
:GH# - Get Half-step Mode (Returns 0xFF if half, 0x00 if full)
:GI# - Get Moving Status (Returns 01 if moving, 00 otherwise)
:GN# - Get Target Position (Returns 4-character hex)
:GP# - Get Current Position (Returns 4-character hex)
:GT# - Get Current Temperature
:GV# - Get Version (Returns firmware version)
:SCxx# - Set New Temperature Coefficient
:SDxx# - Set Stepping Delay (02, 04, 08, 10, 20)
:SF# - Set Full-step Mode
:SH# - Set Half-step Mode
:SNxxxx# - Set New Target Position (4-character hex)
:SPxxxx# - Set Current Position (Sync/Set position to xxxx)
:PLUS# - Activate Temperature Compensation 
