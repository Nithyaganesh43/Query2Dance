CREATE DATABASE toys_db;
USE toys_db;

CREATE TABLE human (
  id INT PRIMARY KEY,
  name VARCHAR(50),
  age INT,
  gender ENUM('male','female'),
  vehical INT,
  favanimal INT,
  favfood VARCHAR(50)
);

CREATE TABLE vehical (
  id INT PRIMARY KEY,
  name VARCHAR(50),
  owner INT,
  nowheel INT,
  type ENUM('car','bike','bus'),
  number VARCHAR(20)
);

CREATE TABLE animal (
  id INT PRIMARY KEY,
  name VARCHAR(50),
  food VARCHAR(50),
  color VARCHAR(20)
);

INSERT INTO human (id, name, age, gender, vehical, favanimal, favfood) VALUES
(1, 'priya', 20, 'female', 4, 7, 'pizza'),
(2, 'abi', 21, 'female', 5, 8, 'burger'),
(3, 'sana', 22, 'female', 6, 9, 'pasta'),
(4, 'saro', 23, 'male', 4, 7, 'noodles');

INSERT INTO vehical (id, name, owner, nowheel, type, number) VALUES
(5, 'car', 1, 4, 'car', 'TN01AB1234'),
(6, 'bike', 2, 2, 'bike', 'TN02CD5678'),
(7, 'bus', 3, 4, 'bus', 'TN03EF9876');

INSERT INTO animal (id, name, food, color) VALUES
(8, 'cow', 'grass', 'white'),
(9, 'deer', 'leaves', 'brown'),
(10, 'raino', 'plants', 'grey');
