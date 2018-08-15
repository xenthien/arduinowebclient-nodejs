
#include <SPI.h>
#include <PN532_SPI.h>
#include "PN532.h"
#include <Ethernet.h>
#include <LiquidCrystal_I2C.h>

PN532_SPI pn532spi(SPI, 9);
PN532 nfc(pn532spi);

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
char serverName[] = "10.99.201.180";
int serverPort = 1190;
EthernetClient client;

LiquidCrystal_I2C lcd(0x3F, 16, 2);

void setup(void) {
  Serial.begin(115200);
  Serial.println("Hello!");

  lcd.init();
  lcd.backlight();

  SPI.setBitOrder(MSBFIRST);
  if (Ethernet.begin(mac) == 0) {
    while (true) {
      Serial.println("No network connection!");
      delay(1000);
      lcd.setCursor(3, 0);
      lcd.print("No Network");
      lcd.setCursor(3, 1);
      lcd.print("Connection");
      delay(1000);
      lcd.clear();
    }
  } else {
    Serial.print("Device IP Address:  ");
    Serial.println(Ethernet.localIP());
    Serial.println("");
    lcd.setCursor(3, 0);
    lcd.print("Device IP:");
    lcd.setCursor(1, 1);
    lcd.print(Ethernet.localIP());
    delay(3000);
  }

  SPI.setBitOrder(LSBFIRST);
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  Serial.print("Found chip PN5"); Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware ver. "); Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.'); Serial.println((versiondata >> 8) & 0xFF, DEC);

  nfc.SAMConfig();
}


void loop(void) {
  //Serial.println("Waiting for an ISO14443A Card ...");
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (success) {
    Serial.println("Found an ISO14443A card");
    Serial.print("  UID Length: ");
    Serial.print(uidLength, DEC);
    Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);

    if (uidLength == 4) {
      uint32_t cardid = uid[0];
      cardid <<= 8;
      cardid |= uid[1];
      cardid <<= 8;
      cardid |= uid[2];
      cardid <<= 8;
      cardid |= uid[3];
      Serial.println("Seems to be a Mifare Classic card");
      Serial.print("Card ID #");
      Serial.println(cardid);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Sending data...");
      lcd.setCursor(5, 1);
      lcd.print(cardid);
      lcd.setCursor(0, 1);
      lcd.print("ID#:");
      delay(1000);
      SPI.setBitOrder(MSBFIRST);
      String idno;
      idno = cardid;
      insertToDb(idno);
      Serial.println("");
      SPI.setBitOrder(LSBFIRST);
      delay(500);
    }
  } else {
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("Scan CARD!");
    lcd.setCursor(6, 1);
    lcd.print("Here");
    delay(500);
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("Scan     !");
    lcd.setCursor(6, 1);
    lcd.print("Here");
    delay(500);
  }
}

void insertToDb(String value) {
  String postData = "value=" + value;
  Serial.println("sending...");
  Serial.println("connecting...");

  if (client.connect(serverName, serverPort)) {
    Serial.println("connected");
    client.println("POST /api/card HTTP/1.1");
    client.print("Host:");
    client.println(serverName);
    client.println("Content-Type: application/x-www-form-urlencoded"); 
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(postData.length());
    client.println();
    client.print(postData);
    Serial.println(postData);
    client.println();
    Serial.println("Insert complete!");
    client.stop();
    lcd.clear();
    lcd.setCursor(4, 0);
    lcd.print("Sending");
    lcd.setCursor(3, 1);
    lcd.print("Completed!");
    delay(1000);
  } else {
    Serial.println("connection failed");
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("Sending Failed");
    lcd.setCursor(3, 1);
    lcd.print("Try Again!");
    delay(1000);
  }

  client.stop();
}

