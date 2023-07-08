#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <AHT10.h>
#define BLINKER_WIFI
#include <Blinker.h>
#include <WiFiManager.h>
#include <IRsend.h>
#include <ir_Gree.h>
#include "Ticker.h"
#include <LittleFS.h>
#include "..\lib\blinker-library-master\src\modules\ArduinoJson\ArduinoJson.h"

// char auth[] = "29455e60830e";
// char ssid[] = "Dell";
// char pswd[] = "1136759016";

#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 32    // OLED display height, in pixels
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

uint8_t readStatus = 0;
AHT10 myAHT20(AHT10_ADDRESS_0X38, AHT20_SENSOR);

uint8_t ir_in_pin = 12;
uint8_t ir_out_pin = 14;
uint8_t led = 16;
IRGreeAC ac(ir_out_pin); // Set the GPIO to be used for sending messages.
String show;
BlinkerButton Button1("BUTTON_1");
BlinkerButton Button2("BUTTON_2");
BlinkerSlider Slider1("temp");
BlinkerSlider Slider2("fengsu");
Ticker timer1;           // 建立Ticker用于实现定时功能
bool screen_change_flag; // 屏幕刷新标志
float humi_read, temp_read;
char auth[20] = "";
char ssid[20] = "";
char pswd[20] = "";
bool shouldSaveConfig = false;

void dataStorage()
{
  Blinker.dataStorage("humi", humi_read);
  Blinker.dataStorage("temp", temp_read);
}

void printState()
{
  // Display the settings.
  Serial.println("GREE A/C remote is in the following state:");
  Serial.printf("  %s\n", ac.toString().c_str());
  // Display the encoded IR sequence.
  unsigned char *ir_code = ac.getRaw();
  Serial.print("IR Code: 0x");
  for (uint8_t i = 0; i < kGreeStateLength; i++)
    Serial.printf("%02X", ir_code[i]);
  Serial.println();
}

void dataRead(const String &data)
{
  BLINKER_LOG("Blinker readString: ", data);
  show = data;
  Blinker.vibrate();
  uint32_t BlinkerTime = millis();
  Blinker.print("millis", BlinkerTime);
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

void button1_callback(const String &state)
{
  BLINKER_LOG("get button state: ", state);
  if (state == BLINKER_CMD_ON)
  {
    ac.on();
    Serial.println("Sending IR command to A/C ...");
    ac.send();
    delay(3000);
    digitalWrite(ir_out_pin, LOW);
    printState();
    BLINKER_LOG("Toggle on!");
    // Button1.icon("icon_1");
    Button1.color("#0090f2");
    Button1.text("NO");
    // Button1.text("Your button name", "describe");
    Button1.print("on");
  }
  else
  {
    ac.off();
    Serial.println("Sending IR command to A/C ...");
    ac.send();
    printState();
    delay(3000);
    digitalWrite(ir_out_pin, LOW);
    BLINKER_LOG("Toggle off!");
    // Button1.icon("icon_1");
    Button1.color("#949494");
    Button1.text("OFF");
    // Button1.text("Your button name", "describe");
    Button1.print("off");
  }
}

void button2_callback(const String &state)
{
  BLINKER_LOG("get button state: ", state);
  if (state == BLINKER_CMD_ON)
  {
    digitalWrite(led, LOW);
    BLINKER_LOG("Toggle on!");
    // Button2.icon("icon_1");
    Button2.color("#0090f2");
    Button2.text("NO");
    // Button1.text("Your button name", "describe");
    Button2.print("on");
  }
  else
  {
    digitalWrite(led, HIGH);
    BLINKER_LOG("Toggle off!");
    // Button2.icon("icon_1");
    Button2.color("#949494");
    Button2.text("OFF");
    // Button1.text("Your button name", "describe");
    Button2.print("off");
  }
}

void slider1_callback(int32_t temp)
{
  BLINKER_LOG("get slider value: ", temp);
  ac.setTemp(temp); // 16-30C
  Serial.println("Sending IR command to A/C ...");
  ac.send();
  printState();
}

void slider2_callback(int32_t fengsu)
{
  BLINKER_LOG("get slider value: ", fengsu);
  ac.setFan(fengsu); // 1-3
  Serial.println("Sending IR command to A/C ...");
  ac.send();
  printState();
}

void screen_change()
{
  screen_change_flag = 1;
}

void init_littlefs()
{
  if (LittleFS.begin())
  {
    // if file not exists
    if (!(LittleFS.exists("/AC.json")))
    {
      LittleFS.open("/AC.json", "w+");
      return;
    }

    File configFile = LittleFS.open("/AC.json", "r");
    if (configFile)
    {
      String a;
      StaticJsonDocument<200> doc;
      while (configFile.available())
      {
        a = configFile.readString();
      }
      Serial.println("");
      Serial.println(a);
      Serial.println(a);
      configFile.close();
      deserializeJson(doc, a);

      if (doc.containsKey("auth"))
      {
        strcpy(auth, doc["auth"]);
      }
      if (doc.containsKey("ssid"))
      {
        strcpy(ssid, doc["ssid"]);
      }
      if (doc.containsKey("pswd"))
      {
        strcpy(pswd, doc["pswd"]);
      }
    }
    configFile.close();
  }
  else
  {
    Serial.println("LittleFS mount failed");
    return;
  }
}

bool saveConfig()
{
  File configFile = LittleFS.open("/AC.json", "r");
  if (!configFile)
  {
    Serial.println("failed to open config file for writing");
    return false;
  }
  configFile.close();
  StaticJsonDocument<200> doc;
  doc["auth"] = auth;
  doc["ssid"] = ssid;
  doc["pswd"] = pswd;
  File fileSaved = LittleFS.open("/AC.json", "w");
  serializeJson(doc, fileSaved);
  serializeJson(doc, Serial);
  Serial.println(fileSaved);
  fileSaved.close();
  // end save
  return true;
}
// 保存wifi信息时的回调函数
void STACallback()
{
  if (LittleFS.begin())
  {
    delay(1000);
    LittleFS.remove("/AC.json");

    LittleFS.end();
    delay(1000);
  }
  // wifiManager.resetSettings();
  // ESP.reset();
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("connected wifi !!!");
  shouldSaveConfig = true;
}
// 设置AP模式时的回调函数
void APCallback(WiFiManager *myWiFiManager)
{
  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("NO connected !!!");
}

void setup()
{
  pinMode(ir_out_pin, OUTPUT);
  digitalWrite(ir_out_pin, LOW);
  pinMode(led, OUTPUT);
  digitalWrite(led, HIGH);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.begin(115200);
  init_littlefs();
  WiFiManager wifiManager;
  WiFiManagerParameter blinker_auth("auth", "blinker_auth(12位)", "", 13);
  WiFiManagerParameter host_hint("<small> Blinker_auth <br></small><br><br>");
  WiFiManagerParameter p_lineBreak_notext("<p></p>");
  wifiManager.setSaveConfigCallback(STACallback);
  wifiManager.setAPCallback(APCallback);

  wifiManager.addParameter(&p_lineBreak_notext);
  // wifiManager.addParameter(&host_hint);
  wifiManager.addParameter(&blinker_auth);

  // wifiManager.resetSettings(); //reset saved settings
  if (!wifiManager.autoConnect("空调助手"))
  {
    // reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  // if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  // {
  //   Serial.println(F("SSD1306 allocation failed"));
  //   for (;;)
  //     ; // Don't proceed, loop forever
  // }
  // while (myAHT20.begin() != true)
  // {
  //   Serial.println(F("AHT20 not connected or fail to load calibration coefficient")); //(F()) save string to flash & keeps dynamic memory free
  //   delay(5000);
  // }
  BLINKER_DEBUG.stream(Serial);

  Serial.println(auth);
  Serial.println(ssid);
  Serial.println(pswd);
  Blinker.begin(auth, ssid, pswd);
  Blinker.attachData(dataRead);
  Button1.attach(button1_callback);
  Button2.attach(button2_callback);
  Slider1.attach(slider1_callback);
  Slider2.attach(slider2_callback);
  Blinker.attachDataStorage(dataStorage, 60, 1);
  timer1.attach(10, screen_change);

  if (shouldSaveConfig)
  {
    int ssid_len = wifiManager.getWiFiSSID().length() + 1;
    int pswd_len = wifiManager.getWiFiPass().length() + 1;
    wifiManager.getWiFiSSID().toCharArray(ssid, ssid_len);
    wifiManager.getWiFiPass().toCharArray(pswd, pswd_len);
    Serial.println(wifiManager.getWiFiSSID());
    Serial.println(wifiManager.getWiFiPass());
    Serial.println(ssid);
    Serial.println(pswd);
    strcpy(auth, blinker_auth.getValue());
    saveConfig();
    ESP.reset();
  }
}

void loop()
{
  if (screen_change_flag)
  {
    readStatus = myAHT20.readRawData(); // read 6 bytes from AHT10 over I2C
    if (readStatus != AHT10_ERROR)
    {
      Serial.print(F("Temperature: "));
      Serial.print(myAHT20.readTemperature(AHT10_USE_READ_DATA));
      Serial.println(F(" +-0.3C"));
      Serial.print(F("Humidity...: "));
      Serial.print(myAHT20.readHumidity(AHT10_USE_READ_DATA));
      Serial.println(F(" +-2%"));
      display.clearDisplay();
      display.setTextSize(1);              // Normal 1:1 pixel scale
      display.setTextColor(SSD1306_WHITE); // Draw white text
      display.setCursor(0, 0);             // Start at top-left corner
      display.println(show);
      display.print(F("Temperature: "));
      display.println(myAHT20.readTemperature(AHT10_USE_READ_DATA));
      display.print(F("Humidity: "));
      display.println(myAHT20.readHumidity(AHT10_USE_READ_DATA));
      display.display();
      humi_read = myAHT20.readHumidity(AHT10_USE_READ_DATA);
      temp_read = myAHT20.readTemperature(AHT10_USE_READ_DATA);
    }
    else
    {
      Serial.print(F("Failed to read - reset: "));
      Serial.println(myAHT20.softReset()); // reset 1-success, 0-failed
    }
    Serial.println(auth);
    Serial.println(ssid);
    Serial.println(pswd);
    screen_change_flag = 0;
  }
  if (WiFi.status() == WL_CONNECTED)
    digitalWrite(LED_BUILTIN, HIGH);
  Blinker.run();
}
