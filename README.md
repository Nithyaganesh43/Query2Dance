# Query2Dance

# Query2Dance

Query2Dance is a fun **DBMS + IoT project** that links database queries to real-world actions. When a SQL query returns rows from a predefined table of dolls, vehicles, or animals, the system converts the result into a **10-bit bitstring** and sends it to an ESP8266-based controller. Each `1` in the bitstring triggers a corresponding doll to "dance" via a servo motor.

---

## Block Diagram

```text
+-------------+       +------------------+       +----------------+       +------------------+
|   Frontend  | ----> | Express Backend  | ----> | ESP8266 Server | ----> | Servo Controller |
|  (Query UI) |       |  (Node + MySQL) |       | (Port 80 API)  |       | (PCA9685 + Dolls)|
+-------------+       +------------------+       +----------------+       +------------------+
        |                      |                         |
        |                      |                         |
        |      MySQL Query     |                         |
        | -------------------> |                         |
        |                      |                         |
        | <------------------- |                         |
        |    Query Results + Bitstring                   |

```

## Flow

User enters a SQL query in frontend.

Backend executes query on MySQL database.

Backend converts result IDs → 10-bit bitstring.

Bitstring sent to ESP via HTTP POST /setcmd.

ESP maps 1 bits to servo channels and triggers motion.

2. Backend Setup
cd backend
npm install
node index.js

3. ESP Setup

Open esp/index.c in Arduino IDE.

Update ssid and password.

Flash to ESP8266.

4. Frontend

Just open frontend/index.html in a browser.

Folder Structure
DBMS-kolu/
 ├── backend/        # Node.js server
 ├── database/       # SQL schema & sample data
 ├── frontend/       # Simple UI to send queries
 └── esp/            # ESP8266 code

Example

Query:

SELECT * FROM human WHERE gender='female';


Response → Bitstring:

"1100000000" → Dolls 1 & 2 Dance
