const WebSocket=require('ws');

const SERVER_URL='wss://query2dance.onrender.com/ws';

let ws;

function connect(){
 ws=new WebSocket(SERVER_URL);

 ws.on('open',()=>{
  console.log('✅ connected');

  setInterval(()=>{
   if(ws.readyState===WebSocket.OPEN){
    ws.send(JSON.stringify({type:'heartbeat'}));
   }
  },10000);
 });

 ws.on('message',data=>{
  const msg=JSON.parse(data);
  console.log('📥',msg);
 });

 ws.on('close',()=>{
  console.log('reconnecting...');
  setTimeout(connect,3000);
 });

 ws.on('error',err=>{
  console.log('error:',err.message);
 });
}

connect();
