#include <Servo.h>

// กำหนดพินของปุ่มกด
const int buttonRedPin = 2;    // K1
const int buttonYellowPin = 3; // K2
const int buttonBluePin = 4;   // K3
const int buttonGreenPin = 5;  // K4

// พินของเซอร์โว
const int baseServoPin = 6;
const int shoulderServoPin = 9;
const int elbowServoPin = 10;
const int gripperServoPin = 11;

Servo baseServo;
Servo shoulderServo;
Servo elbowServo;
Servo gripperServo;

// ตำแหน่งของเซอร์โว
int basePosition = 90;
int shoulderPosition = 90;
int elbowPosition = 90;
int gripperPosition = 90;

// ตัวแปรสำหรับการควบคุมทิศทาง
bool directions[4] = {true, true, true, true}; // true = เพิ่มองศา, false = ลดองศา

// ตัวแปรสำหรับตรวจสอบการกดค้าง
unsigned long lastPressTime[4] = {0, 0, 0, 0};
bool buttonStates[4] = {false, false, false, false};
unsigned long buttonHoldDelay = 500; // เวลาในการกดค้าง (มิลลิวินาที)
unsigned long lastMoveTime[4] = {0, 0, 0, 0};
unsigned long moveInterval = 10; // ระยะเวลาระหว่างการเคลื่อนที่ (มิลลิวินาที)

void setup() {
  Serial.begin(115200);
  
  pinMode(buttonRedPin, INPUT_PULLUP);
  pinMode(buttonYellowPin, INPUT_PULLUP);
  pinMode(buttonBluePin, INPUT_PULLUP);
  pinMode(buttonGreenPin, INPUT_PULLUP);
  
  baseServo.attach(baseServoPin);
  shoulderServo.attach(shoulderServoPin);
  elbowServo.attach(elbowServoPin);
  gripperServo.attach(gripperServoPin);

  baseServo.attach(baseServoPin, 500, 2400);
  shoulderServo.attach(shoulderServoPin, 500, 2400);
  elbowServo.attach(elbowServoPin, 500, 2400);
  gripperServo.attach(gripperServoPin, 500, 2400);

  Serial.println("Arduino initialized");


  controlServo("Base", basePosition);
  controlServo("Shoulder", shoulderPosition);
  controlServo("Elbow", elbowPosition);
  controlServo("Gripper", gripperPosition);
}

void loop() {
  checkButton(0, buttonRedPin, baseServo, basePosition);
  checkButton(1, buttonYellowPin, shoulderServo, shoulderPosition);
  checkButton(2, buttonBluePin, elbowServo, elbowPosition);
  checkButton(3, buttonGreenPin, gripperServo, gripperPosition);

  // รับคำสั่งจาก ESP8266
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    Serial.print("Received from ESP8266: ");
    Serial.println(command);
    executeCommand(command);
  }

  delay(15);
}

void controlServo(String servoName, int angle) {
  Servo* targetServo;
  int* targetPosition;

  if (servoName == "Base") {
    targetServo = &baseServo;
    targetPosition = &basePosition;
  } else if (servoName == "Shoulder") {
    targetServo = &shoulderServo;
    targetPosition = &shoulderPosition;
  } else if (servoName == "Elbow") {
    targetServo = &elbowServo;
    targetPosition = &elbowPosition;
  } else if (servoName == "Gripper") {
    targetServo = &gripperServo;
    targetPosition = &gripperPosition;
  } else {
    return; // ไม่พบ servo ที่ต้องการ
  }

  // จำกัดค่าองศาระหว่าง 0-180
  angle = constrain(angle, 0, 180);
  
  targetServo->write(angle);
  *targetPosition = angle;

  // ส่งข้อมูลกลับไปยัง ESP8266
  Serial.println(servoName + "," + String(angle));
}

void checkButton(int index, int buttonPin, Servo &servo, int &position) {
  bool currentState = !digitalRead(buttonPin);
  unsigned long currentTime = millis();
  
  if (currentState != buttonStates[index]) {
    if (currentState) {
      lastPressTime[index] = currentTime;
    } else {
      unsigned long pressDuration = currentTime - lastPressTime[index];
      Serial.print("Button ");
      Serial.print(index);
      Serial.print(" was pressed for ");
      Serial.print(pressDuration);
      Serial.println(" ms");
    }
    buttonStates[index] = currentState;
  }

  if (currentState && (currentTime - lastPressTime[index] >= buttonHoldDelay) &&
      (currentTime - lastMoveTime[index] >= moveInterval)) {
    String servoName;
    switch(index) {
      case 0: servoName = "Base"; break;
      case 1: servoName = "Shoulder"; break;
      case 2: servoName = "Elbow"; break;
      case 3: servoName = "Gripper"; break;
    }

    if (directions[index]) {
      position++;
      if (position >= 180) {
        position = 180;
        directions[index] = false;
      }
    } else {
      position--;
      if (position <= 0) {
        position = 0;
        directions[index] = true;
      }
    }

    controlServo(servoName, position);
    lastMoveTime[index] = currentTime;
  }
}

void executeCommand(String command) {
  int commaIndex = command.indexOf(',');
  if (commaIndex != -1) {
    String servoName = command.substring(0, commaIndex);
    int angle = command.substring(commaIndex + 1).toInt();
    controlServo(servoName, angle);
  }
}