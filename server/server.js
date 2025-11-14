const express = require('express');
const http = require('http');
const socketIo = require('socket.io');
const path = require('path');
const cors = require('cors');

const app = express();
const server = http.createServer(app);
const io = socketIo(server);

// Middleware
app.use(cors());
app.use(express.json());
app.use(express.static(path.join(__dirname, '../public')));

// Datos del sensor (inicialmente vacíos)
let sensorData = {
  temperatura: 0,
  humedad: 0,
  ventilador: false,
  foco: false,
  lastUpdate: 'Esperando datos...',
  conexion: 'desconectado'
};

// Variable para controlar simulación
let modoSimulacion = true;

// Ruta principal
app.get('/', (req, res) => {
  res.sendFile(path.join(__dirname, '../public/index.html'));
});

// API para obtener datos
app.get('/api/data', (req, res) => {
  res.json(sensorData);
});

// Ruta para recibir datos REALES del ESP32
app.post('/api/data', (req, res) => {
  try {
    const { temperatura, humedad, ventilador, foco } = req.body;
    
    // Actualizar con datos reales
    sensorData.temperatura = parseFloat(temperatura);
    sensorData.humedad = parseFloat(humedad);
    sensorData.ventilador = Boolean(ventilador);
    sensorData.foco = Boolean(foco);
    sensorData.lastUpdate = new Date().toLocaleTimeString();
    sensorData.conexion = 'conectado';
    
    // Desactivar simulación cuando lleguen datos reales
    modoSimulacion = false;
    
    console.log(`📊 Datos REALES recibidos: ${temperatura}°C, ${humedad}%`);
    
    // Enviar a todos los clientes via WebSocket
    io.emit('sensorData', sensorData);
    
    res.json({ success: true, message: 'Datos recibidos' });
  } catch (error) {
    console.log('❌ Error recibiendo datos:', error);
    res.status(500).json({ success: false, error: error.message });
  }
});

// Ruta para ver estado del servidor
app.get('/api/status', (req, res) => {
  res.json({
    status: 'online',
    modo: modoSimulacion ? 'simulación' : 'datos reales',
    puerto: process.env.PORT || 3000,
    timestamp: new Date().toISOString()
  });
});

// Simulación de datos (solo si no hay datos reales)
setInterval(() => {
  if (modoSimulacion) {
    sensorData.temperatura = (20 + Math.random() * 8).toFixed(1);
    sensorData.humedad = (60 + Math.random() * 20).toFixed(1);
    sensorData.ventilador = parseFloat(sensorData.temperatura) > 24;
    sensorData.foco = parseFloat(sensorData.temperatura) > 21 && !sensorData.ventilador;
    sensorData.lastUpdate = new Date().toLocaleTimeString();
    sensorData.conexion = 'simulación';
    
    console.log(`🔄 Datos SIMULADOS: ${sensorData.temperatura}°C`);
    io.emit('sensorData', sensorData);
  }
}, 3000);

// WebSocket para conexiones
io.on('connection', (socket) => {
  console.log('🔌 Cliente conectado via WebSocket');
  
  // Enviar datos actuales inmediatamente
  socket.emit('sensorData', sensorData);
  
  socket.on('disconnect', () => {
    console.log('❌ Cliente desconectado');
  });
});

const PORT = process.env.PORT || 3000;

server.listen(PORT, () => {
  console.log('🚀 =================================');
  console.log('✅ Servidor iniciado correctamente');
  console.log(`📡 Puerto: ${PORT}`);
  console.log(`🌐 URL: http://localhost:${PORT}`);
  console.log(`🔧 Modo: ${modoSimulacion ? 'SIMULACIÓN' : 'DATOS REALES'}`);
  console.log('🚀 =================================');
});