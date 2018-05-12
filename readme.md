# Mimic Panel Controller / Train Emulator
The project: restore an old Queensland Rail signalling mimic panel.

The panel in this case is for the Forest Hill station (the physical building now resides at QPSR's Swanbank station) and the intent is to exhibit it as a interactive display in the station shop area.

![Forest Hill Panel](https://raw.githubusercontent.com/blip2/mimic-ctrl/master/img/illuminated.png)
*Panel with power applied to some of the signalling indicators during testing*

## Electronics

Originally the panel was fed remotely via a 45-pole DC connector on the bottom of the panel. I would have liked to plug directly into the panel to avoid altering any of the existing wiring but haven't been able to find a plug (I may be that it's a bespoke item, QR seem to just make their own components sometimes) so have cut the internal wiring loom at the back of the connector.

![DC Connector](https://raw.githubusercontent.com/blip2/mimic-ctrl/master/img/connector.png)
*Existing 45-pole connector mounted on the bottom of the panel*

I did a bit of reverse engineering on the existing wiring to determine the supply voltage. The LED indicator lights negative terminals are wired back to breadboard mounted in the panel where the negatives are commoned though two 3.9k resistors in parallel.

![Resistor Breadboard](https://raw.githubusercontent.com/blip2/mimic-ctrl/master/img/breadboard.png)
*LED/buzzer resistors mounted on breadboard in the inside the panel. The red cylinder above is the buzzer.*

This gives a total impedance of 1.96kOhm for each LED. Assuming the LEDs are in the region of 10-15mA with a 2-3 voltage drop I tested them with a 24V power supply which gave a reasonable brightness.

I'm using an Arduino Uno and a 16-way Sontay mechanical relay board from Ebay to driver the LEDs and buzzer and handle user input in the form of the two existing push buttons. This is all mounted inside the spacious cabinet.

I've left the original "digram lights" switch in-line with the common negative so it can be used to switch the system on and off.

![New Electronics](https://raw.githubusercontent.com/blip2/mimic-ctrl/master/img/rewired.png)
*New relays connected up to existing wiring looms*

## Software

Given that there might be more panels to restore in the future and that the functionality of the panel might want to be tweaked. The code is object oriented and can be relatively easily extended to model more complex panels.

The software currently defines Blocks which can be Clear/Occupied which make up Routes. Currently 'trains' are essentially captured with the Block class but these could be brought out if more complex emulation was desirable (not really worth it on this panel).

I've attempted to balance realism with functionality/good interaction so there are a number of features which deviate from how the panel originally operated:
* When a train enters the section, it won't proceed unless acknowledged. In reality the train would proceed to the home signal and the buzzer would sound until acknowledged. The panel didn't originally have any ability to control signalling or train movements.
* The ER indicator lights are being used to indicate a train at/departing from the platform. In reality these were used to indicate the status of the electronic releases for the siding groundframes.

## Contributors
Ben Hussey (<a href="mailto:ben@blip2.net">ben@blip2.net</a>)
Thanks due to Francis Tybislawski for advise and support.
