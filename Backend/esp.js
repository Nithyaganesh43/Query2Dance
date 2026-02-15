const WebSocket = require('ws');

// change if server runs elsewhere
const SERVER_URL = 'ws://localhost:3000';

let ws;

function connect() {
  ws = new WebSocket(SERVER_URL);

  ws.on('open', () => {
    console.log('✅ Mock ESP connected to server');

    // send heartbeat every 10s (optional)
    setInterval(() => {
      if (ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({ type: 'heartbeat', ts: Date.now() }));
      }
    }, 10000);
  });

  ws.on('message', (data) => {
    try {
      const msg = JSON.parse(data);
      console.log('📥 Command from server:', msg);

      if (msg.type === 'cmd') {
        console.log('🎯 Bitstring:', msg.bitstring);
        console.log('💡 Light1:', msg.light1);
        console.log('💡 Light2:', msg.light2);
      }

    } catch (e) {
      console.log('Raw message:', data.toString());
    }
  });

  ws.on('close', () => {
    console.log('❌ Disconnected. Reconnecting...');
    setTimeout(connect, 3000);
  });

  ws.on('error', (err) => {
    console.error('Socket error:', err.message);
  });
}

connect();
