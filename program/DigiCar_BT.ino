#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

// デバッグ出力の制御
#define DEBUG_SERIAL 1  // 0: 無効化、1: 有効化

// モーター制御ピン（TB6612FNG）
#define MOTOR_A_IN1  26  // GPIO26
#define MOTOR_A_IN2  25  // GPIO25
#define MOTOR_A_PWM  33  // GPIO33
#define MOTOR_B_IN1  27  // GPIO27
#define MOTOR_B_IN2  14  // GPIO14
#define MOTOR_B_PWM  12  // GPIO12

// サーボモーター制御ピン（ステアリング）
#define SERVO_PIN    32  // GPIO32

// Bluetooth設定
BluetoothSerial SerialBT;
const char* deviceName = "DigiCar_Control";

void setup() {
  Serial.begin(115200);
  #if DEBUG_SERIAL
  Serial.println("=== DigiCar Steering Control (Simple) ===");
  #endif
  
  // CPU周波数を下げて発熱を軽減
  setCpuFrequencyMhz(80);
  #if DEBUG_SERIAL
  Serial.print("CPU Frequency: ");
  Serial.print(getCpuFrequencyMhz());
  Serial.println(" MHz");
  #endif
  
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
  #if DEBUG_SERIAL
  Serial.println("Motors and Steering initialized");
  #endif
  
  // Bluetooth起動
  SerialBT.begin(deviceName);
  #if DEBUG_SERIAL
  Serial.println("Bluetooth started");
  Serial.print("Device Name: ");
  Serial.println(deviceName);
  Serial.println("Commands: F<speed>, B<speed>, S, T<angle>, J<speed>,<steering>,<direction>");
  #endif
}

void loop() {
  // Bluetooth通信を処理
  if (SerialBT.available()) {
    String command = SerialBT.readStringUntil('\n');
    command.trim(); // 前後の空白文字（スペース、タブ、改行など）を削除
    
    #if DEBUG_SERIAL
    Serial.print("Received: ");
    Serial.println(command);
    #endif
    
    parseCommand(command); // コマンドを解析して実行
  }
  
  // サーボ制御を非同期で実行
  updateServoAsync();
  
  yield();
}

// サーボ角度設定関数（ステアリング）- 非ブロッキング方式
static int currentServoAngle = 90;

void setServoAngle(int angle) {
  // 角度を60～120度の範囲に制限
  if (angle < 60) angle = 60;
  if (angle > 120) angle = 120;
  currentServoAngle = angle;
}

// サーボ制御を非同期で実行（非ブロッキング方式）
// 20msごとにパルスを送信して、Bluetooth通信をブロックしない
void updateServoAsync() {
  static unsigned long lastPulse = 0;
  unsigned long now = millis();
  
  // 20msごとにパルスを送信（50Hz）
  if (now - lastPulse >= 20) {
    lastPulse = now;
    // 角度をパルス幅に変換（0度=500us, 180度=2500us）
    int pulseWidth = map(currentServoAngle, 0, 180, 500, 2500);
    digitalWrite(SERVO_PIN, HIGH);
    delayMicroseconds(pulseWidth);
    digitalWrite(SERVO_PIN, LOW);
  }
}

// Bluetoothコマンド解析関数
// コマンド形式: F<speed>, B<speed>, S, T<angle>, J<speed>,<steering>,<direction>
void parseCommand(String command) {
  if (command.length() == 0) return;
  
  char cmd = command.charAt(0);
  
  if (cmd == 'F' || cmd == 'f') {
    // 前進: F<speed> (例: F50 = 50%速度で前進)
    int speed = command.substring(1).toInt();
    if (speed > 0 && speed <= 100) {
      moveForwardWithSpeed(speed);
      #if DEBUG_SERIAL
      Serial.print("Forward: ");
      Serial.println(speed);
      #endif
    }
  } else if (cmd == 'B' || cmd == 'b') {
    // 後退: B<speed> (例: B50 = 50%速度で後退)
    int speed = command.substring(1).toInt();
    if (speed > 0 && speed <= 100) {
      moveBackwardWithSpeed(speed);
      #if DEBUG_SERIAL
      Serial.print("Backward: ");
      Serial.println(speed);
      #endif
    }
  } else if (cmd == 'S' || cmd == 's') {
    // 停止: S
    stopMotors();
    #if DEBUG_SERIAL
    Serial.println("Stop");
    #endif
  } else if (cmd == 'T' || cmd == 't') {
    // ステアリング: T<angle> (例: T90 = 中央, T60 = 左端, T120 = 右端)
    int angle = command.substring(1).toInt();
    if (angle >= 60 && angle <= 120) {
      setServoAngle(angle);
      #if DEBUG_SERIAL
      Serial.print("Steering: ");
      Serial.println(angle);
      #endif
    }
  } else if (cmd == 'J' || cmd == 'j') {
    // ジョイスティック: J<speed>,<steering>,<direction>
    // 例: J50,90,前進 = 速度50%, ステアリング90度, 前進方向
    int firstComma = command.indexOf(',');
    int secondComma = command.indexOf(',', firstComma + 1);
    
    if (firstComma > 0 && secondComma > firstComma) {
      int speed = command.substring(1, firstComma).toInt();
      int steering = command.substring(firstComma + 1, secondComma).toInt();
      String direction = command.substring(secondComma + 1);
      
      if (steering >= 60 && steering <= 120) {
        setServoAngle(steering);
      }
      
      if (speed > 5) {
        if (direction == "前進" || direction == "forward" || direction == "F") {
          moveForwardWithSpeed(speed);
        } else if (direction == "後進" || direction == "backward" || direction == "B") {
          moveBackwardWithSpeed(speed);
        } else {
          moveForwardWithSpeed(speed); // デフォルトは前進
        }
      } else {
        stopMotors();
      }
      
      #if DEBUG_SERIAL
      Serial.print("Joystick: speed=");
      Serial.print(speed);
      Serial.print(", steering=");
      Serial.print(steering);
      Serial.print(", direction=");
      Serial.println(direction);
      #endif
    }
  }
  
  SerialBT.println("OK");
}

// モーター制御関数（PWM速度制御）
void moveForwardWithSpeed(int speedPercent) {
  // 速度をPWM値に変換（0-100% → 0-255）
  int pwmValue = map(speedPercent, 0, 100, 0, 255);
  
  // Motor A: 前進
  digitalWrite(MOTOR_A_IN1, HIGH);
  digitalWrite(MOTOR_A_IN2, LOW);
  analogWrite(MOTOR_A_PWM, pwmValue);
  
  // Motor B: 前進
  digitalWrite(MOTOR_B_IN1, HIGH);
  digitalWrite(MOTOR_B_IN2, LOW);
  analogWrite(MOTOR_B_PWM, pwmValue);
  
  yield();
}

void moveBackwardWithSpeed(int speedPercent) {
  // 速度をPWM値に変換（0-100% → 0-255）
  int pwmValue = map(speedPercent, 0, 100, 0, 255);
  
  // Motor A: 後退
  digitalWrite(MOTOR_A_IN1, LOW);
  digitalWrite(MOTOR_A_IN2, HIGH);
  analogWrite(MOTOR_A_PWM, pwmValue);
  
  // Motor B: 後退
  digitalWrite(MOTOR_B_IN1, LOW);
  digitalWrite(MOTOR_B_IN2, HIGH);
  analogWrite(MOTOR_B_PWM, pwmValue);
  
  yield();
}

void stopMotors() {
  #if DEBUG_SERIAL
  Serial.println("Stop motors");
  #endif
  
  digitalWrite(MOTOR_A_IN1, LOW);
  digitalWrite(MOTOR_A_IN2, LOW);
  analogWrite(MOTOR_A_PWM, 0);
  
  digitalWrite(MOTOR_B_IN1, LOW);
  digitalWrite(MOTOR_B_IN2, LOW);
  analogWrite(MOTOR_B_PWM, 0);
}


