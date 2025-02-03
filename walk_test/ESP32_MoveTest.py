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
        response = requests.post(url, data=data, headers=headers)
        print(f"送信データ: {data} - Status code: {response.status_code}")
        print(f"Response: {response.text}")
    except requests.exceptions.RequestException as e:
        print(f"Error: {e}")

# 1秒間隔でデータを交互に送信
data_sequence = ["000000", "010000"]
index = 0

while True:
    # 現在のデータを取得して送信
    current_data = data_sequence[index]
    send_data(current_data)

    # インデックスを切り替えて次のデータを準備
    index = 1 - index  # 0 と 1 を交互に切り替え

    # 1秒間の待機
    time.sleep(1)
