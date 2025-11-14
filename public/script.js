class SensorMonitor {
    constructor() {
        this.socket = io();
        this.init();
    }
    
    init() {
        this.setupSocketEvents();
        this.checkServerStatus();
    }
    
    setupSocketEvents() {
        // Conexión establecida
        this.socket.on('connect', () => {
            this.updateConnectionStatus('connected', 'Conectado al servidor');
        });
        
        // Desconexión
        this.socket.on('disconnect', () => {
            this.updateConnectionStatus('disconnected', 'Desconectado del servidor');
        });
        
        // Recibir datos del sensor
        this.socket.on('sensorData', (data) => {
            this.updateDisplay(data);
        });
        
        // Error de conexión
        this.socket.on('connect_error', () => {
            this.updateConnectionStatus('disconnected', 'Error de conexión');
        });
    }
    
    updateConnectionStatus(status, text) {
        const indicator = document.getElementById('connectionIndicator');
        const textElement = document.getElementById('connectionText');
        const container = document.querySelector('.connection-status');
        
        container.className = `connection-status ${status}`;
        textElement.textContent = text;
    }
    
    updateDisplay(data) {
        // Actualizar temperatura
        document.getElementById('temperature').textContent = data.temperatura;
        
        // Actualizar humedad
        document.getElementById('humidity').textContent = data.humedad;
        
        // Actualizar estado del ventilador
        const ventiladorElement = document.getElementById('ventiladorStatus');
        ventiladorElement.className = `status ${data.ventilador ? 'status-on' : 'status-off'}`;
        ventiladorElement.querySelector('.status-text').textContent = 
            data.ventilador ? 'ENCENDIDO' : 'APAGADO';
        
        // Actualizar estado del foco
        const focoElement = document.getElementById('focoStatus');
        focoElement.className = `status ${data.foco ? 'status-on' : 'status-off'}`;
        focoElement.querySelector('.status-text').textContent = 
            data.foco ? 'ENCENDIDO' : 'APAGADO';
        
        // Actualizar timestamp y modo
        document.getElementById('lastUpdate').textContent = data.lastUpdate;
        
        // Mostrar modo de operación
        this.showOperationMode(data.conexion);
        
        // Efecto visual de actualización
        this.highlightUpdate();
    }
    
    showOperationMode(mode) {
        const footer = document.querySelector('footer p');
        if (mode === 'conectado') {
            footer.innerHTML = '✅ <strong>DATOS REALES</strong> - Conectado al Arduino';
            footer.style.color = '#28a745';
        } else if (mode === 'simulación') {
            footer.innerHTML = '🔄 <strong>MODO SIMULACIÓN</strong> - Esperando Arduino';
            footer.style.color = '#ffc107';
        } else {
            footer.innerHTML = '❌ <strong>DESCONECTADO</strong> - Sin datos';
            footer.style.color = '#dc3545';
        }
    }
    
    highlightUpdate() {
        const cards = document.querySelectorAll('.status-card');
        cards.forEach(card => {
            card.style.transform = 'scale(1.02)';
            setTimeout(() => {
                card.style.transform = 'scale(1)';
            }, 300);
        });
    }
    
    async checkServerStatus() {
        try {
            const response = await fetch('/api/status');
            const status = await response.json();
            console.log('Estado del servidor:', status);
        } catch (error) {
            console.log('Error verificando estado del servidor:', error);
        }
    }
}

// Inicializar la aplicación cuando el DOM esté listo
document.addEventListener('DOMContentLoaded', () => {
    new SensorMonitor();
});