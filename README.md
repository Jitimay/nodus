# Nodus AI 🌍

> AI for everyone — no internet required. Just send an SMS.

Nodus AI lets anyone with a basic phone access Google Gemini via SMS. No smartphone, no data plan, no app needed. Just text a question and get an AI-powered reply.

---

## How It Works

```
User SMS → ESP32 (SIM800H) → WiFi → Andasy Server → Gemini API → SMS Reply
```

1. User sends an SMS to the Godus AI number
2. The ESP32 reads it via AT commands
3. ESP32 posts it to the FastAPI server on Andasy.io over WiFi
4. Server calls Google Gemini and gets a response
5. Server returns the reply to ESP32
6. ESP32 sends the reply back as SMS to the user

---

## System Components

### 📱 User Side
- Any phone (feature phone or smartphone)
- No internet or app required
- Supports: Kirundi, French, English, Swahili

### 📡 Hardware Gateway — `NodusAI.ino`
- **Board**: LilyGO TTGO T-Call ESP32 SIM800H
- Receives and sends SMS via SIM800H modem (AT commands)
- Connects to internet via WiFi
- Queues up to 5 incoming messages during processing
- Auto splits long AI replies into multi-part SMS

### 🧠 AI Server — `main.py`
- **Framework**: FastAPI (Python)
- **Hosted on**: [Andasy.io](https://andasy.io)
- **AI Model**: Google Gemini 2.5 Flash
- Endpoint: `POST /process` — receives `{sender, message}`, returns `{reply}`

---

## Project Structure

```
├── main.py          # FastAPI server (deployed on Andasy.io)
├── NodusAI.ino      # ESP32 Arduino firmware
├── Dockerfile       # Container config for Andasy deployment
├── requirements.txt # Python dependencies
└── README.md
```

---

## Server Setup (Andasy.io)

```bash
# Install Andasy CLI
curl -fsSL https://andasy.io/install.sh | bash

# Login
andasy auth login

# Setup and deploy
andasy setup
andasy deploy
```

Add your Gemini API key in the Andasy dashboard under **Secrets**:
```
GEMINI_API_KEY = your_key_here
```

Get a free Gemini API key at [aistudio.google.com](https://aistudio.google.com).

---

## ESP32 Setup

1. Open `GodusAI.ino` in Arduino IDE
2. Edit the config section:
```cpp
const char* WIFI_SSID     = "your_wifi";
const char* WIFI_PASSWORD = "your_password";
const String SERVER_URL   = "https://nodus-ai.andasy.dev/process";
```
3. Install required libraries:
   - `WiFi` (built-in)
   - `WiFiClientSecure` (built-in)
   - `HTTPClient` (built-in)
4. Flash to your LilyGO TTGO T-Call board

---

## SMS Commands

| Message | Response |
|---------|----------|
| Any question | AI-generated answer |
| `help` / `aide` / `ubufasha` / `?` | Usage instructions |
| `stop` | Unsubscribe message |

---

## API Endpoint

```
POST https://nodus-ai.andasy.dev/process
Content-Type: application/json

{ "sender": "+25078xxxxxxx", "message": "Explain photosynthesis" }
```

Response:
```json
{ "reply": "Photosynthesis is the process plants use to make food..." }
```

---

## Built With

- [FastAPI](https://fastapi.tiangolo.com) — Python web framework
- [Google Gemini API](https://aistudio.google.com) — AI model
- [Andasy.io](https://andasy.io) — Cloud deployment (Rwanda)
- [LilyGO TTGO T-Call](https://github.com/Xinyuan-LilyGO/LilyGO-T-Call-SIM800) — ESP32 + SIM800H board

---

## License

MIT — built for Africa, by Africa. 🌍
