// ============================================
// Nodus AI — ESP32 LilyGO TTGO T-Call
// SMS via SIM800H  +  Internet via WiFi
// Server: Andasy.io Python (FastAPI)
// ============================================

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#define MODEM_RST      5
#define MODEM_PWKEY    4
#define MODEM_POWER_ON 23
#define MODEM_TX       27
#define MODEM_RX       26

HardwareSerial SerialAT(1);

// ─────────────────────────────────────────────
// ⚙️ CONFIG
// ─────────────────────────────────────────────
const char* WIFI_SSID     = "UHURU";
const char* WIFI_PASSWORD = "uStar@24";
const String SERVER_URL   = "https://nodus-ai.andasy.dev/process";

#define SMS_MAX_LEN   155   // 160 max - prefix overhead
#define HTTP_TIMEOUT  30000 // 30s for Gemini to respond

// ─────────────────────────────────────────────
// SMS BUFFER — handles messages during processing
// ─────────────────────────────────────────────
#define QUEUE_SIZE 5
struct SMSItem { String sender; String body; };
SMSItem smsQueue[QUEUE_SIZE];
int qHead = 0, qTail = 0;

bool enqueue(String sender, String body) {
  int next = (qTail + 1) % QUEUE_SIZE;
  if (next == qHead) return false; // full
  smsQueue[qTail] = {sender, body};
  qTail = next;
  return true;
}

bool dequeue(SMSItem &item) {
  if (qHead == qTail) return false; // empty
  item = smsQueue[qHead];
  qHead = (qHead + 1) % QUEUE_SIZE;
  return true;
}

// ─────────────────────────────────────────────
// WiFi
// ─────────────────────────────────────────────
void connectWiFi() {
  Serial.print("📶 Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 30) {
    delay(1000);
    Serial.print(".");
    tries++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi connected — IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\n❌ WiFi failed — restarting...");
    ESP.restart();
  }
}

// ─────────────────────────────────────────────
// AT HELPERS
// ─────────────────────────────────────────────
void sendAT(String cmd, int waitMs = 1000) {
  while (SerialAT.available()) SerialAT.read(); // flush
  SerialAT.println(cmd);
  delay(waitMs);
}

String readAT(int timeoutMs = 2000) {
  String resp = "";
  unsigned long t = millis();
  while (millis() - t < timeoutMs) {
    while (SerialAT.available()) {
      resp += (char)SerialAT.read();
    }
  }
  resp.trim();
  return resp;
}

// ─────────────────────────────────────────────
// SEND SMS — auto multi-part for long replies
// ─────────────────────────────────────────────
void sendSMS(String number, String message) {
  int total = message.length();
  int parts = (total + SMS_MAX_LEN - 1) / SMS_MAX_LEN;

  for (int i = 0; i < parts; i++) {
    String part = message.substring(i * SMS_MAX_LEN,
                  min((i + 1) * SMS_MAX_LEN, total));
    if (parts > 1) {
      part = "(" + String(i + 1) + "/" + String(parts) + ") " + part;
    }
    SerialAT.println("AT+CMGS=\"" + number + "\"");
    delay(500);
    SerialAT.print(part);
    delay(200);
    SerialAT.write(26); // Ctrl+Z
    delay(3500);
    Serial.println("📤 SMS part " + String(i + 1) + "/" + String(parts) + " sent");
  }
}

// ─────────────────────────────────────────────
// JSON ESCAPE
// ─────────────────────────────────────────────
String jsonEscape(String s) {
  s.replace("\\", "\\\\");
  s.replace("\"", "\\\"");
  s.replace("\n", " ");
  s.replace("\r", "");
  s.replace("\t", " ");
  return s;
}

// ─────────────────────────────────────────────
// ASK GEMINI via WiFi
// ─────────────────────────────────────────────
String askGemini(String sender, String message) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️ WiFi dropped — reconnecting...");
    connectWiFi();
  }

  String body = "{\"sender\":\"" + jsonEscape(sender) +
                "\",\"message\":\"" + jsonEscape(message) + "\"}";
  Serial.println("📦 POST: " + body);

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.begin(client, SERVER_URL);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(HTTP_TIMEOUT);

  int httpCode = http.POST(body);
  Serial.println("⚡ HTTP: " + String(httpCode));

  if (httpCode != 200) {
    http.end();
    return "Server error (" + String(httpCode) + "). Try again.";
  }

  String response = http.getString();
  http.end();
  Serial.println("📬 Response: " + response);

  // Parse {"reply":"..."}
  int start = response.indexOf("\"reply\":\"");
  if (start < 0) return "Could not read AI reply. Try again.";
  start += 9;
  // Handle escaped quotes inside reply
  String reply = "";
  for (int i = start; i < response.length(); i++) {
    if (response[i] == '\\' && i + 1 < response.length() && response[i+1] == '"') {
      reply += '"';
      i++;
    } else if (response[i] == '"') {
      break;
    } else {
      reply += response[i];
    }
  }
  reply.replace("\\n", "\n");
  reply.trim();
  return reply.length() > 0 ? reply : "Empty reply. Try again.";
}

// ─────────────────────────────────────────────
// PROCESS ONE SMS
// ─────────────────────────────────────────────
void processCommand(String sender, String msg) {
  msg.trim();
  Serial.println("──────────────────────────────");
  Serial.println("📥 From: " + sender);
  Serial.println("📥 Msg : " + msg);
  Serial.println("──────────────────────────────");

  String lower = msg;
  lower.toLowerCase();

  if (lower == "help" || lower == "?" || lower == "aide" || lower == "ubufasha") {
    sendSMS(sender, "Nodus AI: Ohereza ikibazo mu Kirundi, Igifaransa, Icongereza cyangwa Swahili. Andika STOP guhagarika.");
    return;
  }
  if (lower == "stop") {
    sendSMS(sender, "Unsubscribed. Send any message to reactivate.");
    return;
  }

  sendSMS(sender, "⏳ Thinking...");
  String reply = askGemini(sender, msg);
  sendSMS(sender, reply);
}

// ─────────────────────────────────────────────
// MODEM INIT
// ─────────────────────────────────────────────
void initModem() {
  pinMode(MODEM_PWKEY,    OUTPUT);
  pinMode(MODEM_RST,      OUTPUT);
  pinMode(MODEM_POWER_ON, OUTPUT);
  digitalWrite(MODEM_PWKEY,    LOW);
  digitalWrite(MODEM_RST,      HIGH);
  digitalWrite(MODEM_POWER_ON, HIGH);

  Serial.println("🔌 Powering modem...");
  delay(3000);
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  sendAT("AT");           Serial.println("AT:       " + readAT());
  sendAT("AT+CPIN?");     Serial.println("SIM:      " + readAT());
  sendAT("AT+CFUN=1", 2000);
  sendAT("AT+CNMP=13");   // GSM only
  sendAT("AT+CSCS=\"GSM\""); // Fix encoding for special chars
  sendAT("AT+COPS?");     Serial.println("Operator: " + readAT());

  Serial.print("📶 Waiting for GSM network");
  for (int i = 0; i < 40; i++) {
    sendAT("AT+CREG?", 500);
    String resp = readAT(800);
    Serial.print(".");
    if (resp.indexOf(",1") >= 0 || resp.indexOf(",5") >= 0) {
      Serial.println(" ✅");
      break;
    }
    delay(1500);
  }

  sendAT("AT+CSQ");       Serial.println("Signal:   " + readAT());
  sendAT("AT+CMGF=1");                  // Text mode
  sendAT("AT+CNMI=1,2,0,0,0");         // Push new SMS to serial
  sendAT("AT+CMGDA=\"DEL ALL\"", 3000); // Clear old messages
  Serial.println("\n🚀 MODEM READY");
}

// ─────────────────────────────────────────────
// SETUP
// ─────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n====================================");
  Serial.println("   Nodus AI — WiFi + SMS Bridge");
  Serial.println("====================================\n");
  connectWiFi();
  initModem();
  Serial.println("\n✅ All systems ready!");
  Serial.println("🌍 Server: " + SERVER_URL);
}

// ─────────────────────────────────────────────
// LOOP — non-blocking SMS reader with queue
// ─────────────────────────────────────────────
void loop() {
  // Keep WiFi alive
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️ WiFi lost — reconnecting...");
    connectWiFi();
  }

  // Read incoming AT lines into queue
  while (SerialAT.available()) {
    String line = SerialAT.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;
    Serial.println("[AT] " + line);

    if (line.startsWith("+CMT:")) {
      // Extract sender from: +CMT: "+25078xxx","","date"
      int q1 = line.indexOf('"') + 1;
      int q2 = line.indexOf('"', q1);
      String sender = line.substring(q1, q2);

      // Read message body (next line)
      String body = "";
      unsigned long t = millis();
      while (millis() - t < 3000) { // increased to 3s
        if (SerialAT.available()) {
          body = SerialAT.readStringUntil('\n');
          body.trim();
          break;
        }
        delay(10);
      }

      if (body.length() > 0) {
        if (!enqueue(sender, body)) {
          Serial.println("⚠️ Queue full — dropping message from " + sender);
          sendSMS(sender, "Busy. Please try again in a moment.");
        }
      }
    }
  }

  // Process one queued SMS per loop
  SMSItem item;
  if (dequeue(item)) {
    processCommand(item.sender, item.body);
  }
}
