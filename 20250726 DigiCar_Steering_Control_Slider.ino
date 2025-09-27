#include <WiFi.h>
#include <WebServer.h>

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
  
  // WiFi APモードで起動
  WiFi.softAP(ssid, password);
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
  
  server.begin();
  Serial.println("Web server started");
  Serial.println("Connect to WiFi 'DigiCar_Control' and visit http://192.168.4.1");
}

void loop() {
  server.handleClient();
  delay(10);
}

// サーボ角度設定関数（ステアリング）- 簡易版
void setServoAngle(int angle) {
  // 角度をパルス幅に変換（0度=500us, 180度=2500us）
  int pulseWidth = map(angle, 0, 180, 500, 2500);
  
  // サーボパルスを送信
  digitalWrite(SERVO_PIN, HIGH);
  delayMicroseconds(pulseWidth);
  digitalWrite(SERVO_PIN, LOW);
  delay(20); // 20ms待機
  
  Serial.print("Steering angle: ");
  Serial.print(angle);
  Serial.print(" degrees, Pulse: ");
  Serial.print(pulseWidth);
  Serial.println(" us");
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
    </style>
</head>
<body>
    <div class="container">
        <h1>🚗 DigiCar</h1>
        
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
    </script>
</body>
</html>
)html";
  
  server.send(200, "text/html", html);
}

// コマンド処理関数
void handleForward() {
  Serial.println("=== 前進 ===");
  moveForward();
  server.send(200, "text/plain", "Forward");
}

void handleBackward() {
  Serial.println("=== 後進 ===");
  moveBackward();
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

void stopMotors() {
  Serial.println("Motor A: 停止 (IN1=LOW, IN2=LOW)");
  Serial.println("Motor B: 停止 (IN1=LOW, IN2=LOW)");
  
  // Motor A: 停止
  digitalWrite(MOTOR_A_IN1, LOW);
  digitalWrite(MOTOR_A_IN2, LOW);
  digitalWrite(MOTOR_A_PWM, LOW);
  
  // Motor B: 停止
  digitalWrite(MOTOR_B_IN1, LOW);
  digitalWrite(MOTOR_B_IN2, LOW);
  digitalWrite(MOTOR_B_PWM, LOW);
} 