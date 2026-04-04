/**
 * FiXPro Web Interface - Module Architecture
 * Modular JavaScript for FiXPro Universal Programmer
 */

const FiXPro = {
    VERSION: '2.1.0',
    
    config: {
        baudRate: 115200,
        bufferSize: 256,
        timeout: 5000
    }
};

FiXPro.Logger = class Logger {
    static levels = { DEBUG: 0, INFO: 1, WARN: 2, ERROR: 3 };
    static currentLevel = 1;
    
    static debug(msg, ...args) {
        if (this.currentLevel <= this.levels.DEBUG) {
            console.debug(`[DEBUG] ${msg}`, ...args);
            this._log('debug', msg, args);
        }
    }
    
    static info(msg, ...args) {
        if (this.currentLevel <= this.levels.INFO) {
            console.info(`[INFO] ${msg}`, ...args);
            this._log('info', msg, args);
        }
    }
    
    static warn(msg, ...args) {
        if (this.currentLevel <= this.levels.WARN) {
            console.warn(`[WARN] ${msg}`, ...args);
            this._log('warn', msg, args);
        }
    }
    
    static error(msg, ...args) {
        if (this.currentLevel <= this.levels.ERROR) {
            console.error(`[ERROR] ${msg}`, ...args);
            this._log('error', msg, args);
        }
    }
    
    static _log(type, msg, args) {
        const logEl = document.getElementById('activityLog');
        if (logEl) {
            const time = new Date().toLocaleTimeString();
            const line = document.createElement('div');
            line.className = `log-${type}`;
            line.textContent = `[${time}] ${msg}`;
            logEl.appendChild(line);
            logEl.scrollTop = logEl.scrollHeight;
        }
    }
};

FiXPro.Serial = class SerialPort {
    constructor() {
        this.port = null;
        this.reader = null;
        this.connected = false;
        this._buffer = '';
    }
    
    async requestPort() {
        try {
            const ports = await navigator.serial.getPorts();
            if (ports.length > 0) {
                return ports[0];
            }
            return await navigator.serial.requestPort();
        } catch (err) {
            FiXPro.Logger.error('Failed to request port:', err);
            throw err;
        }
    }
    
    async connect(port) {
        try {
            await port.open({ baudRate: FiXPro.config.baudRate });
            this.port = port;
            this.connected = true;
            FiXPro.Logger.info('Connected to device');
            this._startReading();
            return true;
        } catch (err) {
            FiXPro.Logger.error('Connection failed:', err);
            throw err;
        }
    }
    
    async disconnect() {
        if (this.reader) {
            await this.reader.cancel();
            this.reader = null;
        }
        if (this.port) {
            await this.port.close();
            this.port = null;
        }
        this.connected = false;
        FiXPro.Logger.info('Disconnected from device');
    }
    
    async _startReading() {
        while (this.port && this.connected) {
            try {
                this.reader = this.port.readable.getReader();
                const { value, done } = await this.reader.read();
                if (done) break;
                
                const text = new TextDecoder().decode(value);
                this._buffer += text;
                
                const lines = this._buffer.split('\n');
                this._buffer = lines.pop() || '';
                
                for (const line of lines) {
                    if (line.trim()) {
                        FiXPro.Events.emit('data', line);
                    }
                }
            } catch (err) {
                if (err.name !== 'AbortError') {
                    FiXPro.Logger.error('Read error:', err);
                }
                break;
            }
        }
    }
    
    async send(data) {
        if (!this.port || !this.connected) {
            throw new Error('Not connected');
        }
        
        const writer = this.port.writable.getWriter();
        const encoder = new TextEncoder();
        await writer.write(encoder.encode(data + '\n'));
        writer.releaseLock();
        FiXPro.Logger.debug('Sent:', data);
    }
};

FiXPro.Events = {
    _listeners: {},
    
    on(event, callback) {
        if (!this._listeners[event]) {
            this._listeners[event] = [];
        }
        this._listeners[event].push(callback);
    },
    
    off(event, callback) {
        if (!this._listeners[event]) return;
        this._listeners[event] = this._listeners[event].filter(cb => cb !== callback);
    },
    
    emit(event, data) {
        if (!this._listeners[event]) return;
        this._listeners[event].forEach(cb => {
            try {
                cb(data);
            } catch (err) {
                FiXPro.Logger.error(`Event handler error (${event}):`, err);
            }
        });
    }
};

FiXPro.Protocol = class Protocol {
    static COMMANDS = {
        PING: 'PING',
        CAPS: 'CAPS',
        VERSION: 'VERSION',
        GPIO: 'GPIO',
        GPIO_TEST: 'GPIO_TEST',
        GPIO_SET: 'GPIO_SET',
        GPIO_CLR: 'GPIO_CLR',
        SPI_ID: 'SPI_ID',
        I2C_SCAN: 'I2C_SCAN',
        STATUS: 'STATUS',
        INFO: 'INFO',
        HELP: 'HELP'
    };
    
    static async ping() {
        await FiXPro.Serial.send(this.COMMANDS.PING);
        return new Promise((resolve, reject) => {
            const timeout = setTimeout(() => reject(new Error('Timeout')), FiXPro.config.timeout);
            FiXPro.Events.once('data', (data) => {
                clearTimeout(timeout);
                resolve(data);
            });
        });
    }
    
    static async getCaps() {
        await FiXPro.Serial.send(this.COMMANDS.CAPS);
        return new Promise((resolve, reject) => {
            const timeout = setTimeout(() => reject(new Error('Timeout')), FiXPro.config.timeout);
            FiXPro.Events.once('data', (data) => {
                clearTimeout(timeout);
                resolve(data);
            });
        });
    }
    
    static async getVersion() {
        await FiXPro.Serial.send(this.COMMANDS.VERSION);
        return new Promise((resolve, reject) => {
            const timeout = setTimeout(() => reject(new Error('Timeout')), FiXPro.config.timeout);
            FiXPro.Events.once('data', (data) => {
                clearTimeout(timeout);
                resolve(data);
            });
        });
    }
    
    static async getGpio() {
        await FiXPro.Serial.send(this.COMMANDS.GPIO);
        return new Promise((resolve, reject) => {
            const timeout = setTimeout(() => reject(new Error('Timeout')), FiXPro.config.timeout);
            FiXPro.Events.once('data', (data) => {
                clearTimeout(timeout);
                resolve(data);
            });
        });
    }
    
    static async getSpiId() {
        await FiXPro.Serial.send(this.COMMANDS.SPI_ID);
        return new Promise((resolve, reject) => {
            const timeout = setTimeout(() => reject(new Error('Timeout')), FiXPro.config.timeout);
            FiXPro.Events.once('data', (data) => {
                clearTimeout(timeout);
                resolve(data);
            });
        });
    }
    
    static async scanI2c() {
        await FiXPro.Serial.send(this.COMMANDS.I2C_SCAN);
        return new Promise((resolve, reject) => {
            const timeout = setTimeout(() => reject(new Error('Timeout')), FiXPro.config.timeout);
            FiXPro.Events.once('data', (data) => {
                clearTimeout(timeout);
                resolve(data);
            });
        });
    }
    
    static parseResponse(response) {
        const parts = response.split(':');
        const type = parts[0];
        const data = parts.slice(1).join(':');
        return { type, data, raw: response };
    }
    
    static parseGpio(response) {
        const parsed = this.parseResponse(response);
        if (parsed.type === 'GPIO') {
            return {
                raw: parsed.data,
                bits: parsed.data.padStart(8, '0').split('').reverse()
            };
        }
        throw new Error('Invalid GPIO response');
    }
    
    static parseSpiId(response) {
        const parsed = this.parseResponse(response);
        if (parsed.type === 'SPI') {
            const id = parsed.data;
            return {
                manufacturer: id.substring(0, 2),
                device: id.substring(2, 4),
                capacity: id.substring(4, 6)
            };
        }
        throw new Error('Invalid SPI response');
    }
    
    static parseI2cScan(response) {
        const parsed = this.parseResponse(response);
        if (parsed.type === 'I2C') {
            return parsed.data.trim().split(/\s+/).filter(Boolean);
        }
        return [];
    }
};

FiXPro.UI = {
    elements: {},
    
    init() {
        this.elements = {
            connectBtn: document.getElementById('connectBtn'),
            statusIndicator: document.getElementById('statusIndicator'),
            connectionStatus: document.getElementById('connectionStatus'),
            activityLog: document.getElementById('activityLog'),
            firmwareVersion: document.getElementById('firmwareVersion'),
            deviceCapabilities: document.getElementById('deviceCapabilities'),
            gpioValue: document.getElementById('gpioValue'),
            spiIdValue: document.getElementById('spiIdValue'),
            i2cDevices: document.getElementById('i2cDevices')
        };
        
        FiXPro.Logger.info('UI initialized');
    },
    
    setConnected(connected) {
        if (this.elements.connectBtn) {
            this.elements.connectBtn.textContent = connected ? 'Disconnect' : '⚡ Connect';
            this.elements.connectBtn.classList.toggle('connected', connected);
        }
        if (this.elements.statusIndicator) {
            this.elements.statusIndicator.classList.toggle('online', connected);
        }
        if (this.elements.connectionStatus) {
            this.elements.connectionStatus.textContent = connected ? 'Connected' : 'Disconnected';
        }
    },
    
    showResponse(command, response) {
        if (this.elements.activityLog) {
            const time = new Date().toLocaleTimeString();
            const line = document.createElement('div');
            line.className = 'log-cmd';
            line.innerHTML = `<span class="cmd-time">[${time}]</span> <span class="cmd-name">${command}</span> → <span class="cmd-response">${response}</span>`;
            this.elements.activityLog.appendChild(line);
            this.elements.activityLog.scrollTop = this.elements.activityLog.scrollHeight;
        }
    },
    
    updateCapabilities(caps) {
        if (this.elements.deviceCapabilities) {
            this.elements.deviceCapabilities.textContent = caps || 'Unknown';
        }
    },
    
    updateFirmware(version) {
        if (this.elements.firmwareVersion) {
            this.elements.firmwareVersion.textContent = version || 'Unknown';
        }
    },
    
    updateGpio(gpio) {
        if (this.elements.gpioValue) {
            this.elements.gpioValue.textContent = gpio;
        }
    },
    
    updateSpiId(id) {
        if (this.elements.spiIdValue) {
            this.elements.spiIdValue.textContent = id || 'Not detected';
        }
    },
    
    updateI2cDevices(devices) {
        if (this.elements.i2cDevices) {
            this.elements.i2cDevices.textContent = devices.length > 0 ? devices.join(', ') : 'None found';
        }
    }
};

FiXPro.Actions = {
    async connect() {
        try {
            const port = await FiXPro.Serial.requestPort();
            await FiXPro.Serial.connect(port);
            FiXPro.UI.setConnected(true);
            
            FiXPro.Events.on('data', (data) => {
                FiXPro.Logger.debug('Received:', data);
            });
            
            const caps = await FiXPro.Protocol.getCaps();
            FiXPro.UI.updateCapabilities(caps);
            
            const version = await FiXPro.Protocol.getVersion();
            FiXPro.UI.updateFirmware(version);
            
            return true;
        } catch (err) {
            FiXPro.Logger.error('Connection failed:', err);
            return false;
        }
    },
    
    async disconnect() {
        await FiXPro.Serial.disconnect();
        FiXPro.UI.setConnected(false);
    },
    
    async ping() {
        try {
            const response = await FiXPro.Protocol.ping();
            FiXPro.UI.showResponse('PING', response);
            return response === 'CAFE';
        } catch (err) {
            FiXPro.Logger.error('PING failed:', err);
            return false;
        }
    },
    
    async gpio() {
        try {
            const response = await FiXPro.Protocol.getGpio();
            FiXPro.UI.showResponse('GPIO', response);
            FiXPro.UI.updateGpio(response);
            return response;
        } catch (err) {
            FiXPro.Logger.error('GPIO read failed:', err);
            return null;
        }
    },
    
    async spiId() {
        try {
            const response = await FiXPro.Protocol.getSpiId();
            FiXPro.UI.showResponse('SPI_ID', response);
            FiXPro.UI.updateSpiId(response);
            return response;
        } catch (err) {
            FiXPro.Logger.error('SPI ID read failed:', err);
            return null;
        }
    },
    
    async i2cScan() {
        try {
            const response = await FiXPro.Protocol.scanI2c();
            FiXPro.UI.showResponse('I2C_SCAN', response);
            const devices = FiXPro.Protocol.parseI2cScan(response);
            FiXPro.UI.updateI2cDevices(devices);
            return devices;
        } catch (err) {
            FiXPro.Logger.error('I2C scan failed:', err);
            return [];
        }
    }
};

FiXPro.Events.once = function(event, callback) {
    const wrapper = (data) => {
        this.off(event, wrapper);
        callback(data);
    };
    this.on(event, wrapper);
};

window.FiXPro = FiXPro;
window.FiXPro.Logger = FiXPro.Logger;
window.FiXPro.Serial = FiXPro.Serial;
window.FiXPro.Protocol = FiXPro.Protocol;
window.FiXPro.UI = FiXPro.UI;
window.FiXPro.Actions = FiXPro.Actions;
