const express = require('express');
const cors = require('cors');
const sqlite3 = require('sqlite3').verbose();
const http = require('http');
const WebSocket = require('ws');

const app = express();
app.use(express.json());
app.use(cors());
app.set('trust proxy', true);

// ================= CONFIG =================
const PORT = process.env.PORT || 3000;
const API_KEY = 'MY_SECRET_KEY';
const DB_PATH = './toys.db';

// ================= DATABASE =================
const db = new sqlite3.Database(DB_PATH);

function initDatabase() {
  return new Promise(resolve => {
    db.serialize(() => {
      db.run("PRAGMA foreign_keys=OFF;");
      db.run("DROP TABLE IF EXISTS human");
      db.run("DROP TABLE IF EXISTS vehicle");
      db.run("DROP TABLE IF EXISTS animal");
      db.run("DROP TABLE IF EXISTS food");
      db.run("DROP TABLE IF EXISTS light");

      db.run("CREATE TABLE light(id INTEGER PRIMARY KEY,name TEXT NOT NULL)");
      const light = db.prepare("INSERT INTO light VALUES(?,?)");
      [[11, 'light 1'], [12, 'light 2']].forEach(r => light.run(r));
      light.finalize();

      db.run("CREATE TABLE food(id INTEGER PRIMARY KEY,name TEXT UNIQUE NOT NULL)");
      const food = db.prepare("INSERT INTO food VALUES(?,?)");
      [[101, 'pizza'], [102, 'burger'], [103, 'pasta'], [104, 'noodles']].forEach(r => food.run(r));
      food.finalize();

      db.run("CREATE TABLE animal(id INTEGER PRIMARY KEY,name TEXT UNIQUE,food TEXT,color TEXT)");
      const animal = db.prepare("INSERT INTO animal VALUES(?,?,?,?)");
      [[8, 'cow', 'grass', 'white'], [6, 'deer', 'leaves', 'brown'], [7, 'rhino', 'plants', 'grey']].forEach(r => animal.run(r));
      animal.finalize();

      db.run(`CREATE TABLE human(
      id INTEGER PRIMARY KEY,
      name TEXT,
      age INTEGER,
      gender TEXT,
      fav_food_id INTEGER,
      fav_animal_id INTEGER,
      vehicle_id INTEGER
     )`);
      const human = db.prepare("INSERT INTO human VALUES(?,?,?,?,?,?,?)");
      [
        [3, 'priya', 20, 'female', 101, 6, null],
        [5, 'abi', 21, 'female', 102, 6, null],
        [10, 'sana', 22, 'female', 103, 7, null],
        [9, 'saro', 23, 'male', 104, 7, null]
      ].forEach(r => human.run(r));
      human.finalize();

      db.run(`CREATE TABLE vehicle(
      id INTEGER PRIMARY KEY,
      name TEXT,
      owner_id INTEGER,
      wheel_count INTEGER,
      type TEXT,
      number TEXT UNIQUE
     )`);
      const vehicle = db.prepare("INSERT INTO vehicle VALUES(?,?,?,?,?,?)");
      [
        [2, 'car', 5, 4, 'car', 'TN01AB1234'],
        [1, 'bike', 3, 2, 'bike', 'TN02CB5678'],
        [4, 'bus', 10, 6, 'bus', 'TN03EF9876']
      ].forEach(r => vehicle.run(r));
      vehicle.finalize();

      db.run("UPDATE human SET vehicle_id=2 WHERE id=5");
      db.run("UPDATE human SET vehicle_id=1 WHERE id=3");
      db.run("UPDATE human SET vehicle_id=4 WHERE id=10");

      console.log("Database ready");
      resolve();
    });
  });
}

// ================= SERVER =================
const server = http.createServer(app);

// 1️⃣ Prevent Node from closing idle sockets (Critical for Render/Proxies)
server.keepAliveTimeout = 65000;
server.headersTimeout = 66000;

// ⭐ attach websocket on /ws path
// 3️⃣ Disable per-message compression for stability
const wss = new WebSocket.Server({
  server,
  path: '/ws',
  perMessageDeflate: false
});

let espSocket = null;

// 2️⃣ Add WebSocket ping/pong keep-alive
wss.on('connection', (ws, req) => {
  console.log("WS connected:", req.socket.remoteAddress);

  espSocket = ws;
  ws.isAlive = true;

  ws.on('pong', () => { ws.isAlive = true; });

  ws.on('message', msg => {
    console.log("ESP:", msg.toString());
  });

  const interval = setInterval(() => {
    if (ws.isAlive === false) {
      console.log("Terminating dead socket");
      return ws.terminate();
    }
    ws.isAlive = false;
    ws.ping();
  }, 20000);   // ← important (20s keeps NAT & proxy alive)

  ws.on('close', () => {
    console.log("WS disconnected");
    clearInterval(interval);
    if (espSocket === ws) espSocket = null;
  });

  ws.on('error', err => {
    console.error("WS error:", err.message);
  });

  ws.send(JSON.stringify({ type: 'status', msg: 'Connected' }));
});

// ================= HELPERS =================
const dollOrder = ['1', '2', '3', '4', '5', '6', '7', '8', '9', '10'];

function buildBitstring(rows) {
  const ids = rows.map(r => String(r.id));
  return dollOrder.map(id => ids.includes(id) ? '1' : '0').join('');
}

function hasLight(rows, name) {
  return rows.some(r => String(r.name || '').toLowerCase() === name.toLowerCase());
}

// ================= API =================
app.post('/query', (req, res) => {
  const { query, API_KEY: clientKey } = req.body;

  if (clientKey !== API_KEY)
    return res.status(401).json({ error: 'Invalid key' });

  if (!query)
    return res.status(400).json({ error: 'Missing query' });

  db.all(query, [], (err, rows) => {
    if (err) return res.status(400).json({ error: err.message });

    const bitstring = buildBitstring(rows || []);
    const count = rows.length;

    res.json({ data: rows, count, bitstring });

    if (espSocket && espSocket.readyState === WebSocket.OPEN) {
      const payload = {
        type: 'cmd',
        bitstring,
        light1: hasLight(rows, 'light 1'),
        light2: hasLight(rows, 'light 2')
      };

      espSocket.send(JSON.stringify(payload));
      console.log("Sent to ESP:", payload);
    } else {
      console.log("ESP not connected");
    }
  });
});

// 4️⃣ Prevent Render sleep (external step)
app.get('/health', (_, res) => res.json({ ok: true }));

// ================= START =================
initDatabase().then(() => {
  server.listen(PORT, () => {
    console.log("Server running on port", PORT);
    console.log("WebSocket ready at /ws");
  });
});