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
const char* password = "digimoku";     // WiFiのパスワード
String ssid;                           // SSIDは起動時にMACアドレスから生成

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

  // MACアドレス下4桁をSSIDに付加（例: DigiCar_Control_B65D）
  // APモードで起動してからMACアドレスを取得
  WiFi.softAP("DigiCar_Control", password); // 仮SSIDで一旦起動
  String mac = WiFi.softAPmacAddress();     // APモードのMACアドレスを取得
  mac.replace(":", "");                     // コロン除去 → "A4CF12B65Dxx"
  String macSuffix = mac.substring(mac.length() - 6, mac.length() - 2); // 下2バイト = 4文字
  macSuffix.toUpperCase();
  ssid = "DigiCar_Control_" + macSuffix;

  // 正式なSSIDで再起動
  WiFi.softAP(ssid.c_str(), password);
  
  // WiFi省電力設定
  WiFi.setSleep(false); // WiFiスリープを無効化（安定性優先）
  
  // 省電力設定（安全な方法）
  #ifdef ESP32
    esp_wifi_set_ps(WIFI_PS_NONE); // WiFi省電力モードを無効化
  #endif
  
  // 起動時にシリアルへ接続情報を表示
  Serial.println("========================================");
  Serial.println("  WiFi AP started");
  Serial.print  ("  SSID     : ");
  Serial.println(ssid);
  Serial.print  ("  Password : ");
  Serial.println(password);
  Serial.print  ("  IP       : ");
  Serial.println(WiFi.softAPIP());
  Serial.println("  ブラウザで http://192.168.4.1 を開いてね！");
  Serial.println("========================================");
  
  // Webサーバーの設定
  server.on("/", handleRoot);
  server.on("/forward", handleForward);
  server.on("/backward", handleBackward);
  server.on("/stop", handleStop);
  server.on("/steering", handleSteering);
  server.on("/joystick", handleJoystick);
  
  server.begin();
  Serial.println("Web server started");
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
            /* テキスト選択を無効化 */
            -webkit-user-select: none;
            user-select: none;
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
            transition: all 0.1s;
            font-weight: bold;
            /* タップ時のハイライト無効化 */
            -webkit-tap-highlight-color: transparent;
            touch-action: none;
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
        .control-btn:active, .control-btn.pressed {
            transform: scale(0.95);
            filter: brightness(0.85);
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
            touch-action: none;
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
            <button class="control-btn forward" id="btnForward">↑<br>前進</button>
            <button class="control-btn stop" id="btnStop">■<br>停止</button>
            <button class="control-btn backward" id="btnBackward">↓<br>後進</button>
        </div>
        
        <div class="steering-section">
            <div class="steering-label">ステアリング制御</div>
            <input type="range" min="60" max="120" value="90" class="steering-slider" id="steeringSlider" oninput="updateSteering(this.value)">
            <div class="steering-value" id="steeringValue">中央 (90度)</div>
        </div>

    </div>

    <script>
        // ===== ジョイスティック用変数 =====
        let isDragging = false;
        let joystickKnob = document.getElementById('joystickKnob');
        let joystickContainer = document.getElementById('joystickContainer');
        let joystickSpeed = document.getElementById('joystickSpeed');
        let joystickDirection = document.getElementById('joystickDirection');
        let maxSpeedValue = document.getElementById('maxSpeedValue');
        
        const maxDistance = 50;
        let maxSpeed = 50;
        let lastCommand = '';

        // ===== ボタン用変数 =====
        // ジョイスティック使用中はボタンを無視するフラグ
        let joystickActive = false;
        // ボタン押しっぱなし中の繰り返し送信用タイマー
        let buttonInterval = null;
        // 現在押されているボタンのコマンド
        let activeButtonCommand = null;

        // ===== ボタンのタッチイベント設定 =====
        // 現在の走行状態（'forward'/'backward'/null）
        let driveState = null;

        function setDriveState(command) {
            // 前進・後進はトグル動作（停止ボタンを押すまで継続）
            const btnForward  = document.getElementById('btnForward');
            const btnBackward = document.getElementById('btnBackward');

            if (command === 'forward' || command === 'backward') {
                driveState = command;
                btnForward.classList.toggle('pressed', command === 'forward');
                btnBackward.classList.toggle('pressed', command === 'backward');
                sendCommand(command);
            } else {
                // 停止
                driveState = null;
                btnForward.classList.remove('pressed');
                btnBackward.classList.remove('pressed');
                sendCommand('stop');
            }
        }

        function setupButton(btnId, command) {
            const btn = document.getElementById(btnId);

            function onPress(e) {
                e.preventDefault();
                if (joystickActive) return; // ジョイスティック優先
                setDriveState(command);
            }

            btn.addEventListener('touchstart', onPress, { passive: false });
            btn.addEventListener('mousedown', onPress);
        }

        setupButton('btnForward', 'forward');
        setupButton('btnBackward', 'backward');
        setupButton('btnStop', 'stop');

        // ===== ジョイスティックのイベントリスナー =====
        joystickContainer.addEventListener('touchstart', handleJoystickStart, { passive: false });
        joystickContainer.addEventListener('mousedown', handleJoystickStart);
        document.addEventListener('touchmove', handleJoystickMove, { passive: false });
        document.addEventListener('mousemove', handleJoystickMove);
        document.addEventListener('touchend', handleJoystickEnd, { passive: false });
        document.addEventListener('mouseup', handleJoystickEnd);

        function sendCommand(command) {
            // 前進・後進はmaxSpeedをパラメータで渡す
            if (command === 'forward' || command === 'backward') {
                fetch('/' + command + '?speed=' + maxSpeed).catch(() => {});
            } else {
                fetch('/' + command).catch(() => {});
            }
        }
        
        function updateSteering(value) {
            const angle = parseInt(value);
            let direction = '';
            if (angle < 75) direction = '左';
            else if (angle > 105) direction = '右';
            else direction = '中央';
            document.getElementById('steeringValue').textContent = direction + ' (' + angle + '度)';
            fetch('/steering?angle=' + angle).catch(() => {});
        }
        
        function handleJoystickStart(e) {
            e.preventDefault();
            isDragging = true;
            joystickActive = true;
            // ボタンの繰り返し送信を止める
            if (buttonInterval) {
                clearInterval(buttonInterval);
                buttonInterval = null;
            }
            joystickKnob.style.transition = 'none';
            joystickKnob.style.transform = 'none'; // translate(-50%,-50%)のオフセットをリセット
            // ジョイスティック操作開始時にボタンの走行状態をリセット
            driveState = null;
            document.getElementById('btnForward').classList.remove('pressed');
            document.getElementById('btnBackward').classList.remove('pressed');
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
            // コンテナの実際の表示サイズから中心と可動範囲を動的計算
            const centerX = rect.width  / 2;
            const centerY = rect.height / 2;
            const dynamicMaxDist = centerX * 0.67; // 可動範囲 = 半径の約2/3
            const x = clientX - rect.left - centerX;
            const y = clientY - rect.top  - centerY;
            
            const distance = Math.sqrt(x * x + y * y);
            const angle = Math.atan2(y, x);
            
            let finalX, finalY;
            if (distance > dynamicMaxDist) {
                finalX = Math.cos(angle) * dynamicMaxDist;
                finalY = Math.sin(angle) * dynamicMaxDist;
            } else {
                finalX = x;
                finalY = y;
            }
            
            // ノブ位置をコンテナ内CSS座標系で更新
            const knobHalf = joystickKnob.offsetWidth / 2;
            joystickKnob.style.left = (centerX + finalX - knobHalf) + 'px';
            joystickKnob.style.top  = (centerY + finalY - knobHalf) + 'px';
            
            updateJoystickControl(finalX, finalY, dynamicMaxDist);
        }
        
        function handleJoystickEnd(e) {
            if (!isDragging) return;
            e.preventDefault();
            isDragging = false;
            joystickActive = false;
            joystickKnob.style.transition = 'all 0.3s ease';
            joystickKnob.style.left = '50%';
            joystickKnob.style.top = '50%';
            joystickKnob.style.transform = 'translate(-50%, -50%)';
            joystickSpeed.textContent = '0%';
            joystickDirection.textContent = '停止';
            sendCommand('stop');
        }
        
        function updateMaxSpeed(value) {
            maxSpeed = parseInt(value);
            document.getElementById('maxSpeedValue').textContent = maxSpeed + '%';
            // 走行中なら即座に速度を更新
            if (driveState === 'forward' || driveState === 'backward') {
                sendCommand(driveState);
            }
        }
        
        function updateJoystickControl(x, y, dynMax) {
            const distance = Math.sqrt(x * x + y * y);
            const effectiveMax = dynMax || maxDistance;
            const speed = Math.min(Math.round((distance / effectiveMax) * maxSpeed), maxSpeed);
            
            joystickSpeed.textContent = speed + '%';
            
            let direction = '停止';
            if (speed > 5) {
                if (y < -10) direction = '前進';
                else if (y > 10) direction = '後進';
                else if (x < -10) direction = '左旋回';
                else if (x > 10) direction = '右旋回';
            }
            joystickDirection.textContent = direction;
            
            const steeringAngle = Math.round(90 + (x / effectiveMax) * 30);
            const newCommand = speed + ',' + steeringAngle + ',' + direction;
            
            if (newCommand !== lastCommand) {
                lastCommand = newCommand;
                sendJoystickCommand(speed, steeringAngle, direction);
            }
        }
        
        function sendJoystickCommand(speed, steering, direction) {
            fetch('/joystick?speed=' + speed + '&steering=' + steering + '&direction=' + direction)
                .catch(() => {});
        }
    </script>
</body>
</html>
)html";
  
  server.send(200, "text/html", html);
}

// コマンド処理関数
void handleForward() {
  // speedパラメータがあればその値を、なければデフォルト70%を使用
  int speed = server.hasArg("speed") ? server.arg("speed").toInt() : 70;
  Serial.print("=== 前進: ");
  Serial.print(speed);
  Serial.println("% ===");
  moveForwardWithSpeed(speed);
  server.send(200, "text/plain", "Forward");
}

void handleBackward() {
  // speedパラメータがあればその値を、なければデフォルト70%を使用
  int speed = server.hasArg("speed") ? server.arg("speed").toInt() : 70;
  Serial.print("=== 後進: ");
  Serial.print(speed);
  Serial.println("% ===");
  moveBackwardWithSpeed(speed);
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
    
    setServoAngle(steering);
    
    if (speed > 5) {
      if (direction == "前進") {
        moveForwardWithSpeed(speed);
      } else if (direction == "後進") {
        moveBackwardWithSpeed(speed);
      } else if (direction == "左旋回") {
        moveForwardWithSpeed(speed);
      } else if (direction == "右旋回") {
        moveForwardWithSpeed(speed);
      }
    } else {
      stopMotors();
    }
    
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Error");
  }
}

// モーター制御関数
void moveForwardWithSpeed(int speedPercent) {
  int pwmValue = map(speedPercent, 0, 100, 0, 255);
  digitalWrite(MOTOR_A_IN1, HIGH);
  digitalWrite(MOTOR_A_IN2, LOW);
  analogWrite(MOTOR_A_PWM, pwmValue);
  digitalWrite(MOTOR_B_IN1, HIGH);
  digitalWrite(MOTOR_B_IN2, LOW);
  analogWrite(MOTOR_B_PWM, pwmValue);
}

void moveBackwardWithSpeed(int speedPercent) {
  int pwmValue = map(speedPercent, 0, 100, 0, 255);
  digitalWrite(MOTOR_A_IN1, LOW);
  digitalWrite(MOTOR_A_IN2, HIGH);
  analogWrite(MOTOR_A_PWM, pwmValue);
  digitalWrite(MOTOR_B_IN1, LOW);
  digitalWrite(MOTOR_B_IN2, HIGH);
  analogWrite(MOTOR_B_PWM, pwmValue);
}

void moveForward() {
  digitalWrite(MOTOR_A_IN1, HIGH);
  digitalWrite(MOTOR_A_IN2, LOW);
  digitalWrite(MOTOR_A_PWM, HIGH);
  digitalWrite(MOTOR_B_IN1, HIGH);
  digitalWrite(MOTOR_B_IN2, LOW);
  digitalWrite(MOTOR_B_PWM, HIGH);
}

void moveBackward() {
  digitalWrite(MOTOR_A_IN1, LOW);
  digitalWrite(MOTOR_A_IN2, HIGH);
  digitalWrite(MOTOR_A_PWM, HIGH);
  digitalWrite(MOTOR_B_IN1, LOW);
  digitalWrite(MOTOR_B_IN2, HIGH);
  digitalWrite(MOTOR_B_PWM, HIGH);
}

void stopMotors() {
  digitalWrite(MOTOR_A_IN1, LOW);
  digitalWrite(MOTOR_A_IN2, LOW);
  analogWrite(MOTOR_A_PWM, 0);
  digitalWrite(MOTOR_B_IN1, LOW);
  digitalWrite(MOTOR_B_IN2, LOW);
  analogWrite(MOTOR_B_PWM, 0);
}
