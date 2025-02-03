#include <ESP32Servo.h>  // ESP32用のサーボライブラリ
#include <WiFi.h>        // WiFi通信ライブラリ

#define SERVO_PIN_1 5    // サーボモータ1の接続ピン
#define SERVO_PIN_2 18   // サーボモータ2の接続ピン

// 中平研のwifiルータ
const char* ssid = "TP-Link_4B00";
const char* password = "31207560";

WiFiServer server(80);   // ポート80でWebサーバーを起動
Servo myServo1;          // サーボモータ1のオブジェクト
Servo myServo2;          // サーボモータ2のオブジェクト

int currentLiftState = 0;      // 昇降状態の管理（0: 降, 1: 昇）
int currentRotateAngle = 0;    // 回転角度の管理

void setup() {
  Serial.begin(115200);  // シリアル通信の開始
  myServo1.attach(SERVO_PIN_1);  // サーボモータ1を指定のピンに接続
  myServo2.attach(SERVO_PIN_2);  // サーボモータ2を指定のピンに接続

  // Wi-Fiに接続
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");  // 接続待機中の表示
  }
  Serial.println("Connected to Wi-Fi");  // Wi-Fi接続成功の表示

  server.begin();  // Webサーバーを起動
  Serial.println("Server started");

  // 初期状態のサーボモーターの位置を設定
  myServo2.write(30);  // モータ2を30度（下げた状態）に設定
  myServo1.write(0);   // モータ1を0度に設定
  Serial.println("Initial state: Motor 2 (lower), Motor 1 (rotate) 0 degrees");
}

void loop() {
  WiFiClient client = server.available();  // クライアントからの接続を待機
  if (client) {
    Serial.println("Client connected");  // クライアント接続の確認

    // HTTPリクエストのヘッダーを読み取る
    while (client.available()) {
      String header = client.readStringUntil('\n');
      if (header == "\r") {  // ヘッダーの終わり
        break;
      }
      Serial.println("Header: " + header);  // ヘッダーを表示
    }

    // クライアントからのデータ（コマンド）を取得
    String data = client.readStringUntil('\r');
    client.flush();  // バッファをクリア
    Serial.print("Received data: ");
    Serial.println(data);

    // データの長さが6であることを確認
    if (data.length() == 6) {
      String liftFlag = data.substring(0, 2);  // 最初の2文字（昇降フラグ）
      String rotateAngleStr = data.substring(2, 6);  // 残りの4文字（回転角度）

      int rotateAngle = rotateAngleStr.toInt();  // 文字列を整数に変換

      Serial.print("Lift flag: ");
      Serial.println(liftFlag);
      Serial.print("Rotate angle: ");
      Serial.println(rotateAngle);

      // 持ち上げ動作（01）で、現在持ち上げていない場合
      if (liftFlag == "01" && currentLiftState == 0) {
        myServo1.write(rotateAngle);  // 回転モーターの角度を変更
        currentRotateAngle = rotateAngle;
        Serial.print("Motor 1 (rotate) set to ");
        Serial.print(rotateAngle);
        Serial.println(" degrees");

        delay(500);  // 0.5秒待機

        myServo2.write(10);  // リフトアップ
        currentLiftState = 1;
        Serial.println("Motor 2 (lift) set to 10 degrees");

      // 降ろす動作（00）で、現在持ち上げている場合
      } else if (liftFlag == "00" && currentLiftState == 1) {
        myServo2.write(30);  // リフトダウン
        currentLiftState = 0;
        Serial.println("Motor 2 (lower) set to 30 degrees");

        delay(500);  // 0.5秒待機

        myServo1.write(rotateAngle);  // 回転モーターの角度を変更
        currentRotateAngle = rotateAngle;
        Serial.print("Motor 1 (rotate) set to ");
        Serial.print(rotateAngle);
        Serial.println(" degrees");

      // すでに希望のリフト状態なら回転のみを処理
      } else {
        Serial.println("Lift already in desired state, processing rotate only.");

        if (rotateAngle >= 0 && rotateAngle <= 180) {  // 角度が有効な範囲内かチェック
          myServo1.write(rotateAngle);  // 回転モーターの角度を変更
          currentRotateAngle = rotateAngle;
          Serial.print("Motor 1 (rotate) set to ");
          Serial.print(rotateAngle);
          Serial.println(" degrees");
        } else {
          Serial.println("Invalid angle for Motor 1");
        }
      }
    } else {
      Serial.println("Invalid command length");  // データ長が正しくない場合のエラーメッセージ
    }

    // クライアントへHTTPレスポンスを送信
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.println("Data received");

    client.stop();  // クライアントとの接続を終了
    Serial.println("Client disconnected");
  }
}
