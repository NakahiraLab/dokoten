#include <ESP32Servo.h>  // ESP32用のサーボライブラリ
#include <WiFi.h>        // WiFi通信ライブラリ

#define SERVO_PIN_1 5  // 回転用サーボモータのピン
#define SERVO_PIN_2 18 // 昇降用サーボモータのピン

// 中平研のwifiルータ
const char* ssid = "TP-Link_4B00";
const char* password = "31207560";

WiFiServer server(80); // Webサーバをポート80で起動
Servo myServo1;        // 回転用サーボモータ
Servo myServo2;        // 昇降用サーボモータ

QueueHandle_t rotateQueue;  // 回転角度を格納するキュー
QueueHandle_t liftQueue;    // 昇降フラグを格納するキュー

void setup() {
  Serial.begin(115200);  // シリアル通信開始

  // サーボモータの初期設定
  myServo1.attach(SERVO_PIN_1); // 回転用サーボを指定ピンに接続
  myServo2.attach(SERVO_PIN_2); // 昇降用サーボを指定ピンに接続
  myServo1.write(0);   // 初期角度（回転モータ）
  myServo2.write(30);  // 初期位置（下降）

  // Wi-Fi接続開始
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { // Wi-Fi接続待機
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi"); // Wi-Fi接続成功
  server.begin();  // Webサーバ開始
  Serial.println("Server started");

  // FreeRTOSキューの作成（キューサイズ10）
  rotateQueue = xQueueCreate(10, sizeof(int)); // 回転角度キュー
  liftQueue = xQueueCreate(10, sizeof(int));   // 昇降フラグキュー

  // タスクの作成（マルチタスク処理）
  xTaskCreatePinnedToCore(wifiTask, "WiFi Task", 4096, NULL, 1, NULL, 0);  // Wi-Fi通信処理タスク（コア0）
  xTaskCreatePinnedToCore(rotateTask, "Rotate Task", 2048, NULL, 2, NULL, 1); // 回転処理タスク（コア1）
  xTaskCreatePinnedToCore(liftTask, "Lift Task", 2048, NULL, 2, NULL, 1); // 昇降処理タスク（コア1）
}

void loop() {}

// Wi-Fiタスク: クライアントからデータを受信
void wifiTask(void* param) {
  while (true) {
    WiFiClient client = server.available(); // クライアントからの接続待機
    if (client) {
      Serial.println("Client connected");

      // クライアントからデータを受信
      String data = client.readStringUntil('\r');
      client.flush();

      Serial.print("Received data: ");
      Serial.println(data);

      if (data.length() == 6) { // データ長6のとき処理（2桁の昇降フラグ + 4桁の角度）
        String liftFlagStr = data.substring(0, 2);  // 昇降フラグ（00 or 01）
        String rotateAngleStr = data.substring(2, 6); // 回転角度（0000〜0180）

        int liftFlag = liftFlagStr.toInt();  // 昇降フラグ（整数型に変換）
        int rotateAngle = rotateAngleStr.toInt(); // 回転角度（整数型に変換）

        // キューにデータを送信（回転角度・昇降フラグ）
        xQueueSend(liftQueue, &liftFlag, portMAX_DELAY);
        xQueueSend(rotateQueue, &rotateAngle, portMAX_DELAY);

        // クライアントへレスポンス送信
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        client.println("Data received");
      } else {
        // 不正なデータ長のときエラーを送信
        client.println("Invalid data");
      }

      client.stop(); // クライアント切断
      Serial.println("Client disconnected");
    }
    delay(10);
  }
}

// 回転タスク: Motor 1 を制御（回転）
void rotateTask(void* param) {
  int rotateAngle = 0; // 角度変数

  while (true) {
    // キューから回転角度を受信
    if (xQueueReceive(rotateQueue, &rotateAngle, portMAX_DELAY) == pdTRUE) {
      if (rotateAngle >= 0 && rotateAngle <= 180) { // 角度が有効範囲内かチェック
        myServo1.write(rotateAngle); // サーボの角度を設定
        Serial.print("Motor 1 (rotate) set to ");
        Serial.print(rotateAngle);
        Serial.println(" degrees");
      } else {
        Serial.println("Invalid angle for Motor 1"); // 無効な角度のとき
      }
    }
    delay(10);
  }
}

// 昇降タスク: Motor 2 を制御（昇降）
void liftTask(void* param) {
  int liftFlag = 0; // 昇降フラグ変数

  while (true) {
    // キューから昇降フラグを受信
    if (xQueueReceive(liftQueue, &liftFlag, portMAX_DELAY) == pdTRUE) {
      if (liftFlag == 1) { // 1なら上昇
        myServo2.write(10);  // 上昇位置（10度）
        Serial.println("Motor 2 (lift) set to 10 degrees (up)");
      } else if (liftFlag == 0) { // 0なら下降
        myServo2.write(30);  // 下降位置（30度）
        Serial.println("Motor 2 (lift) set to 30 degrees (down)");
      } else {
        Serial.println("Invalid lift flag"); // 無効なフラグのとき
      }
    }
    delay(10);
  }
}
