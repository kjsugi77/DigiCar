# DigiCar Bluetooth API リファレンス

## 概要

この文書は、DigiCar（ESP32ベースのロボットカー）をBluetooth経由で制御するためのAPI仕様書です。  
アプリ開発者がDigiCarと通信する際のプロトコル、コマンド形式、実装方法を説明します。

---

## 目次

1. [デバイス情報](#デバイス情報)
2. [接続方法](#接続方法)
3. [通信プロトコル](#通信プロトコル)
4. [コマンド仕様](#コマンド仕様)
5. [応答形式](#応答形式)
6. [実装例](#実装例)
7. [エラーハンドリング](#エラーハンドリング)
8. [注意事項](#注意事項)

---

## デバイス情報

### Bluetoothデバイス名
```
DigiCar_Control
```

### デバイスタイプ
- **プロトコル**: Bluetooth Classic (SPP - Serial Port Profile)
- **ペアリング**: 不要（接続のみ）
- **認証**: なし

### 対応プラットフォーム
- Android (BluetoothAdapter)
- iOS (CoreBluetooth - 注意: iOSはBluetooth Classicの制限あり)
- Windows (Bluetooth Serial)
- Linux (Bluetooth Serial)

---

## 接続方法

### 1. デバイス検索
Bluetoothデバイス一覧から `DigiCar_Control` を検索します。

### 2. 接続
- **ペアリング**: 不要
- **認証**: 不要
- **接続後**: すぐにコマンド送信可能

### 3. 切断
通常のBluetooth切断手順で切断できます。

---

## 通信プロトコル

### 文字エンコーディング
- **形式**: UTF-8
- **改行文字**: `\n` (LF, 0x0A)
- **文字列終端**: 改行文字で終了

### 送信形式
```
<コマンド>\n
```

### 受信形式
```
OK\n
```

### 通信フロー
```
アプリ → DigiCar: <コマンド>\n
DigiCar → アプリ: OK\n
```

---

## コマンド仕様

### 基本形式
すべてのコマンドは改行文字（`\n`）で終了します。

### コマンド一覧

| コマンド | 形式 | 説明 |
|---------|------|------|
| **前進** | `F<speed>` | 指定速度で前進 |
| **後退** | `B<speed>` | 指定速度で後退 |
| **停止** | `S` | モーターを停止 |
| **ステアリング** | `T<angle>` | ステアリング角度を設定 |
| **ジョイスティック** | `J<speed>,<steering>,<direction>` | 速度・ステアリング・方向を同時に設定 |

---

### 1. 前進コマンド

#### 形式
```
F<speed>\n
```

#### パラメータ
- **speed**: 速度（0-100の整数）
  - `0`: 停止
  - `1-100`: 速度パーセンテージ

#### 例
```
F50\n    → 50%速度で前進
F100\n   → 100%速度で前進
F0\n     → 停止（無効、Sコマンドを使用）
```

#### 動作
- 速度が `0` または `100` を超える場合、コマンドは無視されます
- 有効な速度範囲: `1-100`

---

### 2. 後退コマンド

#### 形式
```
B<speed>\n
```

#### パラメータ
- **speed**: 速度（0-100の整数）
  - `0`: 停止
  - `1-100`: 速度パーセンテージ

#### 例
```
B50\n    → 50%速度で後退
B80\n    → 80%速度で後退
```

#### 動作
- 速度が `0` または `100` を超える場合、コマンドは無視されます
- 有効な速度範囲: `1-100`

---

### 3. 停止コマンド

#### 形式
```
S\n
```

#### パラメータ
なし

#### 例
```
S\n      → モーターを停止
```

#### 動作
- モーターA、モーターBの両方を即座に停止します
- PWM信号を0に設定します

---

### 4. ステアリングコマンド

#### 形式
```
T<angle>\n
```

#### パラメータ
- **angle**: ステアリング角度（60-120の整数）
  - `60`: 左端
  - `90`: 中央
  - `120`: 右端

#### 例
```
T90\n    → ステアリングを中央に
T60\n    → ステアリングを左端に
T120\n   → ステアリングを右端に
```

#### 動作
- 角度が `60` 未満の場合、`60` に制限されます
- 角度が `120` を超える場合、`120` に制限されます
- 有効な角度範囲: `60-120`

---

### 5. ジョイスティックコマンド

#### 形式
```
J<speed>,<steering>,<direction>\n
```

#### パラメータ
- **speed**: 速度（0-100の整数）
  - `0-5`: 自動停止
  - `6-100`: 速度パーセンテージ
- **steering**: ステアリング角度（60-120の整数）
  - `60`: 左端
  - `90`: 中央
  - `120`: 右端
- **direction**: 方向（文字列）
  - `"前進"` または `"forward"` または `"F"`: 前進
  - `"後進"` または `"backward"` または `"B"`: 後退
  - その他: デフォルトで前進

#### 例
```
J50,90,前進\n        → 速度50%, ステアリング90度（中央）, 前進
J80,120,backward\n   → 速度80%, ステアリング120度（右端）, 後退
J30,60,F\n          → 速度30%, ステアリング60度（左端）, 前進
J0,90,前進\n        → 停止（speed <= 5）
```

#### 動作
- 速度が `5` 以下の場合、自動的に停止します
- ステアリング角度が範囲外の場合、無視されます
- 方向が認識できない場合、デフォルトで前進します

---

## 応答形式

### 成功応答
すべてのコマンドに対して、成功時は以下の応答が返されます：
```
OK\n
```

### 応答のタイミング
- コマンド受信後、処理完了時に応答を送信します
- 応答は即座に返されます（通常、数ミリ秒以内）

### エラー応答
- 無効なコマンドの場合でも `OK\n` が返されます
- コマンドが無視された場合でも `OK\n` が返されます
- タイムアウトが発生した場合、応答が返らない可能性があります

---

## 実装例

### Android (Java/Kotlin)

#### 接続
```java
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import java.io.OutputStream;
import java.io.InputStream;
import java.util.UUID;

// Bluetooth接続
BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
BluetoothDevice device = adapter.getRemoteDevice("デバイスのMACアドレス");
UUID uuid = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB"); // SPP UUID
BluetoothSocket socket = device.createRfcommSocketToServiceRecord(uuid);
socket.connect();

OutputStream outputStream = socket.getOutputStream();
InputStream inputStream = socket.getInputStream();
```

#### コマンド送信
```java
// 前進コマンド送信
String command = "F50\n";
outputStream.write(command.getBytes("UTF-8"));

// 応答受信
byte[] buffer = new byte[1024];
int bytes = inputStream.read(buffer);
String response = new String(buffer, 0, bytes, "UTF-8");
// response = "OK\n"
```

#### ジョイスティックコマンド送信
```java
// ジョイスティックコマンド送信
int speed = 50;
int steering = 90;
String direction = "前進";
String command = String.format("J%d,%d,%s\n", speed, steering, direction);
outputStream.write(command.getBytes("UTF-8"));
```

---

### iOS (Swift)

#### 注意事項
iOSはBluetooth Classicの制限があるため、`ExternalAccessory` フレームワークを使用する必要があります。  
または、BLE（Bluetooth Low Energy）への移行を検討してください。

#### 接続（CoreBluetooth使用 - BLE推奨）
```swift
import CoreBluetooth

// BLEへの移行を推奨
// または ExternalAccessory フレームワークを使用
```

---

### Python (Windows/Linux)

#### 接続
```python
import bluetooth

# デバイス検索
devices = bluetooth.discover_devices()
target_device = None
for addr in devices:
    name = bluetooth.lookup_name(addr)
    if name == "DigiCar_Control":
        target_device = addr
        break

# 接続
socket = bluetooth.BluetoothSocket(bluetooth.RFCOMM)
socket.connect((target_device, 1))
```

#### コマンド送信
```python
# 前進コマンド送信
command = "F50\n"
socket.send(command.encode('utf-8'))

# 応答受信
response = socket.recv(1024).decode('utf-8')
# response = "OK\n"
```

#### ジョイスティックコマンド送信
```python
# ジョイスティックコマンド送信
speed = 50
steering = 90
direction = "前進"
command = f"J{speed},{steering},{direction}\n"
socket.send(command.encode('utf-8'))
```

---

### JavaScript (Web Bluetooth API)

#### 注意事項
Web Bluetooth APIは一部のブラウザ（Chrome、Edge）でのみサポートされています。

#### 接続
```javascript
// Web Bluetooth APIを使用
navigator.bluetooth.requestDevice({
    filters: [{ name: 'DigiCar_Control' }],
    optionalServices: ['00001101-0000-1000-8000-00805F9B34FB']
})
.then(device => device.gatt.connect())
.then(server => server.getPrimaryService('00001101-0000-1000-8000-00805F9B34FB'))
.then(service => service.getCharacteristic('...'))
.then(characteristic => {
    // コマンド送信
    const command = "F50\n";
    const encoder = new TextEncoder();
    characteristic.writeValue(encoder.encode(command));
});
```

---

## エラーハンドリング

### タイムアウト
- コマンド送信後、5秒以内に応答がない場合、タイムアウトとみなします
- タイムアウト時は、再接続を試みることを推奨します

### 無効なコマンド
- 無効なコマンド形式の場合、コマンドは無視されますが、`OK\n` が返されます
- アプリ側でコマンド形式を検証することを推奨します

### 接続エラー
- 接続が切断された場合、自動再接続機能を実装することを推奨します
- 定期的に接続状態を確認してください

### パラメータ範囲外
- 速度が `0` または `100` を超える場合、コマンドは無視されます
- ステアリング角度が `60-120` の範囲外の場合、範囲内に制限されます
- アプリ側でパラメータを検証することを推奨します

---

## 注意事項

### 1. コマンド送信頻度
- コマンド送信頻度は **10Hz（100ms間隔）** 以下を推奨します
- 過度な送信は、ESP32の処理負荷を増加させる可能性があります

### 2. 文字エンコーディング
- 必ず **UTF-8** エンコーディングを使用してください
- 日本語文字列（"前進"、"後進"）を使用する場合は、UTF-8でエンコードしてください

### 3. 改行文字
- コマンドの終端には必ず **改行文字（`\n`）** を含めてください
- 改行文字がない場合、コマンドは認識されません

### 4. 同時コマンド
- 複数のコマンドを同時に送信しないでください
- 1つのコマンドを送信し、応答を待ってから次のコマンドを送信してください

### 5. ステアリング制御
- ステアリングは非ブロッキング方式で制御されています
- ステアリングコマンドを送信しても、即座に角度が変わるわけではありません
- サーボモーターの応答時間を考慮してください（通常、数百ミリ秒）

### 6. モーター制御
- モーターの速度はPWM信号で制御されています
- 速度変更は即座に反映されます
- 停止コマンド（`S`）を送信すると、モーターは即座に停止します

### 7. iOSの制限
- iOSはBluetooth Classicの制限があるため、`ExternalAccessory` フレームワークを使用する必要があります
- または、BLE（Bluetooth Low Energy）への移行を検討してください

### 8. デバッグ
- デバッグ時は、シリアルモニターでコマンドの受信を確認できます
- ESP32のシリアル出力（115200 baud）でコマンドの処理状況を確認できます

---

## コマンド一覧表（クイックリファレンス）

| コマンド | 形式 | パラメータ | 例 |
|---------|------|-----------|-----|
| **前進** | `F<speed>` | speed: 1-100 | `F50\n` |
| **後退** | `B<speed>` | speed: 1-100 | `B50\n` |
| **停止** | `S` | なし | `S\n` |
| **ステアリング** | `T<angle>` | angle: 60-120 | `T90\n` |
| **ジョイスティック** | `J<speed>,<steering>,<direction>` | speed: 0-100, steering: 60-120, direction: "前進"/"後進" | `J50,90,前進\n` |

---

## バージョン情報

- **API Version**: 1.0
- **最終更新**: 2025年12月
- **対応ファームウェア**: 20251206 DigiCar.ino

---

## サポート

問題や質問がある場合は、プロジェクトのリポジトリにIssueを登録してください。

---

## ライセンス

このAPI仕様書は、DigiCarプロジェクトの一部です。


