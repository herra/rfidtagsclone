#include <SoftwareSerial.h>

//This sketch reads & writes RFID tags with Gwiot 7941E v3.0(read only) or 7941W v1.1
int RX7941W = 3;
int RX7941E = 12;
int RX = RX7941W;
int TX = 4;

bool is7941E = RX == RX7941E;
byte* dataBytes;
int dataBytesLength;

SoftwareSerial rfidSerial (RX, TX);

String decodeCardType(int code);
void read7941E(int bytesReceived[], int length);
void read7941W(int bytesReceived[], int length);
void sendCommand(byte address, byte cmd, byte data[], int dataLength);
byte xorCheck(byte arr[], int from, int to);

void setup() {
  Serial.begin(9600);
  
  rfidSerial.begin(115200);  
  rfidSerial.listen();

  if (!is7941E) {
    Serial.println("Type 1 to read key");
    Serial.println("Type 2 to write readed key (Board dependant)");
  }
}

void loop() {
  delay(500);
  Serial.println(".");
  int rd = Serial.read();
  
  if (rd == '1' && !is7941E) {
    sendCommand(0, 0x15, NULL, 0);
  }

  if (rd == '2' && !is7941E) {
    sendCommand(0, 0x16, dataBytes, dataBytesLength);
  }

  if (rfidSerial.available() > 0) {
    Serial.println();
    int bufferLength = 30;
    int bytesReceived [bufferLength];

    for (int j =0; j < bufferLength; j++) {
      bytesReceived[j] = 0;
    }
     
    int i = 0;
    while(rfidSerial.available() > 0) {
      bytesReceived[i++] = rfidSerial.read();
      delay(1);
    }

    Serial.println("Readed");
    for (int j =0; j < i; j++) {
      Serial.print(String(bytesReceived[j], HEX));Serial.print(" ");
    }
    Serial.println("");

    if (is7941E)
      read7941E(bytesReceived, i);
    else
      read7941W(bytesReceived, i);
  } else {
    delay(500);
  }
}

void read7941E(int bytesReceived[], int length) {
  String dataHead, dataLength, cardType, serialNumber, bcc, dataEnd;
  int bccCheck = 0, dataLengthNum;
  for (int j = 0; j < length; j++) {
    if (j == 0)
      dataHead = String(bytesReceived[j], HEX);
    if (j == 1) {
      dataLengthNum = bytesReceived[j];
      dataLength = String(dataLengthNum, HEX);
      if (dataLengthNum != length) {
        dataLength += " INVALID";
      }
      bccCheck = bytesReceived[j];
    }
    if (j == 2) {
      cardType = String(bytesReceived[j], HEX);
      cardType += " Type: " + decodeCardType(bytesReceived[j]);
      bccCheck = bccCheck ^ bytesReceived[j];
    }
    if (j >= 3 && j <= dataLengthNum - 3) {
      serialNumber += String(bytesReceived[j], HEX) + " ";
      bccCheck = bccCheck ^ bytesReceived[j];
    }
    if (j == dataLengthNum - 2) {
      bcc += String(bytesReceived[j], HEX);
      if (bccCheck != bytesReceived[j])
        bcc += " INVALID BCC";
    }
    if (j == dataLengthNum - 1) {
      dataEnd += String(bytesReceived[j], HEX);
    }
  }
   
  Serial.println(" ");
  Serial.println("DataHead: " + dataHead);
  Serial.println("DataLength: " + dataLength);
  Serial.println("CardType: " + cardType);
  Serial.println("SerialNumber: " + serialNumber);
  Serial.println("bcc Received: " + bcc);
  Serial.print("BCC Check"); Serial.print(" "); Serial.print(bccCheck, HEX); Serial.println(" ");
  Serial.println("DataEnd: " + dataEnd);
  
  Serial.println(" ");
}

void read7941W(int bytesReceived[], int length) {
  String dataHead, dataLength, data = "", bccReceived, bccResult;
  int bccCheck = 0, dataLengthNum, dataBytesIndex = 0;
  for (int j = 0; j < length; j++) {
    if (j == 0)
      dataHead = String(bytesReceived[j], HEX);
    if (j == 1) {
      dataHead += String(bytesReceived[j], HEX);
    }
    if (j == 2) {
      Serial.print("Address: "); Serial.print(bytesReceived[j], HEX); Serial.println(); 
    }
    if (j == 3) {
      Serial.print("Command: "); Serial.print(bytesReceived[j], HEX); Serial.println();
      bccCheck = bytesReceived[j];
    }
    if (j == 4) {
      dataLengthNum = bytesReceived[j];
      Serial.println("Length: "); Serial.print(bytesReceived[j], HEX); Serial.println();
      dataLength = String(dataLengthNum, HEX);
      if (dataLengthNum != length - 5 - 1) {
        dataLength += " INVALID";
      }
      dataBytes = new byte[dataLengthNum];
      dataBytesLength = dataLengthNum;
      bccCheck = bccCheck ^ bytesReceived[j];
    }
    
    if (j >= 5 && j <= dataLengthNum + 4) {
      data += String(bytesReceived[j], HEX) + " ";
      dataBytes[dataBytesIndex++] = bytesReceived[j];
      bccCheck = bccCheck ^ bytesReceived[j];
    }
    
    if (j == length - 1) {
      bccResult = String(bccCheck, HEX);
      bccReceived = String(bytesReceived[j], HEX);
      if (bccCheck != bytesReceived[j])
        bccResult += " INVALID BCC";
    }
  }
   
    
  Serial.println(" ");
  Serial.println("DataHead: " + dataHead);
  Serial.println("DataLength: " + dataLength);
  Serial.println("Data: " + data);
  Serial.print("BCC Received: " + bccReceived); Serial.println(" ");
  Serial.print("BCC Check: " + bccResult);
  
  Serial.println(" ");
}

void sendCommand(byte address, byte cmd, byte data[], int dataLength) {
  int cmdLength = 6 + dataLength;
  byte  command [cmdLength];
  command[0] = 0xAB;
  command[1] = 0xBA;
  command[2] = address;
  command[3] = cmd;
  command[4] = dataLength;
  int cmdIndex = 5;
  for (int i = 0; i < dataLength; i++) {
    command[cmdIndex++] = data[i];
  }    
  command[cmdLength - 1] = xorCheck(command, 3, cmdLength - 2); //XOR

  rfidSerial.flush();

  Serial.println("");
  Serial.println("Sending command:");
  for (int c = 0; c < cmdLength; c++) {
    rfidSerial.write(command[c]);
    Serial.print(command[c], HEX); 
    Serial.print(" ");
    delay(10);
  }
  Serial.println("");
  Serial.println("Done sending:");
}

String decodeCardType(int code) {
  switch(code) {
    case 0x01: return "MIFARE 1K";
    case 0x02: return "EM4100";
    case 0x03: return "MIFARE 4K";
    case 0x10: return "HID Card";
    case 0x11: return "T5567";
    case 0x20: return "2nd Card";
    case 0x21: return "ISO14443B";
    case 0x22: return "FELICA";
    case 0x30: return "15693 Label";
    case 0x50: return "CPU Card";
    case 0x51: return "Sector Information";
    case 0xFF: return "Keyboard Data";
    default: return "CardType DID NOT MATCH KNOWN VALUE";
  }
}

byte xorCheck(byte arr[], int from, int to) {
  int result = 0;
  for (int i = from; i <= to; i++) {
    result = result ^ arr[i];
  }
  return result;
}

/*7941E
 * 
1.Description:

7941E card reader module, integrated high performance card reading radio frequency circuit and antenna and users can select wiegand or UART interface. Rich application support, it supports various cards card reading operation

2.Features:

1>. Voltage: DC 5V

2>. Current: 40mA

3>. Support 125KHz card reading

4>. Interface: Wiegand, UART

5>. Size: 47*26*5mm

6>. Operating Temperature: -20~70 Celsius

7>. EM4100 Card Reading Distance: >8cm

3.Connection Introduction:

1>.Power: DC 5V power supply, choose linearity power can gain better effects

2>.D1 D0: wiegand data output DATA1, DATA0

3>.FOR: format selection (hanging means W26, ground connection means W34)

4. Wirgand Interface Output Introduction:

1>.When it sensors the card, the serial number will be output via Data 0, Data 1 two data wire

2>.Data 0 and Data 1 are high level when there is no data output

3>.Data Bit 0: it will generate 400us low level on Data 0 wire

4>.Data Bit 1: it will generate 400us low level on Data 1 wire

5>.Every data length is 2400us

6>.Every Mifare card has a serial number with 4 bytes, users should output the last three bytes

7>.Add the first 12-bit even check bit in the front and add the last 12-bit odd check bit in the back, 26-bit data in total

8>.Card number is 6B 3D 12 D6

9>.Output data is 3D 12 D6

10>.Weigand 26 Code

0

111101

10010

11010110

1

 
Even Check

3D

12

D6

Odd Check

 
11>.Weigand 34 Code

Weigand 34 Code

0

1101011

111101

10010

11010110

0

Even Check

6B

3D

12

D6

Odd Check

12>.UART Interface Output

UART Interface Output

Data Head

Length

Card Type

Card Type

BCC Check

Data End

0x02

0x09

0x01

SN0~SN3

XOR Check Except Data Head And Data End

0x03

13>.Card Type:

0x02 EM4100

0x01 MIFARE 1K

0x03 MIFARE 4K

0x10 HID Card

0x11 T5567

0x20 2nd Card

0x21 ISO14443B

0x22 FELICA

0x30 15693 Label

0x50 CPU Card

0x51 Sector Information

0xFF Keyboard Data

For example: the data received by serial port tool is 02 0A 02 2E 00 B6 D7 B5 F2 03, then

The first byte 0x02 means data start
The second byte 0x0A means the whole data length is 10 bytes, which includes data start and data end

The third byte 0x02 means the card type is EM4100

The fourth to eighth byte (0x2E 0x00 0xB6 0xD7 0xB5) means the read card number, and the fourth byte 0x2E is hidden card number

The ninth byte 0xF2 means BCC check of second byte to eighth byte

The tenth byte 0x03 means data end

5.Notice:

Keep away from interference source, and strong interference signal may affect card reading

Select linearity power can improve the card reading distance and effect; switch power may attenuate the distance but will not influence normal operation

The module can operate normally in the range of 3-5V, please do not use higher voltage

This module has high frequency signal, so it may cause some interference to the analog circuit. Uses should place the module a little far away from the buzzer; if the distance is too close, the sound of the buzzer will be low

6.Applications:

1>.Attendance Fingerprint Read Write Module

2>.Entrance Guard Intercom Read Write Module

3>.Replicator

4>.Attendance fingerprint card reader module

5>.Entrance guard intercom card reader module
*/


/*7941W
 * 1.Description:

7941W is a multi-protocol dual-band read and write module.set IC and ID read and write one.It can read a variety of IC and ID card.Support Mifare1K,UID card,IC card,T5577 ID card.

2.Features:

1>.Voltage: DC 5V

2>.Current: 50mA

3>.Distance: Mifare>3cm; EM>5cm

4>.Size: 47mmX26mmX5mm

5>.Interface: UART, Wiegand

6>.Support Chips: ISO/IEC 14443 A/MIFARE, NTAG, MF1xxS20, MF1xxS70, MF1xxS50

7>. EM4100, T5577 read and write function

8>.Operating Temperature: -25~85 Celsius

3.Connection Introduction:

1>.5V: DC 5V power supply pin; if you use linearity power, it will gain better effects

2>.RX: receive pin

3>.TX: transmit pin

4>.GND: power supply ground pin

5>.IO: definition

4. Protocol Introduction: (UART serial port communication protocol is shown as below)

Sending Protocol:

Protocol Header | Address | Command | Data Length |    Data     | XOR Check

         2 Byte | 1 Byte  | 1 Byte  | 1 byte      | 1-255 Byte  | 1 Byte
AB BA
           
Receiving Protocol:

Protocol Header | Address  | Command  | Data Length |     Data  | XOR Check 
(2 byte)        | (1 byte) | (1 byte) | (1 byte)    | (1 - 255) | (1 byte)
CD DC

1>. Protocol Header: send (0xAB 0xBA)

2>. Return: (0xCD 0xDC)

3>. Address: default 0x00

4>. Command:

Send:

1). 0x10 read UID number

2). 0x11 write UID number (4 bytes), use default password ffffffffffff

3). 0x12 read specified sector

4). 0x13 write specified sector

5). 0x14 modify the password of group A or group B

6). 0x15 read ID number

7). 0x16 write T5577 number

8). 0x17 read all sector data (M1-1K card)

Return:

1).0x81 return operation succeeded

2).0x80 return operation failed

5>. Data Length: means following data length; if it’s 0, then the following data will not occur

6>. Data: read and written data

Sending Data:

1).Read Specified Sector: the first byte of the data represents sector; the second byte means the certain block of the sector; the third byte means A or B group password (0x0A/0x0B); then it comes with password of 6 bytes

2).Write Specified Sector: the first byte of the data represents sector; the second byte means the certain block of the sector; the third byte means A or B group password (0x0A/0x0B); then it comes with password of 6 bytes and block data of 16 bytes

3).Modify Password: the first byte means the certain sector; the second byte means A or B group password (0x0A/0x0B); then it comes with old password of 6 byte and new password

Receiving Data:

Read specified sector return data format, the first byte is sector; the second byte is the certain block of sector; then it comes with block data of 16 bytes

7>. XOR check: result of other bytes check except protocol header

5.Example:

AB BA 00 10 00 10

AB BA 00 11 04 6D E9 5C 17 DA

AB BA 00 12 09 00 01 0A FF FF FF FF FF FF 10

AB BA 00 13 19 00 01 0A FF FF FF FF FF FF 00 01 02 03 04 05 06 07 08 09 01 02 03 04 05 06 07

AB BA 00 14 0E 00 0A FF FF FF FF FF FF 01 02 03 04 05 06 17

AB BA 00 15 00 15

AB BA 00 16 05 2E 00 B6 A3 02 2A

AB BA 17 07 0A FF FF FF FF FF FF 1A

6.Applications:

1>.Attendance Fingerprint Read Write Module

2>.Entrance Guard Intercom Read Write Module

3>.Replicator
*/
