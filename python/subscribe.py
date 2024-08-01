# MQTTライブラリの読み込み
# pip install paho.mqtt で予めライブラリをインストールしておく
import random
from paho.mqtt import client as mqtt_client

# MQTT設定
#   購買するトピックス
toipx = 'acf/env/#'
#   Brokerのホスト名(IPアドレス)
server = 'mqtt.inlet.wjg.jp'
server = '192.168.2.251'
port = 1883
#   ランダムなClient ID
client_id = f'python-mqtt-{random.randint(0, 1000):04}'

# 接続完了時のイベント処理
def on_connect(client,userdata,flags,rc):
    print('Connected with result code' + str(rc))
    # rc = 0 正常に接続出来た
    # サブスクライバーは接続後に定期購買のトピックを申請する
    client.subscribe(toipx)

# メッセージが配信された時の受信処理
def on_message(client,userdata,message):
    print(message.topic + ' ' + str(message.payload))

# このファイルを実行しているかチェック
if __name__ == "__main__": 
    client = mqtt_client.Client(client_id=client_id)
    client.on_connect = on_connect
    client.on_message = on_message

    client.connect(server, port)

    client.loop_forever()
