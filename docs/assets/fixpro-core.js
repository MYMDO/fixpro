/**
 * FiXPro Web Interface - Module Architecture
 * @version 2.1.0
 * @description Modular JavaScript for FiXPro Universal Programmer
 */

const FiXPro = {
    VERSION: '2.1.0',
    
    config: {
        baudRate: 115200,
        bufferSize: 256,
        timeout: 5000,
        retryCount: 3,
        retryDelay: 1000
    }
};

FiXPro.Logger = class Logger {
    static levels = { DEBUG: 0, INFO: 1, WARN: 2, ERROR: 3 };
    static currentLevel = 1;
    
    static debug(msg, ...args) {
        if (this.currentLevel <= this.levels.DEBUG) {
            console.debug(`[DEBUG] ${msg}`, ...args);
            this._log('debug', msg);
        }
    }
    
    static info(msg, ...args) {
        if (this.currentLevel <= this.levels.INFO) {
            console.info(`[INFO] ${msg}`, ...args);
            this._log('info', msg);
        }
    }
    
    static warn(msg, ...args) {
        if (this.currentLevel <= this.levels.WARN) {
            console.warn(`[WARN] ${msg}`, ...args);
            this._log('warn', msg);
        }
    }
    
    static error(msg, ...args) {
        if (this.currentLevel <= this.levels.ERROR) {
            console.error(`[ERROR] ${msg}`, ...args);
            this._log('error', msg);
        }
    }
    
    static _log(type, msg) {
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
    
    static setLevel(level) {
        this.currentLevel = level;
    }
};

FiXPro.Serial = class SerialPort {
    constructor() {
        this.port = null;
        this.reader = null;
        this.connected = false;
        this._buffer = '';
        this._readPromise = null;
        this._readResolve = null;
    }
    
    async requestPort() {
        try {
            const ports = await navigator.serial.getPorts();
            if (ports.length > 0) {
                FiXPro.Logger.debug('Using previously authorized port');
                return ports[0];
            }
            FiXPro.Logger.debug('Requesting new port');
            return await navigator.serial.requestPort();
        } catch (err) {
            FiXPro.Logger.error('Failed to request port:', err.message);
            throw err;
        }
    }
    
    async connect(port) {
        try {
            await port.open({ 
                baudRate: FiXPro.config.baudRate,
                dataBits: 8,
                stopBits: 1,
                parity: 'none'
            });
            this.port = port;
            this.connected = true;
            FiXPro.Events.emit('connected');
            FiXPro.Logger.info('Connected to device');
            this._startReading();
            return true;
        } catch (err) {
            FiXPro.Logger.error('Connection failed:', err.message);
            throw err;
        }
    }
    
    async disconnect() {
        this.connected = false;
        
        if (this.reader) {
            try {
                await this.reader.cancel();
            } catch (e) {}
            this.reader = null;
        }
        
        if (this.port) {
            try {
                await this.port.close();
            } catch (e) {}
            this.port = null;
        }
        
        FiXPro.Events.emit('disconnected');
        FiXPro.Logger.info('Disconnected from device');
    }
    
    async _startReading() {
        while (this.port && this.connected) {
            try {
                this.reader = this.port.readable.getReader();
                const { value, done } = await this.reader.read();
                
                if (done) {
                    this.connected = false;
                    FiXPro.Events.emit('disconnected');
                    break;
                }
                
                const text = new TextDecoder().decode(value);
                this._buffer += text;
                
                const lines = this._buffer.split('\n');
                this._buffer = lines.pop() || '';
                
                for (const line of lines) {
                    const trimmed = line.trim();
                    if (trimmed) {
                        FiXPro.Events.emit('data', trimmed);
                    }
                }
            } catch (err) {
                if (err.name !== 'AbortError') {
                    FiXPro.Logger.error('Read error:', err.message);
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
        FiXPro.Logger.debug('TX:', data);
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
                FiXPro.Logger.error(`Event handler error (${event}):`, err.message);
            }
        });
    },
    
    once(event, callback) {
        const wrapper = (data) => {
            this.off(event, wrapper);
            callback(data);
        };
        this.on(event, wrapper);
    }
};

FiXPro.Protocol = class Protocol {
    static COMMANDS = {
        PING: 'PING',
        CAPS: 'CAPS',
        VERSION: 'VERSION',
        GPIO: 'GPIO',
        GPIO_SET: 'GPIO_SET',
        GPIO_CLR: 'GPIO_CLR',
        SPI_ID: 'SPI_ID',
        SPI_READ: 'SPI_READ',
        SPI_WRITE: 'SPI_WRITE',
        SPI_ERASE: 'SPI_ERASE',
        SPI_ERASE_CHIP: 'SPI_ERASE_CHIP',
        I2C_SCAN: 'I2C_SCAN',
        I2C_READ: 'I2C_READ',
        I2C_WRITE: 'I2C_WRITE',
        STATUS: 'STATUS',
        INFO: 'INFO',
        HELP: 'HELP'
    };
    
    static _sendCommand(cmd, parseFn = (data) => data) {
        return new Promise(async (resolve, reject) => {
            const timeout = setTimeout(() => {
                reject(new Error(`Timeout waiting for ${cmd}`));
            }, FiXPro.config.timeout);
            
            FiXPro.Events.once('data', (data) => {
                clearTimeout(timeout);
                resolve(parseFn(data));
            });
            
            try {
                await FiXPro.Serial.send(cmd);
            } catch (err) {
                clearTimeout(timeout);
                reject(err);
            }
        });
    }
    
    static async ping() {
        return this._sendCommand(this.COMMANDS.PING, (data) => data === 'CAFE');
    }
    
    static async getCaps() {
        return this._sendCommand(this.COMMANDS.CAPS);
    }
    
    static async getVersion() {
        return this._sendCommand(this.COMMANDS.VERSION);
    }
    
    static async getGpio() {
        return this._sendCommand(this.COMMANDS.GPIO);
    }
    
    static async gpioSet() {
        return this._sendCommand(this.COMMANDS.GPIO_SET);
    }
    
    static async gpioClr() {
        return this._sendCommand(this.COMMANDS.GPIO_CLR);
    }
    
    static async getSpiId() {
        return this._sendCommand(this.COMMANDS.SPI_ID);
    }
    
    static async spiRead(addr, len) {
        const cmd = `${this.COMMANDS.SPI_READ} ${addr} ${len}`;
        return this._sendCommand(cmd);
    }
    
    static async spiWrite(addr, hexData) {
        const cmd = `${this.COMMANDS.SPI_WRITE} ${addr} ${hexData}`;
        return this._sendCommand(cmd);
    }
    
    static async spiErase(addr, len = 4096) {
        const cmd = `${this.COMMANDS.SPI_ERASE} ${addr} ${len}`;
        return this._sendCommand(cmd);
    }
    
    static async spiEraseChip() {
        return this._sendCommand(this.COMMANDS.SPI_ERASE_CHIP);
    }
    
    static async scanI2c() {
        return this._sendCommand(this.COMMANDS.I2C_SCAN);
    }
    
    static async i2cRead(addr, reg, len = 1) {
        const cmd = `${this.COMMANDS.I2C_READ} ${addr} ${reg} ${len}`;
        return this._sendCommand(cmd);
    }
    
    static async i2cWrite(addr, reg, data) {
        const cmd = `${this.COMMANDS.I2C_WRITE} ${addr} ${reg} ${data}`;
        return this._sendCommand(cmd);
    }
    
    static async getStatus() {
        return this._sendCommand(this.COMMANDS.STATUS);
    }
    
    static async getInfo() {
        return new Promise((resolve) => {
            FiXPro.Serial.send(this.COMMANDS.INFO);
            const lines = [];
            const timeout = setTimeout(() => {
                FiXPro.Events.off('data', handler);
                resolve(lines);
            }, 2000);
            
            const handler = (data) => {
                if (data.includes('FiXPro Commands')) {
                    clearTimeout(timeout);
                    FiXPro.Events.off('data', handler);
                    resolve(lines);
                } else {
                    lines.push(data);
                }
            };
            FiXPro.Events.on('data', handler);
        });
    }
    
    static parseResponse(response) {
        const colonIdx = response.indexOf(':');
        if (colonIdx === -1) {
            return { type: response, data: null, raw: response };
        }
        const type = response.substring(0, colonIdx);
        const data = response.substring(colonIdx + 1);
        return { type, data, raw: response };
    }
    
    static parseGpio(response) {
        const parsed = this.parseResponse(response);
        if (parsed.type === 'GPIO') {
            const hex = parsed.data;
            return {
                raw: hex,
                value: parseInt(hex, 16),
                bits: hex.padStart(8, '0').split('').reverse()
            };
        }
        throw new Error('Invalid GPIO response');
    }
    
    static parseSpiId(response) {
        const parsed = this.parseResponse(response);
        if (parsed.type === 'SPI') {
            const id = parsed.data;
            return {
                raw: id,
                manufacturer: id.substring(0, 2),
                device: id.substring(2, 4),
                capacity: id.substring(4, 6),
                manufacturerName: this._getMfgName(id.substring(0, 2))
            };
        }
        throw new Error('Invalid SPI response');
    }
    
    static _getMfgName(code) {
        const mfg = {
            'EF': 'Winbond',
            'C8': 'GigaDevice',
            'C2': 'Macronix',
            '1C': 'eON',
            '68': 'Boya',
            '85': 'Puya',
            '20': 'Micron',
            '1F': 'Atmel',
            '4B': 'Adsun'
        };
        return mfg[code] || 'Unknown';
    }
    
    static parseI2cScan(response) {
        const parsed = this.parseResponse(response);
        if (parsed.type === 'I2C') {
            const data = parsed.data.trim();
            if (!data) return [];
            return data.split(/\s+/).filter(Boolean);
        }
        return [];
    }
    
    static parseStatus(response) {
        const parsed = this.parseResponse(response);
        if (parsed.type === 'STATUS') {
            const parts = parsed.data.split(',');
            const status = {};
            parts.forEach(p => {
                const [key, value] = p.split('=');
                status[key] = value;
            });
            return status;
        }
        return {};
    }
};

FiXPro.UI = {
    elements: {},
    
    init() {
        this.elements = {
            connectBtn: document.getElementById('connectBtn'),
            statusDot: document.getElementById('statusDot'),
            statusText: document.getElementById('statusText'),
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
        if (this.elements.statusDot) {
            this.elements.statusDot.classList.toggle('connected', connected);
            this.elements.statusDot.classList.toggle('busy', !connected);
        }
        if (this.elements.statusText) {
            this.elements.statusText.innerHTML = connected 
                ? '<strong>Connected</strong>' 
                : 'Click to connect device';
        }
    },
    
    setBusy(busy) {
        if (this.elements.statusDot) {
            this.elements.statusDot.classList.toggle('busy', busy);
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
    
    showInfo(message) {
        if (this.elements.activityLog) {
            const time = new Date().toLocaleTimeString();
            const line = document.createElement('div');
            line.className = 'log-info';
            line.textContent = `[${time}] ${message}`;
            this.elements.activityLog.appendChild(line);
            this.elements.activityLog.scrollTop = this.elements.activityLog.scrollHeight;
        }
    },
    
    showError(message) {
        if (this.elements.activityLog) {
            const time = new Date().toLocaleTimeString();
            const line = document.createElement('div');
            line.className = 'log-error';
            line.textContent = `[${time}] ERROR: ${message}`;
            this.elements.activityLog.appendChild(line);
            this.elements.activityLog.scrollHeight;
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
    _info: {
        connected: false,
        firmwareVersion: null,
        capabilities: null
    },
    
    async connect() {
        try {
            const port = await FiXPro.Serial.requestPort();
            await FiXPro.Serial.connect(port);
            FiXPro.UI.setConnected(true);
            this._info.connected = true;
            
            const caps = await FiXPro.Protocol.getCaps();
            this._info.capabilities = caps;
            FiXPro.UI.updateCapabilities(caps);
            
            const version = await FiXPro.Protocol.getVersion();
            this._info.firmwareVersion = version;
            FiXPro.UI.updateFirmware(version);
            
            FiXPro.UI.showInfo('Device connected successfully');
            return true;
        } catch (err) {
            FiXPro.UI.showError(`Connection failed: ${err.message}`);
            FiXPro.Logger.error('Connection failed:', err);
            return false;
        }
    },
    
    async disconnect() {
        await FiXPro.Serial.disconnect();
        FiXPro.UI.setConnected(false);
        this._info.connected = false;
        FiXPro.UI.showInfo('Device disconnected');
    },
    
    async ping() {
        try {
            FiXPro.UI.setBusy(true);
            const success = await FiXPro.Protocol.ping();
            FiXPro.UI.showResponse('PING', success ? 'CAFE' : 'FAILED');
            FiXPro.UI.setBusy(false);
            return success;
        } catch (err) {
            FiXPro.UI.showError('PING failed');
            FiXPro.UI.setBusy(false);
            return false;
        }
    },
    
    async getCaps() {
        try {
            const caps = await FiXPro.Protocol.getCaps();
            FiXPro.UI.updateCapabilities(caps);
            FiXPro.UI.showResponse('CAPS', caps);
            return caps;
        } catch (err) {
            FiXPro.UI.showError('Failed to get capabilities');
            return null;
        }
    },
    
    async getVersion() {
        try {
            const version = await FiXPro.Protocol.getVersion();
            FiXPro.UI.updateFirmware(version);
            FiXPro.UI.showResponse('VERSION', version);
            return version;
        } catch (err) {
            FiXPro.UI.showError('Failed to get version');
            return null;
        }
    },
    
    async gpio() {
        try {
            FiXPro.UI.setBusy(true);
            const response = await FiXPro.Protocol.getGpio();
            FiXPro.UI.showResponse('GPIO', response);
            FiXPro.UI.updateGpio(response);
            FiXPro.UI.setBusy(false);
            return response;
        } catch (err) {
            FiXPro.UI.showError('GPIO read failed');
            FiXPro.UI.setBusy(false);
            return null;
        }
    },
    
    async gpioSet() {
        try {
            await FiXPro.Protocol.gpioSet();
            FiXPro.UI.showResponse('GPIO_SET', 'OK');
            return true;
        } catch (err) {
            FiXPro.UI.showError('GPIO_SET failed');
            return false;
        }
    },
    
    async gpioClr() {
        try {
            await FiXPro.Protocol.gpioClr();
            FiXPro.UI.showResponse('GPIO_CLR', 'OK');
            return true;
        } catch (err) {
            FiXPro.UI.showError('GPIO_CLR failed');
            return false;
        }
    },
    
    async spiId() {
        try {
            FiXPro.UI.setBusy(true);
            const response = await FiXPro.Protocol.getSpiId();
            FiXPro.UI.showResponse('SPI_ID', response);
            FiXPro.UI.updateSpiId(response);
            FiXPro.UI.setBusy(false);
            return response;
        } catch (err) {
            FiXPro.UI.showError('SPI ID read failed');
            FiXPro.UI.setBusy(false);
            return null;
        }
    },
    
    async i2cScan() {
        try {
            FiXPro.UI.setBusy(true);
            const response = await FiXPro.Protocol.scanI2c();
            FiXPro.UI.showResponse('I2C_SCAN', response);
            const devices = FiXPro.Protocol.parseI2cScan(response);
            FiXPro.UI.updateI2cDevices(devices);
            FiXPro.UI.setBusy(false);
            return devices;
        } catch (err) {
            FiXPro.UI.showError('I2C scan failed');
            FiXPro.UI.setBusy(false);
            return [];
        }
    },
    
    async status() {
        try {
            const response = await FiXPro.Protocol.getStatus();
            FiXPro.UI.showResponse('STATUS', response);
            return FiXPro.Protocol.parseStatus(response);
        } catch (err) {
            FiXPro.UI.showError('STATUS failed');
            return null;
        }
    },
    
    async info() {
        try {
            FiXPro.UI.showInfo('Fetching device info...');
            const lines = await FiXPro.Protocol.getInfo();
            lines.forEach(line => FiXPro.UI.showInfo(line));
            return lines;
        } catch (err) {
            FiXPro.UI.showError('INFO failed');
            return [];
        }
    },
    
    getInfo() {
        return { ...this._info };
    }
};

window.FiXPro = FiXPro;
window.FiXPro.Logger = FiXPro.Logger;
window.FiXPro.Serial = FiXPro.Serial;
window.FiXPro.Protocol = FiXPro.Protocol;
window.FiXPro.UI = FiXPro.UI;
window.FiXPro.Actions = FiXPro.Actions;
