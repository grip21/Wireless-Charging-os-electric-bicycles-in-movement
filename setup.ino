#include <MFRC522.h>
#include<SPI.h>

//creating mfrc522 instance
#define RSTPIN 22
#define SSPIN 21
#define controlClockPin 0

MFRC522 mfrc(SSPIN, RSTPIN);

int readsuccess;
uint32_t ts2;
uint32_t ts1;
int counter = 0;
byte greenList[][4] = {{0xD2, 0xC3, 0x50, 0x43}, {0x82, 0x1A, 0xD5, 0x83}, {0x03, 0xDA, 0xD2, 0x83}}; //IDs of four tags
int N = 3; //change this to the number of cards/tags you will use
byte readcard[4]; //stores the UID of current tag which is read

void setup() {
  pinMode(controlClockPin, OUTPUT);
  digitalWrite(controlClockPin, LOW);

  Serial.begin(9600);
  SPI.begin();
  mfrc.PCD_Init();
  Serial.println();
}

//Code to read tag's UID
int getid() {
  digitalWrite(controlClockPin, HIGH);
  if (!mfrc.PICC_IsNewCardPresent()) {
    return 0;
  }
  if (!mfrc.PICC_ReadCardSerial()) {
    return 0;
  }
  mfrc.PICC_HaltA();
  for (int i = 0; i < 4; i++) {
    readcard[i] = mfrc.uid.uidByte[i]; //Store UID on a temporary buffer
    //Serial.print(readcard[i], HEX);
  }  return 1;
}
void loop() {
  digitalWrite(controlClockPin, LOW);
  delay(2);
  ts1 = micros();
  readsuccess = getid();
  if (readsuccess){
    Serial.println();
    for (int i = 0; i < N; i++) { //Verify if UID is on the GreenList
      if (!memcmp(readcard, greenList[i], 4)) {
        Serial.println("YES");
        break;}
      Serial.println("NO");
    }
    delay(4);
    ts2 = micros();
    Serial.print("T: "); 
    Serial.println(ts2 - ts1);
  }
  digitalWrite(controlClockPin, LOW);
  delay(15);
}
