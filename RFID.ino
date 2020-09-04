#include <Wire.h>
#include "MFRC522_I2C.h"
#include <M5Stack.h>
#include <TinyGPS++.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define WIFI_STA_NAME "xxx"
#define WIFI_STA_PASS "yyy"

// 0x28 is i2c address on SDA. Check your address with i2cscanner if not match.
MFRC522 mfrc522(0x28);   // Create MFRC522 instance.
String address = "0x1D5331AB36b1921Db051680ecB938c4D9107351f";
String shopName = "IceZz shop";
String server_url = "https://defi-api.iyawat.com";

static const uint32_t GPSBaud = 9600;
// The TinyGPS++ object
TinyGPSPlus gps;

// The serial connection to the GPS device
HardwareSerial ss(2);

void setup() {
  M5.begin();
  M5.Lcd.fillScreen( BLACK );
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextColor(YELLOW);  
  M5.Lcd.setTextSize(2);

  M5.Lcd.fillScreen( BLACK );
  M5.Lcd.setCursor(0, 0);
//  M5.Lcd.println("M5StackFire MFRC522");
  Serial.begin(115200);           // Initialize serial communications with the PC
  Wire.begin();                   // Initialize I2C
  
  mfrc522.PCD_Init();             // Init MFRC522
  ShowReaderDetails();            // Show details of PCD - MFRC522 Card Reader details

  ss.begin(GPSBaud);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_STA_NAME, WIFI_STA_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
//  Serial.println(F("Scan PICC to see UID, type, and data blocks..."));
//  M5.Lcd.println("Scan PICC to see UID, type, and data blocks...");
}

void loop() {

  M5.Lcd.setCursor(0, 70);
  M5.Lcd.setTextColor(WHITE, BLACK);
//  M5.Lcd.setTextSize(1);
  
  String txt = "address="+address+"&lat="+String(gps.location.lat())+"&lng="+String(gps.location.lng());
  
  M5.Lcd.drawString("Please scan QR or Tap card", 1, 5, 1);
  M5.Lcd.drawString("SHOP: ", 1, 25, 1);
  M5.Lcd.drawString(shopName, 60, 25, 1);
  
  M5.Lcd.qrcode(txt, 10, 50, 150);

  M5.Lcd.setTextColor(GREEN, BLACK);
  M5.Lcd.drawString("LAT: ", 200, 80, 1);
  M5.Lcd.drawFloat(gps.location.lat(), 5, 200, 100);

  M5.Lcd.drawString("LONG: ", 200, 140, 1);
  M5.Lcd.drawFloat(gps.location.lng(), 5, 200, 160);
 
  M5.Lcd.setTextColor(RED, BLACK);
  M5.Lcd.drawString("ThaiChana on Thaichain", 5, 200, 2); 

//  Serial.println(gps.charsProcessed());
//  Serial.println(gps.location.lat(),5);
//  Serial.println(gps.location.lng(),5);
  
  smartDelay(1000);
  // Look for new cards, and select one if present
  if ( ! mfrc522.PICC_IsNewCardPresent() || ! mfrc522.PICC_ReadCardSerial() ) {
    delay(50);
    return;
  } else {
    
    char str[32] = "";
    array_to_string(mfrc522.uid.uidByte, 4, str);
    M5.Lcd.drawString(str, 200, 60, 1);
  
    String url = server_url+"/checkin/nfc";
    String httpRequestData = "nfc=" + String (str) + "&geo=" + String (gps.location.lat(),5) + ", " + String(gps.location.lng(),5);           
      
    Serial.println("New Card");
    Serial.println(str);
    Serial.println(url);
    Serial.println(httpRequestData);
    
    HTTPClient http;
    http.setTimeout(30000);
    http.begin(url);
    http.setTimeout(30000);
    http.setConnectTimeout(30000);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded"); 

    M5.Lcd.clear();
    M5.Lcd.drawString("Loading...", 50, 100, 4);
    int httpCode = http.POST(httpRequestData);
    Serial.println(httpCode);
    
    if (httpCode == 200) {
      String response = http.getString();
      Serial.println(response);
      M5.Lcd.clear();
      
      DynamicJsonDocument doc(2048);
      deserializeJson(doc, response);

      const String result = doc["result"]; 
      const String hash = doc["transactionHash"]; 
      const String balance = doc["balance"]; 

      M5.Lcd.setTextColor(GREEN, BLACK);
      M5.Lcd.drawString("** Check-In Result ** ", 10, 10, 2);
      M5.Lcd.setTextColor(WHITE, BLACK);
      M5.Lcd.drawString("Card: ", 10, 60, 2);
      M5.Lcd.drawString(str, 100, 60, 2);
      M5.Lcd.drawString("Result: ", 10, 100, 2);
      M5.Lcd.drawString(result, 100, 100, 2);
      M5.Lcd.drawString("Balance:", 10, 140, 2);
      M5.Lcd.drawString(balance, 150, 140, 2);

      Serial.println(result);
      Serial.println(hash);
      Serial.println(balance);

      smartDelay(5000);
      M5.Lcd.clear();
    }
  }

  M5.Lcd.fillRect(200, 60, 100, 20, BLACK);      

  M5.update();
  
}

void ShowReaderDetails() {
  // Get the MFRC522 software version
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  Serial.print(F("MFRC522 Software Version: 0x"));
  Serial.print(v, HEX);
  if (v == 0x91)
    Serial.print(F(" = v1.0"));
  else if (v == 0x92)
    Serial.print(F(" = v2.0"));
  else
    Serial.print(F(" (unknown)"));
  Serial.println("");
  // When 0x00 or 0xFF is returned, communication probably failed
  if ((v == 0x00) || (v == 0xFF)) {
    Serial.println(F("WARNING: Communication failure, is the MFRC522 properly connected?"));
  }
}

// This custom version of delay() ensures that the gps object
// is being "fed".
static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}

static void printFloat(float val, bool valid, int len, int prec)
{
  if (!valid)
  {
    while (len-- > 1)
      M5.Lcd.print('*');
    M5.Lcd.print(' ');
  }
  else
  {
    M5.Lcd.print(val, prec);
    int vi = abs((int)val);
    int flen = prec + (val < 0.0 ? 2 : 1); // . and -
    flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
    for (int i=flen; i<len; ++i)
      M5.Lcd.print(' ');
  }
  smartDelay(0);
}

static void printInt(unsigned long val, bool valid, int len)
{
  char sz[32] = "*****************";
  if (valid)
    sprintf(sz, "%ld", val);
  sz[len] = 0;
  for (int i=strlen(sz); i<len; ++i)
    sz[i] = ' ';
  if (len > 0) 
    sz[len-1] = ' ';
  M5.Lcd.print(sz);
  smartDelay(0);
}

static void printDateTime(TinyGPSDate &d, TinyGPSTime &t)
{
  if (!d.isValid())
  {
    M5.Lcd.print(F("********** "));
  }
  else
  {
    char sz[32];
    sprintf(sz, "%02d/%02d/%02d ", d.month(), d.day(), d.year());
    M5.Lcd.print(sz);
  }
  
  if (!t.isValid())
  {
    M5.Lcd.print(F("******** "));
  }
  else
  {
    char sz[32];
    sprintf(sz, "%02d:%02d:%02d ", t.hour(), t.minute(), t.second());
    M5.Lcd.print(sz);
  }

  printInt(d.age(), d.isValid(), 5);
  smartDelay(0);
}

static void printStr(const char *str, int len)
{
  int slen = strlen(str);
  for (int i=0; i<len; ++i)
    M5.Lcd.print(i<slen ? str[i] : ' ');
  smartDelay(0);
}

void array_to_string(byte array[], unsigned int len, char buffer[])
{
   for (unsigned int i = 0; i < len; i++)
   {
      byte nib1 = (array[i] >> 4) & 0x0F;
      byte nib2 = (array[i] >> 0) & 0x0F;
      buffer[i*2+0] = nib1  < 0xA ? '0' + nib1  : 'A' + nib1  - 0xA;
      buffer[i*2+1] = nib2  < 0xA ? '0' + nib2  : 'A' + nib2  - 0xA;
   }
   buffer[len*2] = '\0';
}
