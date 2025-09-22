const express = require('express');
const mysql = require('mysql2/promise');
const axios = require('axios');

const app = express();
app.use(express.json());

const API_KEY = 'MY_SECRET_KEY';
const ESP_IP = 'http://192.168.1.100';

const cors = require('cors');
app.use(cors());

const db = mysql.createPool({
  host: 'localhost',
  user: 'root',
  password: 'Nithyaganesh@123',
  database: 'toys_db',
});

const dollOrder = ['1', '2', '3', '4', '5', '6', '7', '8', '9', '10'];

app.post('/query', async (req, res) => {
  const { query, API_KEY: clientKey } = req.body;
  if (clientKey !== API_KEY)
    return res.status(401).json({ error: 'Invalid key' });

  try {
 const [rows] = await db.execute(query);

 const ids = rows.map((row) => String(row.id));
 const bitstring = dollOrder
   .map((id) => (ids.includes(id) ? '1' : '0'))
   .join('');

 res.json({ data: rows, count: rows.length, bitstring });

    setImmediate(async () => {
      try {
        await axios.post(`${ESP_IP}/setcmd/${bitstring}`);
      } catch (err) {
        console.error('ESP call failed:', err.message, ' for : ', bitstring);
      }
    });
  } catch (err) {
    res.status(400).json({ error: err.message });
  }
});

app.listen(3000, () => console.log('Server running on port 3000'));


// CREATE DATABASE toys_db;
// USE toys_db;

// CREATE TABLE human (
//   id INT PRIMARY KEY,
//   name VARCHAR(50),
//   age INT,
//   gender ENUM('male','female'),
//   vehical INT,
//   favanimal INT,
//   favfood VARCHAR(50)
// );

// CREATE TABLE vehical (
//   id INT PRIMARY KEY,
//   name VARCHAR(50),
//   owner INT,
//   nowheel INT,
//   type ENUM('car','bike','bus'),
//   number VARCHAR(20)
// );

// CREATE TABLE animal (
//   id INT PRIMARY KEY,
//   name VARCHAR(50),
//   food VARCHAR(50),
//   color VARCHAR(20)
// );

// INSERT INTO human (id, name, age, gender, vehical, favanimal, favfood) VALUES
// (1, 'priya', 20, 'female', 4, 7, 'pizza'),
// (2, 'abi', 21, 'female', 5, 8, 'burger'),
// (3, 'sana', 22, 'female', 6, 9, 'pasta'),
// (4, 'saro', 23, 'male', 4, 7, 'noodles');

// INSERT INTO vehical (id, name, owner, nowheel, type, number) VALUES
// (5, 'car', 1, 4, 'car', 'TN01AB1234'),
// (6, 'bike', 2, 2, 'bike', 'TN02CD5678'),
// (7, 'bus', 3, 4, 'bus', 'TN03EF9876');

// INSERT INTO animal (id, name, food, color) VALUES
// (8, 'cow', 'grass', 'white'),
// (9, 'deer', 'leaves', 'brown'),
// (10, 'raino', 'plants', 'grey');

 

// The core idea of this system is to connect a fixed database of dolls, vehicles,
//  and animals to a backend server that dynamically interprets queries and translates 
// them into real-world actions. The database acts as a structured repository of all
//  entities and their attributes, while the Express backend serves as the bridge between 
// data and physical devices. When a frontend sends a query for specific dolls, the backend 
// executes it, generates a corresponding bitstring representing which dolls should act,
//  and immediately triggers the ESP-controlled servos asynchronously. This design allows 
// real-time physical interaction based on database queries, keeping the backend responsive,
//  ensuring frontend stability, and integrating the virtual data world with tangible robotic actions.