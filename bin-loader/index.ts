import * as fs from 'fs/promises'
import * as path from 'path'
import {SerialPort} from 'serialport'

const BOOTLOADER_SIZE = 0x2000;

const DATA_BYTES = 16;
const PACKET_LENGTH = DATA_BYTES+2;

const ACK_DATA = 0x15;
const RETX_DATA = 0x19;

const BL_SYNC_OBS             = (0X20);
const BL_FW_UPD_REQ           = (0x31);
const BL_FW_UPD_RES           = (0x37);
const BL_DEV_ID_REQ           = (0x3C);
const BL_DEV_ID_RES           = (0x3F);
const BL_FW_LEN_REQ           = (0x42);
const BL_FW_LEN_RES           = (0x45);
const BL_READY_FOR_DATA       = (0x48);
const BL_FW_UPDATE_SUCCESSFUL = (0x54);
const BL_NACK                 = (0x59);

const VECTOR_TABLE_SIZE       = (0x150);
const FW_INFO_SIZE            = (10*4);
const FW_INFO_VALIDATE_FROM   = (VECTOR_TABLE_SIZE + FW_INFO_SIZE);

const FW_INFO_DEV_ID_OFFSET   = (VECTOR_TABLE_SIZE + (1 * 4));
const FW_INFO_VERSION_OFFSET  = (VECTOR_TABLE_SIZE + (2 * 4));
const FW_INFO_LENGTH_OFFSET   = (VECTOR_TABLE_SIZE + (3 * 4));
const FW_INFO_CRC32_OFFSET    = (VECTOR_TABLE_SIZE + (9 * 4));

const SYNC_SEQ = Buffer.from([0xC4,0x55,0x7E,0x10]);
const DEFAULT_TIMEOUT = (5000);

const serialPath = "COM11";
const baudRate = 115200;

const crc8 = (data: Buffer | Array<number>) => {
    let crc = 0;
    
    for(const byte of data){
        crc = (crc^byte)&0xff;
        for(let i=0;i<8;i++){
            if(crc&0x80){
                crc = ((crc<<1)^0x07)&0xff;
            }else{
                crc = (crc<<1)&0xff;
            }
        }
    }
    return crc;
}

const crc32 = (data:Buffer, length:number) => {
    let byte;
    let crc = 0xFFFFFFFF;
    let mask;

    for(let i=0;i<length;i++){
        byte = data[i];
        crc = (crc^byte)>>>0;

        for(let j=0;j<8;j++){
            mask = (-(crc&1))>>>0;
            crc = ((crc >>> 1) ^ (0xEDB88320 & mask)) >>> 0;
        }
    }
    return (~crc) >>> 0;
}

const delay = (ms:number) => new Promise(r=>setTimeout(r,ms));

class Logger {
    static info(message : string){ console.log(`[.] ${message}`);}
    static success(message : string) { console.log(`[$] ${message}`);}
    static error(message : string) { console.log(`[!] ${message}`);}
}

class Packet {
    length : number;
    data : Buffer;
    crc : number;

    static retx = new Packet(1,Buffer.from([RETX_DATA])).toBuffer();
    static ack = new Packet(1,Buffer.from([ACK_DATA])).toBuffer();

    constructor(length:number,data:Buffer,crc?:number){
        this.length = length;
        this.data = data;

        const bytesToPad = DATA_BYTES - this.data.length;
        const padding = Buffer.alloc(bytesToPad).fill(0xFF);
        this.data = Buffer.concat([this.data,padding]);

        if(typeof crc === 'undefined'){
            this.crc = this.computeCrc();
        }else{
            this.crc = crc;
        }
    }

    computeCrc(){
        const allData = [this.length,...this.data];
        return crc8(allData);
    }

    toBuffer(){
        return Buffer.concat([Buffer.from([this.length]),this.data,Buffer.from([this.crc])]);
    }

    isSBPacket(byte:number){
        if(this.length !== 1) return false;
        if(this.data[0] !== byte) return false;
        for(let i=1;i<DATA_BYTES;i++){
            if(this.data[i] !== 0xff) return false;
        }
        return true;
    }

    isAck(){
        return this.isSBPacket(ACK_DATA);
    }

    isRetx(){
        return this.isSBPacket(RETX_DATA);
    }

    isNAck(){
        return this.isSBPacket(BL_NACK);
    }

    static createSBPacket(byte : number){
        return new Packet(1,Buffer.from([byte]));
    }
}

const uart = new SerialPort({path:serialPath,baudRate});

let packets : Packet[] = [];
let lastPacket : Buffer = Packet.ack;

const writePacket = (packet:Buffer) => {
    uart.write(packet);
    lastPacket = packet;
}

let rxBuffer = Buffer.from([]);
const consumeFromBuffer = (n:number) => {
    const consumed = rxBuffer.slice(0,n);
    rxBuffer = rxBuffer.slice(n);
    return consumed;
}

uart.on('data',data=>{
    // console.log(`Received ${data.length} Bytes`);
    rxBuffer = Buffer.concat([rxBuffer,data]);

    while(rxBuffer.length >= PACKET_LENGTH){
        // console.log('Building a Packet');
        const raw = consumeFromBuffer(PACKET_LENGTH);
        const packet = new Packet(raw[0],raw.slice(1,DATA_BYTES+1),raw[DATA_BYTES+1]);
        // console.log(packet);
        const computedCrc = packet.computeCrc();
        // console.log(`CRC passed, computed 0x${computedCrc.toString(16)}, got ${packet.crc.toString(16)}`);


        if(packet.crc !== computedCrc){
            console.log(`CRC failed, computed 0x${computedCrc.toString(16)}, got ${packet.crc.toString(16)}`);
            writePacket(Packet.retx);
            continue;
        }

        if(packet.isRetx()){
            console.log('Retransmitting last packet');
            writePacket(lastPacket);
            continue;
        }

        if(packet.isAck()){
            // console.log('Ack received');
            continue;
        }

        if(packet.isNAck()){
            Logger.error('Received NACK. Exiting');
            process.exit(1);
        }

        // console.log('Acking and storing pkt');
        packets.push(packet);
        writePacket(Packet.ack);
    }
});

const waitForPacket = async (timeout = DEFAULT_TIMEOUT) => {
    let timeWaited = 0;
    while(packets.length < 1){
        await delay(1);
        timeWaited += 1;

        if(timeWaited >= timeout){
            throw Error('Timed out waiting for packet');
        }
    }
    return packets.splice(0,1)[0];
}

const waitForSBPacket = (byte : number, timeout = DEFAULT_TIMEOUT) => (
    waitForPacket(timeout)
        .then(packet => {
            if(packet.length !== 1 || packet.data[0] !== byte){
                throw new Error(`Unexpected SB Packet. Expected 0x${byte.toString(16)}`);
            }
        })
        .catch((e:Error) => {
            Logger.error(e.message);
            process.exit(1);
        })
);

const syncWithBootloader = async (timeout = DEFAULT_TIMEOUT) => {
    let timeWaited = 0;
    while(true){
        uart.write(SYNC_SEQ);
        await delay(1000);
        timeWaited += 1000;

        if(packets.length > 0){
            const packet = packets.splice(0,1)[0];
            if(packet.isSBPacket(BL_SYNC_OBS)){
                return;
            }
            Logger.error('Wrong Packet Observed during Sync Sequence');
            process.exit(1);
        }

        if(timeWaited >= timeout){
            Logger.error('Timed out waiting for sync sequence obs');
            process.exit(1);
        }
    }
}

const main = async () => {
    Logger.info('Reading the firmware image.');
    const fwImg = await fs.readFile(path.join(process.cwd(),'firmware.bin'))
        .then(bin => bin.slice(BOOTLOADER_SIZE));
    const fwLen = fwImg.length;
    Logger.success(`Read Firmware Image (${fwLen} bytes)`);

    Logger.info('Injecting Firmware Info');
    fwImg.writeUInt32LE(fwLen,FW_INFO_LENGTH_OFFSET); 
    fwImg.writeUInt32LE(0x00000001,FW_INFO_VERSION_OFFSET);

    const crcValue = crc32(fwImg.slice(FW_INFO_VALIDATE_FROM),fwLen - (VECTOR_TABLE_SIZE+FW_INFO_SIZE)); 
    Logger.info(`Computed CRC = 0x${crcValue.toString(16).padStart(8,'0')}`);
    fwImg.writeUInt32LE(crcValue,FW_INFO_CRC32_OFFSET);

    Logger.info('Attempting to sync with Bootloader');
    await syncWithBootloader();
    Logger.success('Synced!');

    Logger.info('Requesting FW Update');
    const FwUpdatePkt = Packet.createSBPacket(BL_FW_UPD_REQ);
    writePacket(FwUpdatePkt.toBuffer());

    await waitForSBPacket(BL_FW_UPD_RES);
    Logger.success('Firmware Update Request accepted');

    Logger.info('Waiting for Device ID Request');
    await waitForSBPacket(BL_DEV_ID_REQ);

    const DeviceID = fwImg[FW_INFO_DEV_ID_OFFSET];
    const DeviceIDPkt = new Packet(2,Buffer.from([BL_DEV_ID_RES,DeviceID]));
    writePacket(DeviceIDPkt.toBuffer());
    Logger.info(`Responded with Device ID 0x${DeviceID.toString(16)}`);

    Logger.info('Waiting for FW Len Request');
    await waitForSBPacket(BL_FW_LEN_REQ);

    const fwLenPktBuffer = Buffer.alloc(5);
    fwLenPktBuffer[0] = BL_FW_LEN_RES;
    fwLenPktBuffer.writeUInt32LE(fwLen,1);
    const fwLenPkt = new Packet(5, fwLenPktBuffer);
    writePacket(fwLenPkt.toBuffer());
    Logger.info('Responding with Firmware Length');

    Logger.info('Waiting for Erasing of Main Application');
    await delay(1000);
    Logger.info('Waiting for Erasing of Main Application');
    await delay(1000);
    Logger.info('Waiting for Erasing of Main Application');
    await delay(1000);

    let bytesWritten = 0;
    while(bytesWritten < fwLen){
        await waitForSBPacket(BL_READY_FOR_DATA);
        const fwWrite = fwImg.slice(bytesWritten,bytesWritten+DATA_BYTES);
        const dataLength = fwWrite.length;
        const dataPacket = new Packet(dataLength-1,fwWrite);
        writePacket(dataPacket.toBuffer());
        bytesWritten += dataLength;
        Logger.info(`Wrote ${dataLength} bytes (${bytesWritten}/${fwLen})`);
    }

    await waitForSBPacket(BL_FW_UPDATE_SUCCESSFUL);
    Logger.success('Firmware Update Successful & Complete');
}

main()
    .finally(()=>uart.close());