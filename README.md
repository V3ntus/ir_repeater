# ir_repeater

```

/* 

Pin configuration:

> LCD Display:
VSS: GND
VDD: +5V
VO: 10k wiper
  - arms: +5v & GND
RS: 11
RW: GND
E: 10
D4: 5
D5: 4
D6: 3
D7: 2

> IR Receiver: (lookup your pinout for your model)
G: GND (connect a current limiting resistor if not present)
R: +5V
Y: 8

> IR Transmitter: (got it off an old remote)
Anode: +5V
Cathode: resistor to GND

> Push button: (HIGH on active/closed/pushed)
Open: resistor to GND
Closed: +5V
Out: 7

*/
```
