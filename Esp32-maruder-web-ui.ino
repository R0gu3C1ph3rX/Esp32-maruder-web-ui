#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <esp_wifi.h>
#include <ArduinoJson.h>

// ===== CONFIG =====
#define AP_SSID "Marauder-UI"
#define AP_PASS "12345678"

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// ====== Embedded Dark Web UI Page ======
const char mainPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<title>ESP32 Marauder - Web UI</title>
<style>
body { background-color: #121212; color: #eee; font-family: Arial, sans-serif; margin: 0;}
.nav { background: #1f1f1f; padding: 10px; display: flex; gap: 10px; }
button { padding: 10px; border: none; cursor: pointer; background: #333; color: #fff; }
button:hover { background: #555; }
h2 { margin: 15px; }
.table-container { padding: 15px; }
table { border-collapse: collapse; width: 100%; }
th, td { border: 1px solid #333; padding: 8px; text-align: left; }
th { background-color: #222; }
.log { padding: 15px; white-space: pre-wrap; background: #000; margin: 15px; height: 200px; overflow-y: scroll; border: 1px solid #333;}
</style>
</head>
<body>
<div class="nav">
  <button onclick="sendCmd('scan')">Scan</button>
  <button onclick="sendCmd('deauth_start')">Deauth Start</button>
  <button onclick="sendCmd('deauth_stop')">Deauth Stop</button>
  <button onclick="sendCmd('beacon_start')">Beacon Start</button>
  <button onclick="sendCmd('beacon_stop')">Beacon Stop</button>
</div>

<h2>Scan Results</h2>
<div class="table-container">
<table id="scanTable">
<tr><th>SSID</th><th>BSSID</th><th>Channel</th><th>RSSI</th><th>Encryption</th></tr>
</table>
</div>

<h2>Logs</h2>
<div class="log" id="logBox"></div>

<script>
let ws=new WebSocket(`ws://${location.host}/ws`);
ws.onmessage=e=>{
  try {
    let data=JSON.parse(e.data);
    if(data.type==="scan"){
      let table=document.getElementById('scanTable');
      table.innerHTML="<tr><th>SSID</th><th>BSSID</th><th>Channel</th><th>RSSI</th><th>Encryption</th></tr>";
      data.results.forEach(r=>{
        table.innerHTML+=`<tr><td>${r.ssid}</td><td>${r.bssid}</td><td>${r.channel}</td><td>${r.rssi}</td><td>${r.enc}</td></tr>`;
      });
    } else if(data.type==="log"){
      document.getElementById('logBox').textContent += data.message + "\\n";
    }
  }catch(err){console.log("Non-JSON Msg", e.data);}
};
function sendCmd(cmd){ ws.send(cmd); }
</script>
</body>
</html>
)rawliteral";

// ===== Utility to Send Log to UI =====
void sendLog(String msg) {
  DynamicJsonDocument doc(256);
  doc["type"] = "log";
  doc["message"] = msg;
  String out;
  serializeJson(doc, out);
  ws.textAll(out);
}

// ===== Wi-Fi Scan Logic =====
void doScan() {
  int n = WiFi.scanNetworks();
  DynamicJsonDocument doc(2048);
  doc["type"]="scan";
  JsonArray arr = doc.createNestedArray("results");
  for(int i=0;i<n;i++){
    JsonObject obj = arr.createNestedObject();
    obj["ssid"] = WiFi.SSID(i);
    obj["bssid"] = WiFi.BSSIDstr(i);
    obj["channel"] = WiFi.channel(i);
    obj["rssi"] = WiFi.RSSI(i);
    obj["enc"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Secured";
  }
  String out;
  serializeJson(doc, out);
  ws.textAll(out);
}

// ===== Attack Placeholders =====
void startDeauth() { sendLog("[DEAUTH] Attack started"); /* Insert Marauder code here */ }
void stopDeauth()  { sendLog("[DEAUTH] Attack stopped"); }
void startBeacon() { sendLog("[BEACON] Spam started"); }
void stopBeacon()  { sendLog("[BEACON] Spam stopped"); }

// ===== WebSocket Events =====
void onWSMessage(void * arg, AsyncWebSocket * server, AsyncWebSocketClient * client,
                 AwsEventType type, void * data, size_t len) {
  if(type == WS_EVT_DATA) {
    String cmd = String((char*)data).substring(0, len);
    if(cmd == "scan") doScan();
    else if(cmd == "deauth_start") startDeauth();
    else if(cmd == "deauth_stop") stopDeauth();
    else if(cmd == "beacon_start") startBeacon();
    else if(cmd == "beacon_stop") stopBeacon();
  }
}

// ====== Setup ======
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", mainPage);
  });

  ws.onEvent(onWSMessage);
  server.addHandler(&ws);
  server.begin();

  sendLog("[SYSTEM] Marauder Web UI ready.");
}

// ===== Loop =====
void loop() {
  // Background attack/scan handling can go here
}
