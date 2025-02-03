import time
import requests

# ESP32のIPアドレスとポート番号
ESP32_IP = "192.168.1.164"
PORT = 80

# ESP32にデータを送信する関数
def send_data(data):
    url = f"http://{ESP32_IP}:{PORT}"
    headers = {'Content-Type': 'text/plain'}
    try:
        # 送信前の時刻を記録
        send_time = time.time() * 1000  # ミリ秒に変換

        # POSTリクエストを送信
        response = requests.post(url, data=data, headers=headers)

        # 応答を解析
        if response.status_code == 200:
            response_text = response.text
            print(f"Response: {response_text}")

            # ESP32が返したタイムスタンプを取得
            esp_receive_time = int(response_text.split("Time: ")[1].split(" ms")[0])

            # 現在時刻を取得
            receive_time = time.time() * 1000  # ミリ秒に変換

            # 送信時間と往復時間を計算
            send_duration = esp_receive_time - send_time
            round_trip_time = receive_time - send_time

            print(f"送信時間 (推定): {send_duration:.2f} ms")
            print(f"往復時間: {round_trip_time:.2f} ms")
        else:
            print(f"Error: Status code {response.status_code}")
    except requests.exceptions.RequestException as e:
        print(f"Error: {e}")

# データ送信ループ
data_sequence = ["010000", "000000"]
index = 0

while True:
    current_data = data_sequence[index]
    send_data(current_data)
    index = 1 - index
    time.sleep(1)
