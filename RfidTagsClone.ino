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
