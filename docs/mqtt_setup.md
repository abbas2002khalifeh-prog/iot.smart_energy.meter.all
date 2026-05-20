# 📡 MQTT Setup & Communication

This project uses MQTT (Message Queuing Telemetry Transport) for real-time communication between the ESP32 smart energy meter and the backend server.

---

# 📌 What is MQTT?

MQTT is a lightweight publish/subscribe messaging protocol designed for IoT applications.

In this project:

ESP32 publishes live electrical measurements  
while the backend subscribes to receive and process data.

---

# 🏗 MQTT Communication Flow

ESP32  
→ MQTT Broker (Mosquitto)  
→ Node.js Backend  
→ MySQL Database  
→ React Dashboard

---

# 🛠 MQTT Broker Used

This project uses:

- Mosquitto MQTT Broker

Official Website:

🔗 https://mosquitto.org/

---

# 📥 Installing Mosquitto

## Windows Installation

Download Mosquitto:

🔗 https://mosquitto.org/download/

Install with default settings.

---

# ▶ Starting MQTT Broker

Open CMD and run:

mosquitto -v

Expected output:

Opening ipv4 listen socket on port 1883

---

# 📡 MQTT Topics
Live Energy Data
energy/meter/live

Used for:

voltage
current
power
energy
cost
alert status
Device Status
energy/meter/status

Used for:

online/offline device monitoring

---
# 📤 ESP32 MQTT Publishing

ESP32 publishes JSON payloads such as:

{
  "ts": 1779091179774,
  "v_rms": 228.54,
  "i_rms": 0.42,
  "power_w": 95.6,
  "energy_kwh": 0.0123,
  "cost_usd": 0.0013,
  "alert": 0
}

---

# 📥 Backend MQTT Subscription

The backend subscribes to:

energy/meter/live

and:

stores readings into MySQL
exposes REST APIs
forwards data to dashboard

---

# 🧪 Testing MQTT
Subscribe to Topic
mosquitto_sub -h localhost -t energy/meter/live -v
Publish Test Message
mosquitto_pub -h localhost -t energy/meter/live -m "test"
⚠ Important Notes
ESP32 and backend must be on the same network
MQTT broker port:
1883
Windows Firewall may block MQTT communication
ESP32 cannot use:
localhost

as broker address

Use the laptop local IP instead.

Example:

const char* MQTT_BROKER = "192.168.1.7";

---

# 📚 Libraries Used
ESP32
PubSubClient
ArduinoJson
Backend
mqtt.js

---

# 🚀 Advantages of MQTT in this Project
Lightweight communication
Real-time updates
Low network overhead
Fast sensor-to-dashboard transmission
Scalable IoT architecture

---

# 👨‍💻 Author

Abbas Khalifeh

Computer & Communication Engineering
Lebanese International University