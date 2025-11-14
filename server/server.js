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
  lastUpdate: 'Esperando datos del Arduino...',
  conexion: 'desconectado'
};

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
    
    // Validar datos
    if (temperatura === undefined || humedad === undefined) {
      throw new Error('Datos incompletos');
    }
    
    // Actualizar con datos reales
    sensorData.temperatura = parseFloat(temperatura);
    sensorData.humedad = parseFloat(humedad);
    sensorData.ventilador = Boolean(ventilador);
    sensorData.foco = Boolean(foco);
    sensorData.lastUpdate = new Date().toLocaleTimeString();
    sensorData.conexion = 'conectado';
    
    console.log(`📊 Datos REALES recibidos: ${temperatura}°C, ${humedad}%`);
    
    // Enviar a todos los clientes via WebSocket
    io.emit('sensorData', sensorData);
    
    res.json({ success: true, message: 'Datos recibidos' });
  } catch (error) {
    console.log('❌ Error recibiendo datos:', error);
    res.status(400).json({ success: false, error: error.message });
  }
});

// Ruta para ver estado del servidor
app.get('/api/status', (req, res) => {
  res.json({
    status: 'online',
    modo: 'esperando datos reales del Arduino',
    datos_recibidos: sensorData.conexion === 'conectado',
    ultima_actualizacion: sensorData.lastUpdate
  });
});

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
  console.log('✅ Servidor iniciado - SIN SIMULACIÓN');
  console.log(`📡 Puerto: ${PORT}`);
  console.log(`🌐 URL: http://localhost:${PORT}`);
  console.log('🔧 Modo: ESPERANDO DATOS REALES DEL ARDUINO');
  console.log('🚀 =================================');
});