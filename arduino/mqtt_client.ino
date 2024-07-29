#include <DHTesp.h>

DHTesp dht;

int dhtPin = 4;
ComfortState cf;

#include <WiFi.h>

const char * ssid = "510";
const char * pass = "anabuki-fukuyama";

#include "time.h"

// NTPサーバ設定
const char* ntpServer1 = "ntp.nict.jp";
const char* ntpServer2 = "time.google.com";
const char* ntpServer3 = "ntp.jst.mfeed.ad.jp";
const long  gmtOffset_sec = 9 * 3600;
const int   daylightOffset_sec = 0;

#include <MQTT.h>

const char* mqtt_server = "mqtt.inlet.wjg.jp";

// MQTTトピックス
#define TOPIX "acf/env/"
// 端末ID
#define TERMINAL_ID "arduino_000"
// 端末位置 [ID] → ユニークなコードに置き換える
#define TERMINAL_LOCATION "[ID]"

WiFiClient net;
MQTTClient client;


unsigned long lastMillis = 0;
// 更新間隔 5秒 5*1000
const unsigned long interval = 5000;

// 現地時刻を返す
// 
char * getLocalTime()
{
  static char buffer[22];
  time_t t;
  struct tm *tm;

  t = time(NULL);
  tm = localtime(&t);

  if(!tm) {
    Serial.println("Failed to obtain time");
    return "----";
  }

  sprintf(buffer,"%04d/%02d/%02d %02d:%02d:%02d",
        tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
        tm->tm_hour, tm->tm_min, tm->tm_sec);

  return buffer;
}


// MQTTサーバへ接続する
void connect() {
  Serial.print("\nconnecting...");
  while (!client.connect(TERMINAL_ID)) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nconnected!");

  // 交配するトピックスを登録する
  client.subscribe(TOPIX "#");
}

/**
 * 定期購買したトピックスを受信する
 *  引数
 *    String& topic   ：受信したトピックス
 *    String& payload ：受信したメッセージを保存する入れ物
 */
void messageReceived(String& topic, String& payload) {
  Serial.println("incoming: " + topic + " - " + payload);
}


/**
 * DHT11からのデータを読み取りJSON形式で返す
 * 
 */
char* getTemperature() {
  static char buffer[160];
  
  TempAndHumidity newValues = dht.getTempAndHumidity();
  
  if (dht.getStatus() != 0) {
    sprintf(buffer, "{\"E\":\"DHT11 error status: %s\"", dht.getStatusString());
    return buffer;
  }

  float heatIndex = dht.computeHeatIndex(newValues.temperature, newValues.humidity);
  float dewPoint = dht.computeDewPoint(newValues.temperature, newValues.humidity);
  float cr = dht.getComfortRatio(cf, newValues.temperature, newValues.humidity);

  char* comfortStatus;
  switch (cf) {
    case Comfort_OK: 
      comfortStatus = "Comfort_OK"; 
      break;
    case Comfort_TooHot: 
      comfortStatus = "Comfort_TooHot"; 
      break;
    case Comfort_TooCold: 
      comfortStatus = "Comfort_TooCold"; 
      break;
    case Comfort_TooDry: 
      comfortStatus = "Comfort_TooDry"; 
      break;
    case Comfort_TooHumid: 
      comfortStatus = "Comfort_TooHumid"; 
      break;
    case Comfort_HotAndHumid: 
      comfortStatus = "Comfort_HotAndHumid"; 
      break;
    case Comfort_HotAndDry: 
      comfortStatus = "Comfort_HotAndDry"; 
      break;
    case Comfort_ColdAndHumid: 
      comfortStatus = "Comfort_ColdAndHumid"; 
      break;
    case Comfort_ColdAndDry: 
      comfortStatus = "Comfort_ColdAndDry"; 
      break;
    default: 
      comfortStatus = "Unknown:"; 
      break;
  };

  sprintf(buffer, "{\"ID\":\"%s\",\"TS\":\"%s\",\"T\":%5.2f,\"H\":%5.2f,\"I\":%5.2f,\"D\":%5.2f,\"S\":\"%s\"}", 
          TERMINAL_ID,
          getLocalTime(),
          newValues.temperature, newValues.humidity, heatIndex, dewPoint, comfortStatus);

  return buffer;
}

void setup() {
  Serial.begin(115200);

  // WiFi接続
  WiFi.begin(ssid, pass);

  Serial.print("connecting to :");
  Serial.print(ssid);
  Serial.print(":");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("connected!");

  // NTPサーバへ同期
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2, ntpServer3);

  // DHT11へ接続
  dht.setup(dhtPin, DHTesp::DHT11);
  Serial.println("DHT initiated");

  // MQTTサーバ設定
  //  サーバ設定
  client.begin(mqtt_server, net);
  //  コールバック関数を登録する
  client.onMessage(messageReceived);

  // MQTTサーバへ接続
  connect();
}

void loop() {
  // 購買データをチェックする
  client.loop();
  delay(10);

  if (!client.connected()) {
    // 再接続
    connect(); 
  }

  // 定時配信処理 interval 時間間隔で配信する
  if (millis() - lastMillis > interval) {
    lastMillis = millis();
    // 温度・湿度を配信する
    client.publish(TOPIX TERMINAL_LOCATION, getTemperature());
  }
}
