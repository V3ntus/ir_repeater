#include <IRremote.hpp>
#include <LiquidCrystal.h>

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

// initialize LCD
const int rs = 11, en = 10, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

#define IR_TX_PIN 8
#define TX_BUTTON 7
#define STATUS_PIN 13
int REPEAT_DElAY = 50;
int last_button_state;

typedef const __FlashStringHelper *FlashString;

FlashString revEnum(int val) {
    switch (val)
    {
    case 1: return F("UNKNOWN");
    case 2: return F("PULSE_DIS");
    case 3: return F("DENON");
    case 4: return F("DISH");
    case 5: return F("JVC");
    case 6: return F("LG");
    case 7: return F("LG2");
    case 8: return F("NEC");
    default: return F("UNKNOWN");
    }
}

// store IR data
struct stored_IR_data {
    IRData receivedIRData;
    uint8_t rawCode[RAW_BUFFER_LENGTH];
    uint8_t rawCodeLength;
} sStoredIRData;

void storeCode(IRData *aIRReceivedData);
void sendCode(stored_IR_data *aIRDataToSend);

void setup()
{
    Serial.begin(9600);
    lcd.begin(16, 2);
    IrReceiver.begin(12, ENABLE_LED_FEEDBACK); // Start the receiver
    IrSender.begin(IR_TX_PIN, ENABLE_LED_FEEDBACK); // Start the transmitter

    pinMode(STATUS_PIN, OUTPUT);

    // pinMode(TX_BUTTON, INPUT);
    lcd.print("Ready");
    Serial.print("IR protocols ready: ");
    printActiveIRProtocols(&Serial);
}

void loop()
{
    int button_state = digitalRead(TX_BUTTON);
    lcd.setCursor(0, 0);

    // button is inactive low
    if (last_button_state == HIGH && button_state == LOW) {
        IrReceiver.start();
        Serial.println("Button released");
        // lcd.clear();
        // lcd.print("RX Ready");
    }

    // button active is high
    // do work here
    if (button_state == HIGH) {
        IrReceiver.stop();
        Serial.println("Button pressed");
        digitalWrite(STATUS_PIN, HIGH);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Sending...");

        if (last_button_state == button_state){
            sStoredIRData.receivedIRData.flags = IRDATA_FLAGS_IS_REPEAT;
        }

        sendCode(&sStoredIRData);
        digitalWrite(STATUS_PIN, LOW);
        delay(50);
    } else if (IrReceiver.available()) {
        // Check for incoming data
        Serial.println("Reading data...");
        storeCode(IrReceiver.read());
        IrReceiver.resume();
        lcd.clear();
        lcd.print("RX incoming...");
        lcd.setCursor(0, 1);

        int _index = (int) sStoredIRData.receivedIRData.protocol;
        lcd.print(String(revEnum(_index)) + " - 0x" + String(
            sStoredIRData.receivedIRData.command, HEX
        ));
    }

    last_button_state = button_state;
}

void storeCode(IRData *aIRReceivedData) {
    if (aIRReceivedData->flags & IRDATA_FLAGS_IS_REPEAT) {
        Serial.println("Ignore repeat");
        return;
    }

    if (aIRReceivedData->flags & IRDATA_FLAGS_IS_AUTO_REPEAT) {
        Serial.println("Ignore autorepeat");
        return;
    }

    if (aIRReceivedData->flags & IRDATA_FLAGS_PARITY_FAILED) {
        Serial.println("Ignore parity");
        return;
    }

    sStoredIRData.receivedIRData = *aIRReceivedData;

    if (sStoredIRData.receivedIRData.protocol == UNKNOWN) {
        Serial.print("RX unknown");
        Serial.print(IrReceiver.decodedIRData.rawDataPtr->rawlen - 1);
        Serial.println(" timing entries as raw ");
        IrReceiver.printIRResultRawFormatted(&Serial, true);
        sStoredIRData.rawCodeLength = IrReceiver.decodedIRData.rawDataPtr->rawlen - 1;
        IrReceiver.compensateAndStoreIRResultInArray(sStoredIRData.rawCode);

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("RX: UNKNOWN");
        lcd.setCursor(0, 1);
        lcd.print("LEN: " + IrReceiver.decodedIRData.rawDataPtr->rawlen - 1);
    } else {
        IrReceiver.printIRResultShort(&Serial);
        sStoredIRData.receivedIRData.flags = 0;
        Serial.println();
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("RX: " + sStoredIRData.receivedIRData.protocol);
        lcd.setCursor(0, 1);
        lcd.print("LEN %s" + IrReceiver.decodedIRData.rawDataPtr->rawlen - 1);
    }
}

void sendCode(stored_IR_data *aIRDataToSend) {
    if (aIRDataToSend->receivedIRData.protocol == UNKNOWN) {
        IrSender.sendRaw(aIRDataToSend->rawCode, aIRDataToSend->rawCodeLength, 38);

        Serial.print("Sent raw ");
        Serial.print(aIRDataToSend->rawCodeLength);
        Serial.println(" marks or spaces");
        lcd.setCursor(0, 1);
        int _index = (int) sStoredIRData.receivedIRData.protocol;
        lcd.print("RAW - 0x" + String(
            sStoredIRData.receivedIRData.command, HEX
        ) + "; " + sStoredIRData.rawCodeLength);
    } else {
        IrSender.write(&aIRDataToSend->receivedIRData, NO_REPEATS);

        Serial.print("Sent: ");
        printIRResultShort(&Serial, &aIRDataToSend->receivedIRData);
        lcd.setCursor(0, 1);
        int _index = (int) sStoredIRData.receivedIRData.protocol;
        lcd.print(String(revEnum(_index)) + " - 0x" + String(
            sStoredIRData.receivedIRData.command, HEX
        ) + "; " + sStoredIRData.rawCodeLength);
    }
}