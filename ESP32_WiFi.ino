#include "Arduino.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <MFRC522.h>
#include<SPI.h>
#define RSTPIN 22
#define SSPIN 21

//instanciate udp
WiFiUDP udp;
//Instanciate MFRC
MFRC522 mfrc(SSPIN, RSTPIN);

//Packet
const byte numChars = 32;
boolean newSignup = false, newDelete = false, newSet = false, connectedd = false;
char signup[35], del[11], set[11], readCard[4], bufferr[10], consumo[10], receivedChars[numChars], *protocolid = "#";
char greenList[3][9] = {0}, blackList[3][9] = {0}, inBuffer[255]; // Buffer from greenList, blackList and to store the incomming message from server

byte readcard[4]; // Buffer to store UID after being read
int readsuccess, sendNotSend = 0, listPos = 0; // Control variables
const int serverPort = 11111; //Server door
// Variables to the delivery ratio packet test
uint32_t ts4, ts6;
unsigned long timee, tmin = 1000000, tmax = 0;
int packet_Reception = 0, sendedPackets;

//WiFi Settings
const char* ssid = "Vodafone-6EC95F";
const char* password = "PASSWORD";
const char* serverAddress = "192.168.1.70";
/*
const char* ssid = "NOS-D7D5";
const char* password = "PASSWORD";
const char* serverAddress = "192.168.1.10";

const char* ssid = "Helder";
const char* password = "PASSWORD";
const char* serverAddress = "10.42.0.1";

*/
void setup() {

  Serial.begin(9600);
  SPI.begin();
  mfrc.PCD_Init(); // Initialize the MFRC522 module
  while (!Serial) {
    ; // wait for serial port to connect.
  }

  Serial.println();
  connectToWiFi(ssid, password); // Connection to the Access Point
  if (connectedd = true) {
    Serial.println("Conectado ao ponto de acesso");
  }
}

void loop() {
  memset(bufferr, '\0', sizeof(bufferr));
  memset(consumo, '\0', sizeof(consumo));
  memset(inBuffer, '\0', 255); //clear reception buffer
  memset(signup, '\0', sizeof(signup));
  memset(receivedChars, '\0', sizeof(receivedChars));

  newPacket();
  readsuccess = getid();  //Check if any PICC is in PCD reading range and if true, read it

  if (readsuccess) {
    //Check if UID exists in greenList or blackList
    searchList(bufferr);
    // If not, send the permission package to server
    if (sendNotSend == 1) {
      WriteUID(serverAddress, serverPort, bufferr, 9); //Sending the package to server
      Serial.print("E: "); Serial.println(sendedPackets++);
    }
    // If UID is in greenList, send the consumption package
    if (sendNotSend == 0) {
      consumo[0] = '%'; // Consumption package type character
      strncpy(consumo + 1, bufferr + 1, 8 ); //Assemble the package
      WriteUID(serverAddress, serverPort, consumo, 9); // Send the package
      //Serial.println("Pacote de contagem de consumo enviado");
    }
    else if (sendNotSend == 2) { //Don't send anything
    }
  }


  int packetSize = udp.parsePacket();
  if (packetSize) { // If there is any message from the server

    while (udp.read(inBuffer, 255) > 0) { // Read it
      udp.parsePacket();
      ts6 = micros();
      //Serial.print("Pacote do servidor:"); Serial.println(inBuffer);
      timee += (ts6 - ts4);
      if ((ts6 - ts4) > tmax) {
        tmax = (ts6 - ts4);
      }
      Serial.print("Tmax: "); Serial.println(tmax);
      if ((ts6 - ts4) < tmin) {
        tmin = (ts6 - ts4);
      }
      Serial.print("Tmin: "); Serial.println(tmin);
      Serial.print("Tempo Total: "); Serial.println(timee);
      Serial.print("T_Pacote: "); Serial.println(ts6 - ts4);
      Serial.print("R: "); Serial.println(packet_Reception ++);

      if (inBuffer[0] == '?') { // Package to store UID in greenList
        //Serial.println("Guardar UID na lista verde");
        saveBicycleGreen(inBuffer); // Save it in greenList
      }
      else if (inBuffer[0] == '/') {  // Package to store UID in blackList
        //Serial.println("Guardar UID na lista negra");
        saveBicycleBlack(inBuffer); // Save it in blackList
      }
      else if ((inBuffer[0] == '(') || (inBuffer[0] == ')')) {  //Package to change UID from blackList to greenList or vice-versa
        updateList(inBuffer);
      }
      udp.flush();
    }
  }
}
//////////////////////////////////////////////////////***************************************************////////////////////////////////////////////////////////
//FUNCTION to search for UID in a list, DELETE if it exists and shift left UIDs from the LIST
void updateList(char *incomming) {
  int existsGR = 0, existsBL = 0, posicao = 0, maxSize = 3;
  char *uid[8];

  //If UID is to put in the greenList
  if (strncmp(incomming, "(", 1) == 0) {
    *uid = strtok(incomming, "(");

    //Check if UID exists in blackList, and if yes, delete it
    for ( int i = 0; i <= (maxSize - 1); i++) {
      if ((strlen(blackList[i]) == 8) && (strncmp(blackList[i], *uid, 8) == 0)) {
        Serial.println("UID existe na BlackList");
        existsBL = 1;
        posicao = i;
        break;
      }
    }
    //SHIFT to left on blackList UIDs to delete the UID that goes to greenList
    if (existsBL == 1) {
      for (int pos = posicao; pos < maxSize; pos++) { //Shifting to the left if the position of the list its occupied
        if ((strlen(blackList[pos]) == 8) && (strlen(blackList[pos + 1]) != 0)) {
          strncpy(blackList[pos], blackList[pos + 1], 8);
        }
        else {
          memset(blackList[pos], '\0', 8);
          break;
        }
      }
      memset(blackList[maxSize - 1], '\0', 8);
      Serial.println("UID eliminado da BlackList");
      for (int k = 0; k < maxSize; k++) {
        Serial.print("BlackList Atualizada:"); Serial.print(k); Serial.print(":"); Serial.println(blackList[k]);
      }
    }
    SaveGreenListUpdate(*uid);//Save UID in the first empty position of the greenList
  }

  // If the packet is to put the UID into the blackList and remove it from the greenList if exists
  else {
    *uid = strtok(incomming, ")");
    for (int j = 0; j <= (maxSize - 1); j++) {
      if ((strlen(greenList[j]) == 8) && (strncmp(greenList[j], *uid, 8) == 0)) { // Check if UID exists in the greenList
        Serial.println("UID existe na GreenList");
        posicao = j;
        existsGR = 1;
        break;
      }
    }
    //If UID existe in the greenList, shit the other UIDs to delete it
    if (existsGR == 1) {
      for (int pos = posicao; pos < maxSize; pos++) {
        if ((strlen(greenList[pos]) == 8) && (strlen(greenList[pos + 1]) != 0)) {
          strncpy(greenList[pos], greenList[pos + 1], 8);
        }
        else {
          memset(greenList[pos], '\0', 8);
          break;
        }
      }
      memset(greenList[maxSize - 1], '\0', 8);
      Serial.println("UID eliminado da GreenList");
      for (int l = 0; l < maxSize; l++) {
        Serial.print("GreenList Atualizada:"); Serial.print(l); Serial.print(":"); Serial.println(greenList[l]);
      }
    }
    SaveBlackListUpdate(*uid);//Save the UID in the first empty position of the blackList
  }
}

void SaveGreenListUpdate(char *incomming) { // Save UID in the greenList after reception of update package
  int maxSize = 3;
  for (int i = 0; i <= (maxSize - 1); i++) {

    if ((strlen(greenList[i]) == 8)) { //If [i] position of the list is filled, compare the UIDs
      if (strncmp(greenList[i], incomming, 8) == 0) { // If UID exists in the list, the search is stopped
        break;
      }
    }
    //Position [i] from the list it's empty
    else {
      snprintf(greenList[i], 9, "%s", incomming); // Copy the new UID to the greenList [i] position
      break;
    }
    //If the greenList is full and the UID doesn't exists
    if (i == (maxSize - 1)) {
      Serial.println("Remover UID mais antigo e inserir novo UID na GreenList");
      for (int j = 0; j < maxSize - 1; j++) {
        snprintf(greenList[j], 9, "%s", greenList[j + 1]); //Shift to the left
      }
      snprintf(greenList[maxSize - 1], 9, "%s", incomming); //Store UID in the last position of the greenList
    }
  }
  Serial.println("Novo UID na GreenList");
  for (int k = 0; k < maxSize; k++) {
    Serial.print("GreenList Final:"); Serial.print(k); Serial.print(":"); Serial.println(greenList[k]);
  }
}

void SaveBlackListUpdate(char *incomming) { // Save UID in the blackList after reception of update package
  int maxSize = 3;
  for (int i = 0; i <= maxSize; i++) {
    if (strlen(blackList[i]) == 8) { //If [i] position of the list is filled, compare the UIDs
      if (strncmp(blackList[i], incomming, 8) == 0) { // If UID exists in the list, the search is stopped
        break;
      }
    }
    //Position [i] from the list it's empty
    else {
      snprintf(blackList[i], 9, "%s", incomming); // Copy the new UID to the blackList [i] position
      break;
    }
    //If the blackList is full and the UID doesn't exists
    if (i == (maxSize - 1)) {
      Serial.println("Remover UID mais antigo e inserir novo UID na BlackList");
      for (int j = 0; j < maxSize - 1; j++) {
        //Serial.println("SHIFT STORE BL");
        snprintf(blackList[j], 9, "%s", blackList[j + 1]); //Shift to the left
      }
      snprintf(blackList[maxSize - 1], 9, "%s", incomming); //Store UID in the last position of the blackList
    }
  }
  Serial.println("Novo UID na BlackList");
  for (int k = 0; k < maxSize; k++) {
    Serial.print("BlackList Final:"); Serial.print(k); Serial.print(":"); Serial.println(blackList[k]);
  }
}


//After the tag is read, the UID is  searched in both lists and if doesnt exists, an authorization packet is sent to the server
int searchList(char *incomming) {
  int existsGR = 0, existsBL = 0, maxSize = 3 ;
  char *uid[8];
  *uid = strtok(incomming, "#");

  for (int i = 0; i <= (maxSize - 1); i++) { // Check if UID exists in the greenList
    if ((strlen(greenList[i]) == 8) && (strncmp(greenList[i], *uid, 8) == 0)) {
      Serial.println("UID existe na GreenList");
      listPos = i;
      existsGR = 1;
      break;
    }
  }
  //UID doesn't exists in the greenList, so lets check on the blackList
  if (existsGR == 0) {
    for ( int j = 0; j <= (maxSize - 1); j++) {
      if ((strlen(blackList[j]) == 8) && (strncmp(blackList[j], *uid, 8) == 0)) {
        listPos = j;
        existsBL = 1;
        Serial.println("UID existe na BlackList");
        break;
      }
    }
  }
  //If UID exists on greenList, UID must pass to the last position os the list
  if ((existsGR == 1) && (existsBL == 0)) {
    //Check is the last position of the greenList is filled and if the last UID its different from the UID being checked
    if ((strlen(greenList[maxSize - 1]) == 8) && (strncmp(greenList[maxSize - 1], *uid, 8) != 0)) {
      for (int pos = listPos; pos < (maxSize - 1); pos++) {
        strncpy(greenList[pos], greenList[pos + 1], 8);
      }
      snprintf(greenList[maxSize - 1], 9, "%s", *uid); //Store UID in the last position os the greenList
      Serial.println("UID existe na GreenList, passa para a cabeça da lista");
      for (int k = 0; k < maxSize; k++) {
        Serial.print("GreenList atualizada:"); Serial.print(k); Serial.print(":"); Serial.println(greenList[k]);
      }
    }
    sendNotSend = 0;
  }
  //If the UID does not exist in any of the lists
  if ((existsGR == 0) && (existsBL == 0)) {
    //Serial.println("Nao existe. Enviar pedido de autorizacao");
    sendNotSend = 1;
  }
  //UID exists in the blackList
  if (existsBL == 1) {
    //Serial.println("Search Lista negra");
    // Check if the last position  of the list is filled
    if ((strlen(blackList[maxSize - 1]) == 8) && (strncmp(blackList[maxSize - 1], *uid, 8) != 0)) {
      for (int pos = listPos; pos < (maxSize - 1); pos++) {
        strncpy(blackList[pos], blackList[pos + 1], 8);
      }
      snprintf(blackList[maxSize - 1], 9, "%s", *uid); //Store UID in the last position of the blackList
      Serial.println("UID existe na BlackList, passa para a cabeça da lista");
      for (int k = 0; k < maxSize; k++) {
        Serial.print("BlackList atualizada:"); Serial.print(k); Serial.print(":"); Serial.println(blackList[k]);
      }
    }
    sendNotSend = 2;
  }
  return sendNotSend;
}

// If the packet is the response to a permission request packet and the UID should be stored in the greenList
void saveBicycleGreen(char *incomming) {
  char *uid[8];
  int maxSize = 3;
  *uid = strtok(incomming, "?"); // The character that identifies the packet to put the UID in the greenList

  for (int i = 0; i <= (maxSize - 1); i++) { // Ckeck if the greenList has an empty position
    if ((strlen(greenList[i]) == 8)) {
      if (strncmp(greenList[i], *uid, 8) == 0) { // If the UID already exists, stop the search
        break;
      }
    }
    //Position I of the greenList is empty
    else {
      Serial.println("Inserir UID na GreenList");
      snprintf(greenList[i], 9, "%s", *uid); // Put the UID in the i position of the greenList
      break;
    }
    if (i == (maxSize - 1)) { //If the list is fully occupied, then the UID will be stored in the last position od the greenList
      Serial.println("Remover UID mais antigo e inserir novo UID na GreenList");
      for (int j = 0; j < maxSize - 1; j++) {
        snprintf(greenList[j], 9, "%s", greenList[j + 1]);
      }
      snprintf(greenList[maxSize - 1], 9, "%s", *uid);
    }
  }
  Serial.println("GreenList após novo UID ser inserido");
  // Print the greenList to verify that the new UID is stored
  for (int k = 0; k < maxSize; k++) {
    Serial.print("GreenList"); Serial.print(k); Serial.print(":"); Serial.println(greenList[k]);
  }
}

// If the packet is the response to a permission request packet and the UID should be stored in the blackList
void saveBicycleBlack(char *incomming) {
  char *uid[8];
  int maxSize = 3;
  *uid = strtok(incomming, "/"); // The character that identifies the packet to put the UID in the blackList

  for (int i = 0; i <= (maxSize - 1); i++) { // Ckeck if the greenList has an empty position
    if (strlen(blackList[i]) == 8) {
      if (strncmp(blackList[i], *uid, 8) == 0) { // If the UID already exists, stop the search
        break;
      }
    }
    //Position I of the greenList is empty
    else {
      Serial.println("Inserir UID na BlackList");
      snprintf(blackList[i], 9, "%s", *uid); // Put the UID in the i position of the blackList
      break;
    }
    //If the list is fully occupied, then the UID will be stored in the last position of the greenList
    if (i == (maxSize - 1)) {
      Serial.println("Remover UID mais antigo e inserir novo UID na BlackList");
      for (int j = 0; j < maxSize - 1; j++) {
        snprintf(blackList[j], 9, "%s", blackList[j + 1]);
      }
      snprintf(blackList[maxSize - 1], 9, "%s", *uid);
    }
  }
  Serial.println("BlackList após novo UID ser inserido");
  // Print the blackList to verify that the new UID is stored
  for (int k = 0; k < maxSize; k++) {
    Serial.print("BlackList:"); Serial.print(k); Serial.print(":"); Serial.println(blackList[k]);
  }
}

//Reading the management packets from the serial monitor
void newPacket() {
  static boolean recvInProgress = false, receiving = false, receiveSet = false;
  static int ndx = 0;
  char startMarker = '&', endMarker = '$', startDelete = '@', startSet = '!', rc;
  int successSignup = 0, successDelete = 0, successSet = 0;

  while (Serial.available() > 0 && newSignup == false) {
    rc = Serial.read();

    if (recvInProgress == true) {
      if (rc != endMarker) { // Detecting the last character -> $
        receivedChars[ndx] = rc;
        ndx++;
        if (ndx >= numChars) {
          ndx = numChars - 1;
        }
      }
      else { // Registration package
        successSignup = 0;
        char* s = new char[ndx + 1];
        receivedChars[ndx] = '\0'; // Terminate the string
        recvInProgress = false;
        while (!successSignup) {
          successSignup = getid(); // Obtaining the UID from the client's TAG to complete the registration process
        }
        snprintf(signup, sizeof(signup), "&%s%s", receivedChars, bufferr); // Assemble the registration packet
        WriteUID(serverAddress, serverPort, signup, ndx + 10); // Write REGISTRATION packet for the server
        ndx = 0;
      }
    }
    else if ( receiving == true) {
      if (rc != endMarker) {
        receivedChars[ndx] = rc;
        ndx++;
        if (ndx >= numChars) {
          ndx = numChars - 1;
        }
      }
      else { //DELETE account packet
        memset(del, '\0', sizeof(del));
        successDelete = 0;
        char* d = new char[ndx + 1];
        receiving = false;
        while (!successDelete) {
          successDelete = getid(); // Obtaining the UID from the client's TAG to complete the account delete process
          strcpy(bufferr, bufferr + 1); // Remove the # character from the UID

        }
        snprintf(del, sizeof(del), "@%s%s", receivedChars, bufferr); // Assemble the packet
        WriteUID(serverAddress, serverPort, del, ndx + 9); // Send delete account packet for the server
        ndx = 0;
      }
    }
    else if ( receiveSet == true) {
      if (rc != endMarker) {
        receivedChars[ndx] = rc;
        ndx++;
        if (ndx >= numChars) {
          ndx = numChars - 1;
        }
      }
      else { //Payment packet
        memset(set, '\0', sizeof(set));
        successSet = 0;
        char* Set = new char[ndx + 1];
        receiveSet = false;
        while (!successSet) {
          successSet = getid(); // Obtaining the UID from the client's TAG to complete the payment process
          strcpy(bufferr, bufferr + 1); // Remove the # character from the UID

        }
        snprintf(set, sizeof(set), "!%s%s", receivedChars, bufferr);
        WriteUID(serverAddress, serverPort, set, ndx + 9); // Write Payment packet for the server
        ndx = 0;
      }
    }
    else if ((rc == startMarker)) {
      recvInProgress = true;
    }
    else if (rc == startDelete) {
      receiving = true;
    }
    else if (rc == startSet) {
      receiveSet = true;
    }
  }
}
// Function that sends UID packet to server, k is the size
void WriteUID(const char* address, int port, char* package, int k)
{
  //stabilishing the route
  udp.beginPacket(address, port);
  ts4 = micros();
  udp.write((uint8_t *)package, k);
  udp.endPacket();
}

// Obtaining the UID from the tag which the bicycle is passing by
int getid() {
  if (!mfrc.PICC_IsNewCardPresent()) { // Detecting if the PICC is in the range of PCD
    return 0;
  }
  if (!mfrc.PICC_ReadCardSerial()) { // PICC is in the PCD reading range, and its read
    return 0;
  }
  for (int i = 0; i < 4; i++) {
    readcard[i] = mfrc.uid.uidByte[i]; // Storing the UID of the tag in readcard
  }
  Serial.println();
  array_to_string(readcard, 4, readCard); // Cast the 4 bytes into a string
  snprintf(bufferr, sizeof(bufferr), "%s%s", protocolid, readCard); // Assemble the ID package ( concat the # character with the UID)
  Serial.print("UID:"); Serial.println(bufferr);
  mfrc.PICC_HaltA(); // Pause the communication  between PCD and PICC
  return 1;
}
//////////////////////////////////////***********************************************////////////////////////////////////////////////////////
//Connect to WiFi network
void connectToWiFi(const char * ssid, const char * pwd) {
  delay(500);
  Serial.println("Connecting to WiFi network: " + String(ssid));
  // delete old config
  WiFi.disconnect(true);
  //register event handler
  WiFi.onEvent(WiFiEvent);
  //Initiate connection
  WiFi.begin(ssid, pwd);
  Serial.println("Waiting for WIFI connection...");
}
//wifi event handler
void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.print("Endereço IP do móduloESP32: ");
      Serial.println(WiFi.localIP());
      udp.begin(WiFi.localIP(), serverPort);
      connectedd = true;
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi lost connection");
      connectedd = false;
      break;
  }
}
// Convert byte array to char array
void array_to_string(byte array[], unsigned int len, char buffer[])
{
  for (unsigned int i = 0; i < len; i++)
  {
    byte nib1 = (array[i] >> 4) & 0x0F;
    byte nib2 = (array[i] >> 0) & 0x0F;
    buffer[i * 2 + 0] = nib1  < 0xA ? '0' + nib1  : 'A' + nib1  - 0xA;
    buffer[i * 2 + 1] = nib2  < 0xA ? '0' + nib2  : 'A' + nib2  - 0xA;
  }
  buffer[len * 2] = '\0';
}
