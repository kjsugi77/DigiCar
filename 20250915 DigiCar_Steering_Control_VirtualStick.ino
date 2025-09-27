#include <WiFi.h>
#include <WebServer.h>
#include <esp_wifi.h>

// 正しいモーター制御ピン
#define MOTOR_A_IN1  26  // GPIO26
#define MOTOR_A_IN2  25  // GPIO25
#define MOTOR_A_PWM  33  // GPIO33
#define MOTOR_B_IN1  27  // GPIO27
#define MOTOR_B_IN2  14  // GPIO14
#define MOTOR_B_PWM  12  // GPIO12

// サーボモーター制御ピン（ステアリング）
#define SERVO_PIN    32  // GPIO32（サーボ信号線）

// WiFi設定
const char* ssid = "DigiCar_Control";  // WiFiのSSID
const char* password = "digimoku";     // WiFiのパスワード

WebServer server(80);

void setup() {
  Serial.begin(115200);
  Serial.println("=== DigiCar Steering Control (Simple) ===");
  
  // CPU周波数を下げて発熱を軽減
  setCpuFrequencyMhz(80); // 240MHz → 80MHz
  Serial.print("CPU Frequency: ");
  Serial.print(getCpuFrequencyMhz());
  Serial.println(" MHz");
  
  // モーター制御ピンの設定
  pinMode(MOTOR_A_IN1, OUTPUT);
  pinMode(MOTOR_A_IN2, OUTPUT);
  pinMode(MOTOR_A_PWM, OUTPUT);
  pinMode(MOTOR_B_IN1, OUTPUT);
  pinMode(MOTOR_B_IN2, OUTPUT);
  pinMode(MOTOR_B_PWM, OUTPUT);
  
  // サーボピンの設定
  pinMode(SERVO_PIN, OUTPUT);
  
  // 初期状態：停止
  stopMotors();
  setServoAngle(90); // サーボを中央位置に
  Serial.println("Motors and Steering initialized");
  
  // WiFi APモードで起動（省電力設定）
  WiFi.softAP(ssid, password);
  
  // WiFi省電力設定
  WiFi.setSleep(false); // WiFiスリープを無効化（安定性優先）
  
  // 省電力設定（安全な方法）
  #ifdef ESP32
    esp_wifi_set_ps(WIFI_PS_NONE); // WiFi省電力モードを無効化
  #endif
  
  Serial.println("WiFi AP started");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(password);
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());
  
  // Webサーバーの設定
  server.on("/", handleRoot);
  server.on("/forward", handleForward);
  server.on("/backward", handleBackward);
  server.on("/stop", handleStop);
  server.on("/steering", handleSteering);
  server.on("/joystick", handleJoystick);
  
  server.begin();
  Serial.println("Web server started");
  Serial.println("Connect to WiFi 'DigiCar_Control' and visit http://192.168.4.1");
}

void loop() {
  server.handleClient();
  delay(10); // 適切な遅延でCPU負荷を軽減
}

// サーボ角度設定関数（ステアリング）- 最適化版
void setServoAngle(int angle) {
  // 角度をパルス幅に変換（0度=500us, 180度=2500us）
  int pulseWidth = map(angle, 0, 180, 500, 2500);
  
  // サーボパルスを送信（1回のみ）
  digitalWrite(SERVO_PIN, HIGH);
  delayMicroseconds(pulseWidth);
  digitalWrite(SERVO_PIN, LOW);
  
  // シリアル出力を削減（デバッグ時のみ有効）
  // Serial.print("S: ");
  // Serial.println(angle);
}

// WebページのHTML
void handleRoot() {
  String html = R"html(
<!DOCTYPE html>
<html lang="ja">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>DigiCar Steering Control</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            text-align: center;
            background-color: #f0f0f0;
            margin: 0;
            padding: 20px;
        }
        .container {
            max-width: 400px;
            margin: 0 auto;
            background: white;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }
        h1 {
            color: #333;
            margin-bottom: 30px;
        }
        .control-grid {
            display: flex;
            flex-direction: column;
            gap: 10px;
            margin-bottom: 30px;
            align-items: center;
        }
        .control-btn {
            padding: 25px;
            font-size: 20px;
            border: none;
            border-radius: 10px;
            cursor: pointer;
            transition: all 0.3s;
            font-weight: bold;
        }
        .forward { 
            background-color: #4CAF50; 
            color: white; 
            width: 80%;
        }
        .backward { 
            background-color: #f44336; 
            color: white; 
            width: 80%;
        }
        .stop { 
            background-color: #9E9E9E; 
            color: white; 
            width: 100%; 
        }
        
        .control-btn:hover {
            transform: scale(1.05);
            box-shadow: 0 4px 8px rgba(0,0,0,0.2);
        }
        .control-btn:active {
            transform: scale(0.95);
        }
        
        .steering-section {
            margin-bottom: 20px;
        }
        .steering-label {
            font-size: 18px;
            font-weight: bold;
            margin-bottom: 10px;
            color: #333;
        }
        .steering-slider {
            width: 100%;
            height: 40px;
            -webkit-appearance: none;
            appearance: none;
            background: #ddd;
            outline: none;
            border-radius: 20px;
            margin-bottom: 10px;
        }
        .steering-slider::-webkit-slider-thumb {
            -webkit-appearance: none;
            appearance: none;
            width: 50px;
            height: 50px;
            background: #2196F3;
            cursor: pointer;
            border-radius: 50%;
            border: 3px solid white;
            box-shadow: 0 2px 5px rgba(0,0,0,0.2);
        }
        .steering-slider::-moz-range-thumb {
            width: 50px;
            height: 50px;
            background: #2196F3;
            cursor: pointer;
            border-radius: 50%;
            border: 3px solid white;
            box-shadow: 0 2px 5px rgba(0,0,0,0.2);
        }
        .steering-value {
            font-size: 16px;
            color: #666;
            margin-bottom: 20px;
        }
        
        .status {
            margin-top: 20px;
            padding: 10px;
            background-color: #e8f5e8;
            border-radius: 5px;
            color: #2e7d32;
        }
        
        .joystick-section {
            margin-top: 20px;
        }
        
        .joystick-container {
            position: relative;
            width: 150px;
            height: 150px;
            margin: 0 auto 15px;
            background: radial-gradient(circle, #e0e0e0 0%, #c0c0c0 100%);
            border-radius: 50%;
            border: 2px solid #999;
            box-shadow: inset 0 2px 10px rgba(0,0,0,0.1);
            cursor: pointer;
        }
        
        .joystick-knob {
            position: absolute;
            width: 40px;
            height: 40px;
            background: linear-gradient(145deg, #4CAF50, #45a049);
            border-radius: 50%;
            border: 2px solid #fff;
            box-shadow: 0 2px 5px rgba(0,0,0,0.3);
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            transition: all 0.1s ease;
        }
        
        .joystick-info {
            display: flex;
            justify-content: space-around;
            font-size: 14px;
            color: #666;
        }
        
        .speed-control {
            margin-top: 15px;
        }
        
        .speed-label {
            font-size: 14px;
            font-weight: bold;
            margin-bottom: 8px;
            color: #333;
        }
        
        .speed-slider {
            width: 100%;
            height: 30px;
            -webkit-appearance: none;
            appearance: none;
            background: #ddd;
            outline: none;
            border-radius: 15px;
            margin-bottom: 8px;
        }
        
        .speed-slider::-webkit-slider-thumb {
            -webkit-appearance: none;
            appearance: none;
            width: 30px;
            height: 30px;
            background: #FF9800;
            cursor: pointer;
            border-radius: 50%;
            border: 2px solid white;
            box-shadow: 0 2px 5px rgba(0,0,0,0.2);
        }
        
        .speed-slider::-moz-range-thumb {
            width: 30px;
            height: 30px;
            background: #FF9800;
            cursor: pointer;
            border-radius: 50%;
            border: 2px solid white;
            box-shadow: 0 2px 5px rgba(0,0,0,0.2);
        }
        
        .speed-value {
            font-size: 14px;
            color: #666;
            text-align: center;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🚗 DigiCar</h1>
        
        <div class="joystick-section">
            <div class="steering-label">バーチャルスティック</div>
            <div class="joystick-container" id="joystickContainer">
                <div class="joystick-knob" id="joystickKnob"></div>
            </div>
            <div class="joystick-info">
                <div>速度: <span id="joystickSpeed">0%</span></div>
                <div>方向: <span id="joystickDirection">停止</span></div>
            </div>
            <div class="speed-control">
                <div class="speed-label">速度調整</div>
                <input type="range" min="10" max="100" value="50" class="speed-slider" id="speedSlider" oninput="updateMaxSpeed(this.value)">
                <div class="speed-value">最大速度: <span id="maxSpeedValue">50%</span></div>
            </div>
        </div>
        
        <div class="control-grid">
            <button class="control-btn forward" onclick="sendCommand('forward')">↑<br>前進</button>
            <button class="control-btn stop" onclick="sendCommand('stop')">■<br>停止</button>
            <button class="control-btn backward" onclick="sendCommand('backward')">↓<br>後進</button>
        </div>
        
        <div class="steering-section">
            <div class="steering-label">ステアリング制御</div>
            <input type="range" min="60" max="120" value="90" class="steering-slider" id="steeringSlider" oninput="updateSteering(this.value)">
            <div class="steering-value" id="steeringValue">中央 (90度)</div>
        </div>

    </div>

    <script>
        // バーチャルスティック用の変数
        let isDragging = false;
        let joystickKnob = document.getElementById('joystickKnob');
        let joystickContainer = document.getElementById('joystickContainer');
        let joystickSpeed = document.getElementById('joystickSpeed');
        let joystickDirection = document.getElementById('joystickDirection');
        let maxSpeedValue = document.getElementById('maxSpeedValue');
        let speedSlider = document.getElementById('speedSlider');
        
        const centerX = 75; // ジョイスティックの中心X座標
        const centerY = 75; // ジョイスティックの中心Y座標
        const maxDistance = 50; // 最大移動距離
        let maxSpeed = 50; // 最大速度（%）
        
        // バーチャルスティックのイベントリスナー
        joystickContainer.addEventListener('mousedown', handleJoystickStart);
        joystickContainer.addEventListener('touchstart', handleJoystickStart, {passive: false});
        document.addEventListener('mousemove', handleJoystickMove);
        document.addEventListener('touchmove', handleJoystickMove, {passive: false});
        document.addEventListener('mouseup', handleJoystickEnd);
        document.addEventListener('touchend', handleJoystickEnd, {passive: false});
        
        function sendCommand(command) {
            fetch('/' + command)
                .then(response => {
                    // コマンド送信成功
                })
                .catch(error => {
                    // エラー処理
                });
        }
        
        function updateSteering(value) {
            const angle = parseInt(value);
            let direction = '';
            
            if (angle < 75) direction = '左';
            else if (angle > 105) direction = '右';
            else direction = '中央';
            
            document.getElementById('steeringValue').textContent = direction + ' (' + angle + '度)';
            
            // 即座に送信（反応を良くする）
            fetch('/steering?angle=' + angle)
                .then(response => {
                    // ステアリング設定成功
                })
                .catch(error => {
                    // エラー処理
                });
        }
        
        // バーチャルスティックの関数
        function handleJoystickStart(e) {
            e.preventDefault();
            isDragging = true;
            joystickKnob.style.transition = 'none';
        }
        
        function handleJoystickMove(e) {
            if (!isDragging) return;
            e.preventDefault();
            
            let clientX, clientY;
            
            if (e.touches && e.touches.length > 0) {
                clientX = e.touches[0].clientX;
                clientY = e.touches[0].clientY;
            } else {
                clientX = e.clientX;
                clientY = e.clientY;
            }
            
            const rect = joystickContainer.getBoundingClientRect();
            const x = clientX - rect.left - centerX;
            const y = clientY - rect.top - centerY;
            
            const distance = Math.sqrt(x * x + y * y);
            const angle = Math.atan2(y, x);
            
            let finalX, finalY;
            if (distance > maxDistance) {
                finalX = Math.cos(angle) * maxDistance;
                finalY = Math.sin(angle) * maxDistance;
            } else {
                finalX = x;
                finalY = y;
            }
            
            // スティックの位置を更新
            joystickKnob.style.left = (centerX + finalX - 20) + 'px';
            joystickKnob.style.top = (centerY + finalY - 20) + 'px';
            
            // 車の制御を更新
            updateJoystickControl(finalX, finalY);
        }
        
        function handleJoystickEnd(e) {
            e.preventDefault();
            isDragging = false;
            joystickKnob.style.transition = 'all 0.3s ease';
            joystickKnob.style.left = '50%';
            joystickKnob.style.top = '50%';
            joystickKnob.style.transform = 'translate(-50%, -50%)';
            
            // 停止
            joystickSpeed.textContent = '0%';
            joystickDirection.textContent = '停止';
            sendCommand('stop');
        }
        
        function updateMaxSpeed(value) {
            maxSpeed = parseInt(value);
            maxSpeedValue.textContent = maxSpeed + '%';
        }
        
        // レスポンス改善用の変数
        let lastCommand = '';
        let commandTimeout = null;
        
        function updateJoystickControl(x, y) {
            const distance = Math.sqrt(x * x + y * y);
            const speed = Math.min(Math.round((distance / maxDistance) * maxSpeed), maxSpeed);
            
            // 速度表示更新
            joystickSpeed.textContent = speed + '%';
            
            // 方向判定
            let direction = '停止';
            if (speed > 5) {
                if (y < -10) direction = '前進';
                else if (y > 10) direction = '後進';
                else if (x < -10) direction = '左旋回';
                else if (x > 10) direction = '右旋回';
            }
            joystickDirection.textContent = direction;
            
            // サーバーにコマンド送信（重複を避ける）
            const steeringAngle = Math.round(90 + (x / maxDistance) * 30); // 60-120度の範囲
            const newCommand = speed + ',' + steeringAngle + ',' + direction;
            
            // 前回と同じコマンドの場合は送信しない
            if (newCommand !== lastCommand) {
                lastCommand = newCommand;
                
                // 前回のタイムアウトをクリア
                if (commandTimeout) {
                    clearTimeout(commandTimeout);
                }
                
                // 即座に送信
                sendJoystickCommand(speed, steeringAngle, direction);
            }
        }
        
        function sendJoystickCommand(speed, steering, direction) {
            fetch('/joystick?speed=' + speed + '&steering=' + steering + '&direction=' + direction)
                .then(response => {
                    // コマンド送信成功
                })
                .catch(error => {
                    console.error('Error:', error);
                });
        }
    </script>
</body>
</html>
)html";
  
  server.send(200, "text/html", html);
}

// コマンド処理関数
void handleForward() {
  Serial.println("=== 前進 ===");
  moveForwardWithSpeed(70); // ボタン操作は70%の速度で固定
  server.send(200, "text/plain", "Forward");
}

void handleBackward() {
  Serial.println("=== 後進 ===");
  moveBackwardWithSpeed(70); // ボタン操作は70%の速度で固定
  server.send(200, "text/plain", "Backward");
}

void handleStop() {
  Serial.println("=== 停止 ===");
  stopMotors();
  server.send(200, "text/plain", "Stop");
}

void handleSteering() {
  if (server.hasArg("angle")) {
    int angle = server.arg("angle").toInt();
    Serial.print("=== ステアリング: ");
    Serial.print(angle);
    Serial.println("度 ===");
    setServoAngle(angle);
    server.send(200, "text/plain", "Steering: " + String(angle));
  } else {
    server.send(400, "text/plain", "Angle parameter required");
  }
}

void handleJoystick() {
  if (server.hasArg("speed") && server.hasArg("steering") && server.hasArg("direction")) {
    int speed = server.arg("speed").toInt();
    int steering = server.arg("steering").toInt();
    String direction = server.arg("direction");
    
    // シリアル出力を削減（デバッグ時のみ有効）
    // Serial.print("J: ");
    // Serial.print(speed);
    // Serial.print(",");
    // Serial.print(steering);
    // Serial.print(",");
    // Serial.println(direction);
    
    // ステアリング設定
    setServoAngle(steering);
    
    // モーター制御（PWM速度制御付き）
    if (speed > 5) {
      if (direction == "前進") {
        moveForwardWithSpeed(speed);
      } else if (direction == "後進") {
        moveBackwardWithSpeed(speed);
      } else if (direction == "左旋回") {
        // 左旋回の場合は前進しながら左にステアリング
        moveForwardWithSpeed(speed);
      } else if (direction == "右旋回") {
        // 右旋回の場合は前進しながら右にステアリング
        moveForwardWithSpeed(speed);
      }
    } else {
      stopMotors();
    }
    
    // レスポンスを高速化（最小限の応答）
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Error");
  }
}

// モーター制御関数（前進・後進・停止のみ）
void moveForward() {
  Serial.println("Motor A: 前進 (IN1=HIGH, IN2=LOW)");
  Serial.println("Motor B: 前進 (IN1=HIGH, IN2=LOW)");
  
  // Motor A: 前進
  digitalWrite(MOTOR_A_IN1, HIGH);
  digitalWrite(MOTOR_A_IN2, LOW);
  digitalWrite(MOTOR_A_PWM, HIGH);
  
  // Motor B: 前進
  digitalWrite(MOTOR_B_IN1, HIGH);
  digitalWrite(MOTOR_B_IN2, LOW);
  digitalWrite(MOTOR_B_PWM, HIGH);
}

void moveBackward() {
  Serial.println("Motor A: 後退 (IN1=LOW, IN2=HIGH)");
  Serial.println("Motor B: 後退 (IN1=LOW, IN2=HIGH)");
  
  // Motor A: 後退
  digitalWrite(MOTOR_A_IN1, LOW);
  digitalWrite(MOTOR_A_IN2, HIGH);
  digitalWrite(MOTOR_A_PWM, HIGH);
  
  // Motor B: 後退
  digitalWrite(MOTOR_B_IN1, LOW);
  digitalWrite(MOTOR_B_IN2, HIGH);
  digitalWrite(MOTOR_B_PWM, HIGH);
}


void moveForwardWithSpeed(int speedPercent) {
  // 速度をPWM値に変換（0-255）
  int pwmValue = map(speedPercent, 0, 100, 0, 255);
  
  // シリアル出力を削減（デバッグ時のみ有効）
  // Serial.print("F: ");
  // Serial.println(pwmValue);
  
  // Motor A: 前進
  digitalWrite(MOTOR_A_IN1, HIGH);
  digitalWrite(MOTOR_A_IN2, LOW);
  analogWrite(MOTOR_A_PWM, pwmValue);
  
  // Motor B: 前進
  digitalWrite(MOTOR_B_IN1, HIGH);
  digitalWrite(MOTOR_B_IN2, LOW);
  analogWrite(MOTOR_B_PWM, pwmValue);
}

void moveBackwardWithSpeed(int speedPercent) {
  // 速度をPWM値に変換（0-255）
  int pwmValue = map(speedPercent, 0, 100, 0, 255);
  
  // シリアル出力を削減（デバッグ時のみ有効）
  // Serial.print("B: ");
  // Serial.println(pwmValue);
  
  // Motor A: 後退
  digitalWrite(MOTOR_A_IN1, LOW);
  digitalWrite(MOTOR_A_IN2, HIGH);
  analogWrite(MOTOR_A_PWM, pwmValue);
  
  // Motor B: 後退
  digitalWrite(MOTOR_B_IN1, LOW);
  digitalWrite(MOTOR_B_IN2, HIGH);
  analogWrite(MOTOR_B_PWM, pwmValue);
}

void stopMotors() {
  Serial.println("Motor A: 停止 (IN1=LOW, IN2=LOW)");
  Serial.println("Motor B: 停止 (IN1=LOW, IN2=LOW)");
  
  // Motor A: 停止
  digitalWrite(MOTOR_A_IN1, LOW);
  digitalWrite(MOTOR_A_IN2, LOW);
  analogWrite(MOTOR_A_PWM, 0);
  
  // Motor B: 停止
  digitalWrite(MOTOR_B_IN1, LOW);
  digitalWrite(MOTOR_B_IN2, LOW);
  analogWrite(MOTOR_B_PWM, 0);
} 