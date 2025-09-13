tabata-timer-module
***
**Personal Project Description**:


This Tabata Timer Module is used to program Calisthenics and any other static/dynamic exercises that are to be done in a specific time interval. The purpose of this timer is to show exercise time, resting time, and sets utilizing finite state machine logic. The module includes 5 fully customizable profiles that are written to the EEPROM to allow for multi-profile saving. Buzzer alerts are given nearing the end of each state to notify user of when they have reached the set amount of time. All components are through-hole and were soldered to the PCB board (Note a simple soldering iron will be required to solder all the through-hole components). Files and descriptions are listed below in the table along with all the required parts for the project.

**Files**:

| File | Description |
| ------- | --------- |
| tabata.kicad_sch | Schematic |
| SSD1306.pretty | SSD1306 Footprint | 
| tabata.kicad_pcb | PCB | 
| tabata_v1.ino | Program |

**Parts List**:
* (1x) SSD1306 OLED (I2C)
* (1x) 5641AS (common anode)
* (4x) Push Button
* (1x) Arduino NANO / NON-OEM equivalent
* (4x) 10kΩ Resistors
* (7x) 220Ω Resistors
* (1x) 330Ω Resistors
* (1x) Passive Buzzer

1.**Schematic**

![image alt](https://github.com/NathanielM14/tabata-timer/blob/44222909e9ed671a1c4394a2dc6a68d3b45191cb/images/schematic_design.png)

2. **SSD1306 Footprint**

![image alt](https://github.com/NathanielM14/tabata-timer/blob/f717f7226c26d0fee968ef8726d06513bc65c88a/images/ssd1306_footprint.png)

3. **PCB**

![image alt](https://github.com/NathanielM14/tabata-timer/blob/54fe5bcee97a72f83aa2d56b4533ac9723e4b041/images/pcb_design.png)

4. **Physical**

![image alt](https://github.com/NathanielM14/tabata-timer/blob/f717f7226c26d0fee968ef8726d06513bc65c88a/images/physical.png)

![image alt](https://github.com/NathanielM14/tabata-timer/blob/f717f7226c26d0fee968ef8726d06513bc65c88a/images/physical_demo.png)
