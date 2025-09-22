# Query2Dance

Query2Dance is a **DBMS + IoT project** that links SQL query results to real-world actions. When a query matches rows in a predefined table, the backend converts matching IDs into a **10-bit bitstring** and sends it directly to an **ESP32** via HTTP. Each `1` in the bitstring triggers a corresponding doll to "dance" through a servo motor.

---

## Block Diagram

```text
+-------------+       +------------------+       +--------------+       +------------------+
|   Frontend  | ----> | Express Backend  | ----> |   ESP32      | ----> | Servo Controller |
|  (Query UI) |       |  (Node + MySQL) |       | (Port 80 API)|       | (PCA9685 + Dolls)|
+-------------+       +------------------+       +--------------+       +------------------+
        |                      |                         |
        |      MySQL Query     |                         |
        | -------------------> |                         |
        |                      |                         |
        | <------------------- |                         |
        |    Query Results + Bitstring                   |
```

## Flow

User enters SQL query in the frontend.

Backend executes query on MySQL database.

Backend converts result IDs → 10-bit bitstring.

Bitstring is sent directly to ESP32 (POST /setcmd).

ESP32 maps 1 bits to servo channels and triggers motion.

Setup
1. Backend
cd backend
npm install
node index.js

2. ESP32

Open esp/index.c in Arduino IDE

Update Wi-Fi SSID & Password

Flash to ESP32

3. Frontend

Just open frontend/index.html in a browser.

Folder Structure
DBMS-kolu/
 ├── backend/        # Node.js server
 ├── database/       # SQL schema & sample data
 ├── frontend/       # Query UI
 └── esp/            # ESP32 code

Example

Query:

SELECT * FROM human WHERE gender='female';


Response → Bitstring:

"1100000000" → Dolls 1 & 2 Dance