#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

// Initialize the LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Adjust the address if different for your LCD

#define RST_PIN         16   //pin-D0       
#define SS_PIN          0    //pin-D3
byte buzzer = 15;            //pin D8
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance

const char* ssid = "OPPOA53";    //Your Wifi SSID
const char* password = "12345678";   //Wifi Password
String server_addr= "192.168.43.73";  //your server address or computer IP

byte readCard[4];   
uint8_t successRead;    
String UIDCard;

void setup() {
  pinMode(buzzer, OUTPUT);
  Serial.begin(115200);
  SPI.begin();                      
  mfrc522.PCD_Init();               
  Serial.println(F("Read Uid data on a MIFARE PICC:"));
  ShowReaderDetails();

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.print("ARDUCODING");
  lcd.setCursor(0,1);
  lcd.print("Mesin Absen");
  delay(1000);
  
  ConnectWIFI(); 
  delay(2000);
}

void loop() { 
  lcd.clear();
  lcd.print("Tempelkan Kartu");
  successRead = getID();
}

uint8_t getID() {
  if (!mfrc522.PICC_IsNewCardPresent()) { 
    return 0;
  }
  if (!mfrc522.PICC_ReadCardSerial()) {   
    return 0;
  }
  UIDCard ="";
  Serial.println(F("Scanned PICC's UID:"));
  for (uint8_t i = 0; i < mfrc522.uid.size; i++) {  
    UIDCard += String(mfrc522.uid.uidByte[i], HEX);
  }
  UIDCard.toUpperCase(); 
  Serial.print("UID:");
  Serial.println(UIDCard);
  Serial.println(F("**End Reading**"));
  digitalWrite(buzzer, HIGH); delay(200);
  digitalWrite(buzzer, LOW); delay(200);
  digitalWrite(buzzer, HIGH); delay(200);
  digitalWrite(buzzer, LOW);
  
  storeData();
  delay(2000); 
  mfrc522.PICC_HaltA();
  return 1;
}

void storeData() {
  ConnectWIFI();
  WiFiClient client;
  String address, message, first_name;
  
  address = "http://" + server_addr + "/absensi/webapi/api/create.php?uid=" + UIDCard;
  
  HTTPClient http;  
  http.begin(client, address);
  int httpCode = http.GET();        
  String payload; 
  Serial.print("Response: "); 
  if (httpCode > 0) {               
      payload = http.getString();   
      payload.trim();
      if(payload.length() > 0) {
         Serial.println(payload + "\n");
      }
  }
  http.end();

  const size_t capacity = JSON_OBJECT_SIZE(4) + 70;
  DynamicJsonDocument doc(capacity);
      
  DeserializationError error = deserializeJson(doc, payload);
  
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }
  
  const char* waktu_res = doc["waktu"];
  String nama_res = doc["nama"];
  const char* uid_res = doc["uid"];
  String status_res = doc["status"];

  for(int i = 0; i < nama_res.length(); i++) {
    if(nama_res.charAt(i) == ' ') {
      first_name = nama_res.substring(0, i);
      break;
    }
  }
  
  lcd.clear();
  if (status_res == "INVALID") {
    lcd.print("Siapa Kamu?");
    lcd.setCursor(0,1);
    lcd.print(uid_res);
  } else {
    if (status_res == "IN") {
      lcd.print("Selamat Datang, " + first_name);
    } else {
      lcd.print("Sampai Jumpa, " + first_name);
    }
    lcd.setCursor(0,1);
    lcd.print(waktu_res);
  }
  delay(3000);
}

void ConnectWIFI() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    int i = 0;
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      lcd.clear();
      lcd.print("Connecting...");
      delay(1000);
      ++i;
      if (i == 30) {
        i = 0;
        Serial.println("\n Failed to Connect.");
        lcd.clear();
        lcd.print("Failed to Connect");
        break;
      }
    }
    Serial.println("\n Connected!"); 
    lcd.clear();
    lcd.print("Connected to WiFi");
    delay(2000);
  }
}

void ShowReaderDetails() {
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  Serial.print(F("MFRC522 Software Version: 0x"));
  Serial.print(v, HEX);
  if (v == 0x91) {
    Serial.print(F(" = v1.0"));
  } else if (v == 0x92) {
    Serial.print(F(" = v2.0"));
  } else {
    Serial.print(F(" (unknown), probably a chinese clone?"));
  }
  Serial.println("");
  if ((v == 0x00) || (v == 0xFF)) {
    Serial.println(F("WARNING: Communication failure, is the MFRC522 properly connected?"));
    Serial.println(F("SYSTEM HALTED: Check connections."));
    while (true); 
  }
}
