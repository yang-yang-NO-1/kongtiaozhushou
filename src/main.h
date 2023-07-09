#include <WiFiManager.h>
#include "Ticker.h"
#include <LittleFS.h>
#include <IRsend.h>
#include <ir_Gree.h>
#include "..\lib\blinker-library-master\src\modules\ArduinoJson\ArduinoJson.h"

extern IRGreeAC ac;
bool screen_change_flag, dht_flag; // 屏幕刷新标志,dht刷新标志
bool screen_power_flag = 1;        // 屏幕开启标志
bool shouldSaveConfig = false;
char auth[20] = "";
char ssid[20] = "";
char pswd[20] = "";

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

void screen_change()
{
  screen_change_flag = 1;
}

void dht_flag_change()
{
  dht_flag = 1;
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