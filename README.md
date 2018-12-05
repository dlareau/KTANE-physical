# KTANE-physical
A physical version of the game "Keep Talking and Nobody Explodes". 
Not a real bomb.

### Planned Modules

| Module Name         | Sketched         | Coded            | Made             |
| ------------------- |:----------------:|:----------------:|:----------------:|
| Memory              |:heavy_check_mark:|:heavy_check_mark:|:heavy_check_mark:|
| Simon Says          |:heavy_check_mark:|:heavy_check_mark:|:heavy_check_mark:|
| Wires               |:heavy_check_mark:|:heavy_check_mark:|:heavy_check_mark:|
| Morse Code          |:heavy_check_mark:|:heavy_check_mark:|:heavy_check_mark:|
| The Button          |:heavy_check_mark:|                  |                  |
| Maze                |                  |                  |                  |
| Password            |:heavy_check_mark:|:heavy_check_mark:|                  |
| Keypad (symbols)    |:heavy_check_mark:|                  |                  |
| Switches            |:heavy_check_mark:|:heavy_check_mark:|:heavy_check_mark:|
| Letter Keys         |:heavy_check_mark:|                  |                  |
| Twobits             |:heavy_check_mark:|                  |                  |
| Crazy Talk          |:heavy_check_mark:|                  |                  |
| Connection Check    |:heavy_check_mark:|                  |                  |
| Venting Gas         |                  |                  |                  |
| Capacitor Discharge |                  |                  |                  |
| Knobs               |                  |                  |                  |

The general plan is for each module to contain an ATmega328p which will handle 
running all of the local I/O and communicate with a controller module
(also an Atmega328p).

### Intermodule communication (DSerial)

Many modules need to act as an I2C master to display devices or sensors, and
an unfortunate number of display devices have locked I2C addresses. This means
that I2C isn't feasibly a possibility for intermodule communication. Desiring
a solution that didn't require running a dedicated wire from each module to the
controller module, I opted for a multidrop serial bus. 

The downside of a multidrop bus is that some form of arbitration is
needed to stop bus contention. This is solved by the DSerial library I wrote
for this project. The DSerial library implements a layer on top of any Arduino
[Stream](www.arduino.cc/reference/en/language/functions/communication/stream/) 
interface. The DSerial library operates with one master and many clients and 
in addition to managing bus contention, it makes a best effort to guarantee
that each message will be delivered to the desired client exactly once. To this
end, it implements message-level parity checking, a pre-defined number of 
retries upon failed delivery, and can determine which clients are alive on the
network.

Currently DSerial has two major limitations: 
- Due to being comprised of entirely non-blocking calls, it requires whatever 
is using it to periodically and continuously call the doSerial method.

- It currently only allows the transmission of bytes with values between 0x01
and 0x7F inclusive. Future implementations will hopefully allow for the
transmission of null bytes and bytes above 0x7F.

### Module behavior and templates

Common functionality across modules has been extracted out into the KTANECommon
library. The library handles the sending/receiving of strikes, solves and
configuration data between solvable modules and the controller module. The 
library also handles the controller waiting for modules to setup and the modules
easily querying various aspects of the configuration data. The KTANECommon
library suffers from the same upside/downside as the DSerial library in that it
requires the code to periodically and continuously call the interpretData
function. 

A template for modules can be found [here](modules/exampleModule/example.ino).

### The Hardware

The current iteration of the hardware is Version 1. V1 consists of one
controller module and up to five swappable, playable modules in a suitcase as
an enclosure. The modules connect to the main bus through directional 
male/female 4 pin plugs intended for LED strips. The major limiting factor
imposed by V1 is the 1.25" height limit on user interface protrusions needed
to allow the case to close. 


