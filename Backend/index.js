// server.js
const express = require('express');
const cors = require('cors');
const sqlite3 = require('sqlite3').verbose();
const http = require('http');
const WebSocket = require('ws');

const app = express();
app.use(express.json());
app.use(cors());

// Config
const PORT = 3000;
const API_KEY = 'MY_SECRET_KEY';
const DB_PATH = './Backend/toys.db';

// -----------------------------------------------------------------------------
// Database Setup (SQLite)
// -----------------------------------------------------------------------------
const db = new sqlite3.Database(DB_PATH);

function initDatabase() {
  return new Promise((resolve, reject) => {
    db.serialize(() => {
      // 1. Reset tables (Drop in correct order)
      db.run("PRAGMA foreign_keys = OFF;"); 
      db.run("DROP TABLE IF EXISTS human");
      db.run("DROP TABLE IF EXISTS vehicle");
      db.run("DROP TABLE IF EXISTS animal");
      db.run("DROP TABLE IF EXISTS food");
      db.run("DROP TABLE IF EXISTS light");
      
      // 2. Create Tables & Insert Data
      
      // Light
      db.run("CREATE TABLE light (id INTEGER PRIMARY KEY, name TEXT NOT NULL)");
      const stmtLight = db.prepare("INSERT INTO light (id, name) VALUES (?, ?)");
      [[11, 'light 1'], [12, 'light 2']].forEach(r => stmtLight.run(r));
      stmtLight.finalize();

      // Food
      db.run("CREATE TABLE food (id INTEGER PRIMARY KEY, name TEXT UNIQUE NOT NULL)");
      const stmtFood = db.prepare("INSERT INTO food (id, name) VALUES (?, ?)");
      [[101, 'pizza'], [102, 'burger'], [103, 'pasta'], [104, 'noodles']].forEach(r => stmtFood.run(r));
      stmtFood.finalize();

      // Animal
      db.run("CREATE TABLE animal (id INTEGER PRIMARY KEY, name TEXT UNIQUE NOT NULL, food TEXT NOT NULL, color TEXT NOT NULL)");
      const stmtAnimal = db.prepare("INSERT INTO animal (id, name, food, color) VALUES (?, ?, ?, ?)");
      [[8, 'cow', 'grass', 'white'], [6, 'deer', 'leaves', 'brown'], [7, 'rhino', 'plants', 'grey']].forEach(r => stmtAnimal.run(r));
      stmtAnimal.finalize();

      // Human
      db.run(`CREATE TABLE human (
        id INTEGER PRIMARY KEY,
        name TEXT NOT NULL,
        age INTEGER NOT NULL,
        gender TEXT NOT NULL,
        fav_food_id INTEGER,
        fav_animal_id INTEGER,
        vehicle_id INTEGER,
        FOREIGN KEY (fav_food_id) REFERENCES food(id),
        FOREIGN KEY (fav_animal_id) REFERENCES animal(id)
      )`);
      const stmtHuman = db.prepare("INSERT INTO human (id, name, age, gender, fav_food_id, fav_animal_id, vehicle_id) VALUES (?, ?, ?, ?, ?, ?, ?)");
      // Note: vehicle_id is null initially
      [[3, 'priya', 20, 'female', 101, 6, null],
       [5, 'abi',   21, 'female', 102, 6, null],
       [10,'sana',  22, 'female', 103, 7, null],
       [9, 'saro',  23, 'male',   104, 7, null]].forEach(r => stmtHuman.run(r));
      stmtHuman.finalize();

      // Vehicle
      db.run(`CREATE TABLE vehicle (
        id INTEGER PRIMARY KEY,
        name TEXT NOT NULL,
        owner_id INTEGER,
        wheel_count INTEGER NOT NULL,
        type TEXT NOT NULL,
        number TEXT UNIQUE NOT NULL,
        FOREIGN KEY (owner_id) REFERENCES human(id)
      )`);
      const stmtVehicle = db.prepare("INSERT INTO vehicle (id, name, owner_id, wheel_count, type, number) VALUES (?, ?, ?, ?, ?, ?)");
      [[2, 'car',  5,  4, 'car',  'TN01AB1234'],
       [1, 'bike', 3,  2, 'bike', 'TN02CB5678'],
       [4, 'bus',  10, 6, 'bus',  'TN03EF9876']].forEach(r => stmtVehicle.run(r));
      stmtVehicle.finalize();

      // 3. Update circular references
      db.run("UPDATE human SET vehicle_id = 2 WHERE id = 5");
      db.run("UPDATE human SET vehicle_id = 1 WHERE id = 3");
      db.run("UPDATE human SET vehicle_id = 4 WHERE id = 10");

      console.log('Database initialized and seeded.');
      resolve();
    });
  });
}

// -----------------------------------------------------------------------------
// WebSocket & HTTP Server
// -----------------------------------------------------------------------------
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

let espSocket = null; // Store the ESP connection

wss.on('connection', (ws, req) => {
  console.log(`New WS Client connected: ${req.socket.remoteAddress}`);
  
  // Assuming the ESP is the only client or we treat the last connected client as ESP for simplicity
  espSocket = ws;

  ws.on('message', (message) => {
    console.log(`Received from ESP: ${message}`);
    // Keep-alive or ack logic if needed
  });

  ws.on('close', () => {
    console.log('WS Client disconnected');
    if (espSocket === ws) espSocket = null;
  });

  ws.on('error', (err) => {
    console.error('WS error:', err.message);
  });
  
  // Send a ping/hello to confirm connection
  ws.send(JSON.stringify({ type: 'status', msg: 'Connected to Server' }));
});

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------
const dollOrder = ['1', '2', '3', '4', '5', '6', '7', '8', '9', '10'];

function buildBitstring(rows) {
  const ids = rows.map((r) => String(r.id));
  return dollOrder.map((id) => (ids.includes(id) ? '1' : '0')).join('');
}

function hasLight(rows, name) {
  const target = name.toLowerCase();
  return rows.some((r) => String(r.name || '').toLowerCase() === target);
}

// -----------------------------------------------------------------------------
// API Routes
// -----------------------------------------------------------------------------
app.post('/query', async (req, res) => {
  const { query, API_KEY: clientKey } = req.body;
  if (clientKey !== API_KEY) {
    return res.status(401).json({ error: 'Invalid key' });
  }
  if (!query || typeof query !== 'string') {
    return res.status(400).json({ error: 'Missing SQL query' });
  }

  // Execute query against SQLite
  db.all(query, [], (err, rows) => {
    if (err) {
      console.error('SQL Error:', err.message);
      return res.status(400).json({ error: err.message });
    }
    
    // Process results
    const bitstring = buildBitstring(rows || []);
    const count = rows ? rows.length : 0;
    
    // 1. Respond to Frontend immediately (unchanged contract)
    res.json({ data: rows, count, bitstring });

    // 2. Send command to ESP via WebSocket
    if (espSocket && espSocket.readyState === WebSocket.OPEN) {
      const light1 = hasLight(rows || [], 'light 1');
      const light2 = hasLight(rows || [], 'light 2');

      const payload = {
        type: 'cmd',
        bitstring: bitstring,
        light1: light1,
        light2: light2
      };

      try {
        espSocket.send(JSON.stringify(payload));
        console.log('Sent to ESP:', JSON.stringify(payload));
      } catch (e) {
        console.error('WS Send error:', e.message);
      }
    } else {
      console.warn('ESP not connected or socket closed. Command not sent.');
    }
  });
});

app.get('/health', (_req, res) => res.json({ ok: true }));

// -----------------------------------------------------------------------------
// Start Server
// -----------------------------------------------------------------------------
initDatabase().then(() => {
  server.listen(PORT, () => {
    console.log(`Server running on port ${PORT}`);
    console.log(`WebSocket server ready on ws://localhost:${PORT}`);
  });
});
