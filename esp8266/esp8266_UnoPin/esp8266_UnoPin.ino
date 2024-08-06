#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <vector>
#include <string>
#include <sstream>

struct ServoInfo {
  String servoName;
  int currentPosition;
};

std::vector<ServoInfo> servoInfos = {
    { "Base", 90 },
    { "Shoulder", 90 },
    { "Elbow", 90 },
    { "Gripper", 90 }
};

struct RecordedStep {
  int servoIndex;
  int value;
  int delayInStep;
};

std::vector<RecordedStep> recordedSteps;

bool recordSteps = false;
bool playRecordedSteps = false;

unsigned long previousTimeInMilli = millis();

const char *ssid = "RobotArm";
const char *password = "12345678";

AsyncWebServer server(80);
AsyncWebSocket wsRobotArmInput("/RobotArmInput");

// HTML content remains the same as in your original code
const char *htmlHomePage PROGMEM = R"HTMLHOMEPAGE(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
<title>Robot Arm Control</title>
<style>
  body {
    font-family: Arial, sans-serif;
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    color: #ffffff;
    margin: 0;
    padding: 20px;
    min-height: 100vh;
    display: flex;
    flex-direction: column;
    justify-content: center;
    align-items: center;
  }
  .container {
    background: rgba(255, 255, 255, 0.1);
    border-radius: 10px;
    padding: 20px;
    box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
    backdrop-filter: blur(10px);
    max-width: 500px;
    width: 100%;
  }
  h1, h2 {
    text-align: center;
    color: #ffffff;
    text-shadow: 2px 2px 4px rgba(0,0,0,0.5);
  }
  input[type=button] {
    background-color: #4CAF50;
    color: white;
    border: none;
    border-radius: 30px;
    width: 100%;
    height: 40px;
    font-size: 18px;
    text-align: center;
    cursor: pointer;
    transition: background-color 0.3s, transform 0.1s;
  }
  input[type=button]:hover {
    background-color: #45a049;
  }
  input[type=button]:active {
    transform: scale(0.98);
  }
  .slidecontainer {
    width: 100%;
    display: flex;
    align-items: center;
  }
  .slider {
    -webkit-appearance: none;
    width: 100%;
    height: 15px;
    border-radius: 5px;
    background: #d3d3d3;
    outline: none;
    opacity: 0.7;
    -webkit-transition: .2s;
    transition: opacity .2s;
    margin-right: 10px;
  }
  .slider:hover {
    opacity: 1;
  }
  .slider::-webkit-slider-thumb {
    -webkit-appearance: none;
    appearance: none;
    width: 25px;
    height: 25px;
    border-radius: 50%;
    background: #4CAF50;
    cursor: pointer;
  }
  .slider::-moz-range-thumb {
    width: 25px;
    height: 25px;
    border-radius: 50%;
    background: #4CAF50;
    cursor: pointer;
  }
  table {
    width: 100%;
    border-collapse: separate;
    border-spacing: 0 15px;
  }
  td {
    padding: 10px;
  }
  .label {
    font-size: 18px;
    font-weight: bold;
  }
  .value {
    font-size: 18px;
    width: 40px;
    text-align: right;
  }
</style>
</head>
<body class="noselect">
<div class="container">
  <h1>Robot Arm Control</h1>
  <table id="mainTable">
    <tr>
      <td class="label">Base:</td>
      <td colspan="2">
        <div class="slidecontainer">
          <input type="range" min="0" max="180" value="90" class="slider" id="Base">
          <span class="value" id="BaseValue">90</span>
        </div>
      </td>
    </tr>
    <tr>
      <td class="label">Shoulder:</td>
      <td colspan="2">
        <div class="slidecontainer">
          <input type="range" min="0" max="180" value="90" class="slider" id="Shoulder">
          <span class="value" id="ShoulderValue">90</span>
        </div>
      </td>
    </tr>
    <tr>
      <td class="label">Elbow:</td>
      <td colspan="2">
        <div class="slidecontainer">
          <input type="range" min="0" max="180" value="90" class="slider" id="Elbow">
          <span class="value" id="ElbowValue">90</span>
        </div>
      </td>
    </tr>
    <tr>
      <td class="label">Gripper:</td>
      <td colspan="2">
        <div class="slidecontainer">
          <input type="range" min="0" max="180" value="90" class="slider" id="Gripper">
          <span class="value" id="GripperValue">90</span>
        </div>
      </td>
    </tr>
    <tr>
      <td class="label">Record:</td>
      <td><input type="button" id="Record" value="OFF"></td>
    </tr>
    <tr>
      <td class="label">Play:</td>
      <td><input type="button" id="Play" value="OFF"></td>
    </tr>
  </table>
</div>

<script>
class RobotArmControl {
  constructor() {
    this.webSocketUrl = "ws://192.168.4.1/RobotArmInput";
    this.websocket = null;
    this.lastSendTime = 0;
    this.sendInterval = 50; // ส่งค่าทุก 50 มิลลิวินาที
    this.initWebSocket();
  }

  async initWebSocket() {
    try {
      this.websocket = new WebSocket(this.webSocketUrl);
      this.websocket.onopen = this.onWebSocketOpen.bind(this);
      this.websocket.onclose = this.onWebSocketClose.bind(this);
      this.websocket.onmessage = this.onWebSocketMessage.bind(this);
      this.websocket.onerror = this.onWebSocketError.bind(this);
    } catch (error) {
      console.error('WebSocket initialization error:', error);
      setTimeout(() => this.initWebSocket(), 2000);
    }
  }

  onWebSocketOpen(event) {
    console.log('WebSocket connection established');
  }

  onWebSocketClose(event) {
    console.log('WebSocket connection closed. Reconnecting...');
    setTimeout(() => this.initWebSocket(), 2000);
  }

  onWebSocketMessage(event) {
    const [key, value] = event.data.split(',');
    const element = document.getElementById(key);
    if (element) {
      if (element.type === 'range') {
        element.value = value;
        document.getElementById(key + 'Value').textContent = value;
      } else if (element.type === 'button') {
        element.value = value;
        element.style.backgroundColor = value === "ON" ? "#45a049" : "#4CAF50";
        this.enableDisableButtonsSliders(element);
      }
    }
  }

  onWebSocketError(error) {
    console.error('WebSocket error:', error);
  }

  sendMessage(key, value) {
    const data = `${key},${value}`;
    console.log(data);
    if (this.websocket && this.websocket.readyState === WebSocket.OPEN) {
      this.websocket.send(data);
    }
  }

  onClickButton(button) {
    button.value = button.value === "ON" ? "OFF" : "ON";
    button.style.backgroundColor = button.value === "ON" ? "#45a049" : "#4CAF50";
    const value = button.value === "ON" ? 1 : 0;
    this.sendMessage(button.id, value);
    this.enableDisableButtonsSliders(button);
  }

  enableDisableButtonsSliders(button) {
    const isRecording = button.id === "Record" && button.value === "ON";
    const buttons = document.getElementsByTagName("input");
    for (const btn of buttons) {
      if (btn.type === "button") {
        if (btn.id !== "Record" && btn.id !== "Play") {
          btn.disabled = isRecording;
        }
      }
    }
  }

  init() {
    window.addEventListener('load', () => {
      document.getElementById("mainTable").addEventListener("touchend", (event) => {
        event.preventDefault();
      });

      const buttons = document.querySelectorAll('input[type="button"]');
      buttons.forEach(button => {
        button.addEventListener('touchstart', (e) => {
          e.preventDefault();
          this.onClickButton(button);
        });
        button.addEventListener('click', () => this.onClickButton(button));
      });

      const sliders = document.querySelectorAll('input[type="range"]');
      sliders.forEach(slider => {
        const valueSpan = document.getElementById(slider.id + 'Value');
        slider.addEventListener('input', () => {
          valueSpan.textContent = slider.value;
          const currentTime = Date.now();
          if (currentTime - this.lastSendTime >= this.sendInterval) {
            this.sendMessage(slider.id, slider.value);
            this.lastSendTime = currentTime;
          }
        });
      });
    });
  }
}

const robotArmControl = new RobotArmControl();
robotArmControl.init();
</script>
</body>
</html>
)HTMLHOMEPAGE";

void handleRoot(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", htmlHomePage);
}

void handleNotFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "File Not Found");
}

void sendCurrentRobotArmState() {
  for (int i = 0; i < servoInfos.size(); i++) {
    String message = servoInfos[i].servoName + "," + String(servoInfos[i].currentPosition);
    wsRobotArmInput.textAll(message);
  }
  wsRobotArmInput.textAll(String("Record,") + (recordSteps ? "ON" : "OFF"));
  wsRobotArmInput.textAll(String("Play,") + (playRecordedSteps ? "ON" : "OFF"));
}

void writeServoValues(int servoIndex, int value) {
  if (recordSteps) {
    RecordedStep recordedStep;
    if (recordedSteps.size() == 0) {
      for (int i = 0; i < servoInfos.size(); i++) {
        recordedStep.servoIndex = i;
        recordedStep.value = servoInfos[i].currentPosition;
        recordedStep.delayInStep = 0;
        recordedSteps.push_back(recordedStep);
      }
    }
    unsigned long currentTime = millis();
    recordedStep.servoIndex = servoIndex;
    recordedStep.value = value;
    recordedStep.delayInStep = currentTime - previousTimeInMilli;
    recordedSteps.push_back(recordedStep);
    previousTimeInMilli = currentTime;
  }
  servoInfos[servoIndex].currentPosition = value;
  Serial.println(servoInfos[servoIndex].servoName + "," + String(value));
}

void playRecordedRobotArmSteps() {
  if (recordedSteps.size() == 0) {
    return;
  }
  for (int i = 0; i < recordedSteps.size() && playRecordedSteps; i++) {
    RecordedStep &recordedStep = recordedSteps[i];
    delay(recordedStep.delayInStep);
    servoInfos[recordedStep.servoIndex].currentPosition = recordedStep.value;
    String message = servoInfos[recordedStep.servoIndex].servoName + "," + String(recordedStep.value);
    Serial.println(message);
    wsRobotArmInput.textAll(message);
  }
}

void onRobotArmInputWebSocketEvent(AsyncWebSocket *server,
                                   AsyncWebSocketClient *client,
                                   AwsEventType type,
                                   void *arg,
                                   uint8_t *data,
                                   size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      sendCurrentRobotArmState();
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      AwsFrameInfo *info;
      info = (AwsFrameInfo *)arg;
      if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        String message = String((char*)data).substring(0, len);
        int commaIndex = message.indexOf(',');
        if (commaIndex != -1) {
          String key = message.substring(0, commaIndex);
          int value = message.substring(commaIndex + 1).toInt();
          Serial.printf("Key [%s] Value[%d]\n", key.c_str(), value);

          if (key == "Record") {
            recordSteps = value;
            if (recordSteps) {
              recordedSteps.clear();
              previousTimeInMilli = millis();
            }
          } else if (key == "Play") {
            playRecordedSteps = value;
          } else {
            for (int i = 0; i < servoInfos.size(); i++) {
              if (key == servoInfos[i].servoName) {
                writeServoValues(i, value);
                break;
              }
            }
          }
        }
      }
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
    default:
      break;
  }
}

void setup(void) {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);

  wsRobotArmInput.onEvent(onRobotArmInputWebSocketEvent);
  server.addHandler(&wsRobotArmInput);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  wsRobotArmInput.cleanupClients();
  if (playRecordedSteps) {
    playRecordedRobotArmSteps();
  }

  // รับข้อมูลจาก Arduino
  if (Serial.available()) {
    String data = Serial.readStringUntil('\n');
    int commaIndex = data.indexOf(',');
    if (commaIndex != -1) {
      String servoName = data.substring(0, commaIndex);
      int position = data.substring(commaIndex + 1).toInt();
      for (int i = 0; i < servoInfos.size(); i++) {
        if (servoInfos[i].servoName == servoName) {
          servoInfos[i].currentPosition = position;
          String message = servoName + "," + String(position);
          wsRobotArmInput.textAll(message);
          break;
        }
      }
    }
  }
}