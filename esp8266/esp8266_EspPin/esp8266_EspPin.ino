#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Servo.h>

#include <vector>
#include <string>
#include <sstream>

struct ServoPins {
  Servo servo;
  int servoPin;
  String servoName;
  int initialPosition;
};

std::vector<ServoPins> servoPins = {
    { Servo(), 16, "Base", 90 },      // D0 is GPIO16
    { Servo(), 2, "Shoulder", 90 },   // D1 is GPIO2
    { Servo(), 4, "Elbow", 90 },      // D2 is GPIO4
    { Servo(), 0, "Gripper", 90 }     // D3 is GPIO0
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

const char *htmlHomePage PROGMEM = R"HTMLHOMEPAGE(
<!DOCTYPE html>
  <html>
  <head>
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
    <style>
    input[type=button] {
      background-color:red;color:white;border-radius:30px;width:100%;height:40px;font-size:20px;text-align:center;
    }
    .noselect {
      -webkit-touch-callout: none; 
      -webkit-user-select: none; 
      -khtml-user-select: none; 
      -moz-user-select: none; 
      -ms-user-select: none; 
      user-select: none; 
    }
    .slidecontainer {
      width: 100%;
    }
    .slider {
      -webkit-appearance: none;
      width: 100%;
      height: 20px;
      border-radius: 5px;
      background: #d3d3d3;
      outline: none;
      opacity: 0.7;
      -webkit-transition: .2s;
      transition: opacity .2s;
    }
    .slider:hover {
      opacity: 1;
    }
    .slider::-webkit-slider-thumb {
      -webkit-appearance: none;
      appearance: none;
      width: 40px;
      height: 40px;
      border-radius: 50%;
      background: red;
      cursor: pointer;
    }
    .slider::-moz-range-thumb {
      width: 40px;
      height: 40px;
      border-radius: 50%;
      background: red;
      cursor: pointer;
    }
    </style>

  </head>
  <body class="noselect" align="center" style="background-color:white">
    <h1 style="color: teal;text-align:center;">Hash Include Electronics</h1>
    <h2 style="color: teal;text-align:center;">Robot Arm Control</h2>
    <table id="mainTable" style="width:400px;margin:auto;table-layout:fixed" CELLSPACING=10>
      <tr/><tr/>
      <tr/><tr/>
      <tr>
        <td style="text-align:left;font-size:25px"><b>Gripper:</b></td>
        <td colspan=2>
        <div class="slidecontainer">
            <input type="range" min="0" max="180" value="90" class="slider" id="Gripper">
          </div>
        </td>
      </tr> 
      <tr/><tr/>
      <tr>
        <td style="text-align:left;font-size:25px"><b>Elbow:</b></td>
        <td colspan=2>
        <div class="slidecontainer">
            <input type="range" min="0" max="180" value="90" class="slider" id="Elbow">
          </div>
        </td>
      </tr> 
      <tr/><tr/>      
      <tr>
        <td style="text-align:left;font-size:25px"><b>Shoulder:</b></td>
        <td colspan=2>
        <div class="slidecontainer">
            <input type="range" min="0" max="180" value="90" class="slider" id="Shoulder">
          </div>
        </td>
      </tr>  
      <tr/><tr/>      
      <tr>
        <td style="text-align:left;font-size:25px"><b>Base:</b></td>
        <td colspan=2>
        <div class="slidecontainer">
            <input type="range" min="0" max="180" value="90" class="slider" id="Base">
          </div>
        </td>
      </tr> 
      <tr/><tr/> 
      <tr>
        <td style="text-align:left;font-size:25px"><b>Record:</b></td>
        <td><input type="button" id="Record" value="OFF"></td>
        <td></td>
      </tr>
      <tr/><tr/> 
      <tr>
        <td style="text-align:left;font-size:25px"><b>Play:</b></td>
        <td><input type="button" id="Play" value="OFF"></td>
        <td></td>
      </tr>      
    </table>

    <script>
      class RobotArmControl {
        constructor() {
          this.webSocketUrl = "ws://192.168.4.1/RobotArmInput";
          this.websocket = null;
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
          const button = document.getElementById(key);
          if (button) {
            button.value = value;
            if (button.id === "Record" || button.id === "Play") {
              button.style.backgroundColor = value === "ON" ? "green" : "red";
              this.enableDisableButtonsSliders(button);
            }
          }
        }

        onWebSocketError(error) {
          console.error('WebSocket error:', error);
        }

        sendButtonInput(key, value) {
          const data = `${key},${value}`;
          console.log(value);
          if (this.websocket && this.websocket.readyState === WebSocket.OPEN) {
            this.websocket.send(data);
          }
        }

        onClickButton(button) {
          button.value = button.value === "ON" ? "OFF" : "ON";
          button.style.backgroundColor = button.value === "ON" ? "green" : "red";
          const value = button.value === "ON" ? 1 : 0;
          this.sendButtonInput(button.id, value);
          this.enableDisableButtonsSliders(button);
        }

        enableDisableButtonsSliders(button) {
          const disable = button.id === "Record" && button.value === "ON";
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
              slider.addEventListener('input', () => this.sendButtonInput(slider.id, slider.value));
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
  for (int i = 0; i < servoPins.size(); i++) {
    wsRobotArmInput.textAll(servoPins[i].servoName + "," + servoPins[i].servo.read());
  }
  wsRobotArmInput.textAll(String("Record,") + (recordSteps ? "ON" : "OFF"));
  wsRobotArmInput.textAll(String("Play,") + (playRecordedSteps ? "ON" : "OFF"));
}

void writeServoValues(int servoIndex, int value) {
  if (recordSteps) {
    RecordedStep recordedStep;
    if (recordedSteps.size() == 0)  // We will first record initial position of all servos.
    {
      for (int i = 0; i < servoPins.size(); i++) {
        recordedStep.servoIndex = i;
        recordedStep.value = servoPins[i].servo.read();
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
  servoPins[servoIndex].servo.write(value);
}

void playRecordedRobotArmSteps() {
  if (recordedSteps.size() == 0) {
    return;
  }
  for (int i = 0; i < 4 && playRecordedSteps; i++) {
    RecordedStep &recordedStep = recordedSteps[i];
    int currentServoPosition = servoPins[recordedStep.servoIndex].servo.read();
    while (currentServoPosition != recordedStep.value && playRecordedSteps) {
      currentServoPosition = (currentServoPosition > recordedStep.value ? currentServoPosition - 1 : currentServoPosition + 1);
      servoPins[recordedStep.servoIndex].servo.write(currentServoPosition);
      wsRobotArmInput.textAll(servoPins[recordedStep.servoIndex].servoName + "," + currentServoPosition);
      delay(50);
    }
  }
  delay(1000);  

  for (int i = 4; i < recordedSteps.size() && playRecordedSteps; i++) {
    RecordedStep &recordedStep = recordedSteps[i];
    delay(recordedStep.delayInStep);
    servoPins[recordedStep.servoIndex].servo.write(recordedStep.value);
    wsRobotArmInput.textAll(servoPins[recordedStep.servoIndex].servoName + "," + recordedStep.value);
  }
}

void setUpPinModes() {
  for (int i = 0; i < servoPins.size(); i++) {
    servoPins[i].servo.attach(servoPins[i].servoPin,500,2500);
    servoPins[i].servo.write(servoPins[i].initialPosition);
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
      // Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      sendCurrentRobotArmState();
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      AwsFrameInfo *info;
      info = (AwsFrameInfo *)arg;
      if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        std::string myData = "";
        myData.assign((char *)data, len);
        std::istringstream ss(myData);
        std::string key, value;
        std::getline(ss, key, ',');
        std::getline(ss, value, ',');
        Serial.printf("Key [%s] Value[%s]\n", key.c_str(), value.c_str());
        int valueInt = atoi(value.c_str());

        if (key == "Record") {
          recordSteps = valueInt;
          if (recordSteps) {
            recordedSteps.clear();
            previousTimeInMilli = millis();
          }
        } else if (key == "Play") {
          playRecordedSteps = valueInt;
        } else if (key == "Base") {
          writeServoValues(0, valueInt);
        } else if (key == "Shoulder") {
          writeServoValues(1, valueInt);
        } else if (key == "Elbow") {
          writeServoValues(2, valueInt);
        } else if (key == "Gripper") {
          writeServoValues(3, valueInt);
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
  setUpPinModes();
  Serial.begin(115200);
  while (!Serial) { ; } // รอให้ Serial port พร้อม
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
}