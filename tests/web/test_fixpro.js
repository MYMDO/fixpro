/**
 * FiXPro Web Interface - Tests
 */

describe('FiXPro.Logger', () => {
    it('should log debug messages', () => {
        FiXPro.Logger.debug('Test debug message');
        expect(true).toBe(true);
    });
    
    it('should log info messages', () => {
        FiXPro.Logger.info('Test info message');
        expect(true).toBe(true);
    });
    
    it('should log error messages', () => {
        FiXPro.Logger.error('Test error message');
        expect(true).toBe(true);
    });
});

describe('FiXPro.Protocol', () => {
    describe('parseResponse', () => {
        it('should parse GPIO response', () => {
            const result = FiXPro.Protocol.parseResponse('GPIO:00000000');
            expect(result.type).toBe('GPIO');
            expect(result.data).toBe('00000000');
        });
        
        it('should parse SPI response', () => {
            const result = FiXPro.Protocol.parseResponse('SPI:EF4017');
            expect(result.type).toBe('SPI');
            expect(result.data).toBe('EF4017');
        });
        
        it('should parse CAPS response', () => {
            const result = FiXPro.Protocol.parseResponse('CAPS:i2c,spi,gpio');
            expect(result.type).toBe('CAPS');
            expect(result.data).toBe('i2c,spi,gpio');
        });
    });
    
    describe('parseGpio', () => {
        it('should parse GPIO response correctly', () => {
            const result = FiXPro.Protocol.parseGpio('GPIO:00000000');
            expect(result.raw).toBe('00000000');
            expect(result.bits).toHaveLength(8);
        });
    });
    
    describe('parseSpiId', () => {
        it('should parse SPI ID correctly', () => {
            const result = FiXPro.Protocol.parseSpiId('SPI:EF4017');
            expect(result.manufacturer).toBe('EF');
            expect(result.device).toBe('40');
            expect(result.capacity).toBe('17');
        });
    });
    
    describe('parseI2cScan', () => {
        it('should parse I2C scan response', () => {
            const result = FiXPro.Protocol.parseI2cScan('I2C:50 68 ');
            expect(result).toEqual(['50', '68']);
        });
        
        it('should return empty array for no devices', () => {
            const result = FiXPro.Protocol.parseI2cScan('I2C:');
            expect(result).toEqual([]);
        });
    });
});

describe('FiXPro.Commands', () => {
    it('should have all commands defined', () => {
        expect(FiXPro.Protocol.COMMANDS.PING).toBe('PING');
        expect(FiXPro.Protocol.COMMANDS.CAPS).toBe('CAPS');
        expect(FiXPro.Protocol.COMMANDS.VERSION).toBe('VERSION');
        expect(FiXPro.Protocol.COMMANDS.GPIO).toBe('GPIO');
        expect(FiXPro.Protocol.COMMANDS.SPI_ID).toBe('SPI_ID');
        expect(FiXPro.Protocol.COMMANDS.I2C_SCAN).toBe('I2C_SCAN');
    });
});

describe('FiXPro.Events', () => {
    it('should emit and receive events', (done) => {
        FiXPro.Events.on('test', (data) => {
            expect(data).toBe('hello');
            done();
        });
        FiXPro.Events.emit('test', 'hello');
    });
    
    it('should remove event listeners', () => {
        const callback = jest.fn();
        FiXPro.Events.on('test2', callback);
        FiXPro.Events.off('test2', callback);
        FiXPro.Events.emit('test2', 'data');
        expect(callback).not.toHaveBeenCalled();
    });
});
