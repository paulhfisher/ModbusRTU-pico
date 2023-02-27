/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "pico/stdlib.h"


#define LIGHTMODBUS_SLAVE_FULL
#define LIGHTMODBUS_DEBUG
#define LIGHTMODBUS_IMPL
#define debug_uart(...) uart_puts(UART_ID, __VA_ARGS__);
#include "liblightmodbus/include/lightmodbus/lightmodbus.h"

#ifndef REG_COUNT
#define REG_COUNT 16
#endif

#define UART_ID uart1
#define BAUDRATE 9600
#define UART_TX_PIN 4
#define UART_RX_PIN 5
#define MAX_LENGTH_W 23


typedef enum StateOfSlave
{
	STATE_IDLE,
	STATE_RECEIVING,
	STATE_WAITING,
	STATE_WAITING_INVALID,
} StateOfSlave;

uint8_t coils[REG_COUNT / 8];
uint16_t regs[REG_COUNT];

// prototype functions
void init(const uint led_used);
void decodeFrame(char reception[], int size, int sizeOfData);
void hexToASCII (char usefulData[]);
ModbusError registerCallback(const ModbusSlave *slaveID,const ModbusRegisterCallbackArgs *args,ModbusRegisterCallbackResult *result);
ModbusError exceptionCallback(const ModbusSlave *slave,  uint8_t function, ModbusExceptionCode code);
void printErrorInfo(ModbusErrorInfo err);
void printAndSendFrameResponse(ModbusErrorInfo err, const ModbusSlave *slave);




int main() {

    const uint LED_PIN = 25;
    char *ptr;
    char receiveBuffer[MAX_LENGTH_W];
    char betterArray[MAX_LENGTH_W];
    char single;
    int i_get = 0;
    char str[50];
    ModbusSlave slave;
    ModbusErrorInfo error;

    stdio_init_all();
    uart_init(UART_ID, BAUDRATE);
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    debug_uart("begining of program..\r\n");



    error = modbusSlaveInit(&slave, registerCallback, exceptionCallback, modbusDefaultAllocator, modbusSlaveDefaultFunctions, modbusSlaveDefaultFunctionCount);
    assert(modbusIsOk(error) && "modbusSlaveInit() failed!");

    StateOfSlave state = STATE_IDLE;

    while(1){
        // reset receive frame and local variable

    
        memset(receiveBuffer, i_get, sizeof(receiveBuffer));

        i_get=0;
        while ((single = getchar()) != '\n' && i_get < MAX_LENGTH_W) {
            receiveBuffer[i_get] = single;
            i_get++;
            gpio_put(LED_PIN,1);
            debug_uart("LED on\r\n");
        }

        debug_uart("outside while\r\n");
        receiveBuffer[i_get] = '\0';
        sprintf(str, "data%02X\r\n", receiveBuffer);
        debug_uart(str);

        sleep_ms(1000);

        // fgets(receiveBuffer, MAX_LENGTH, stdin);

        // printf frame
        debug_uart("RTU frame before treatment by library ..\r\n");
        for(int i = 0; i<MAX_LENGTH_W; i++){
            debug_uart("printing the frame\r\n");
            if((receiveBuffer[i_get] != '\0' || receiveBuffer[i_get] != '\n')){
                printf(" %02X", receiveBuffer[i]);
            }else{
                debug_uart("problem in receiving the frame\r\n");
            }
        }
        gpio_put(LED_PIN,0);

        error = modbusParseRequestRTU(&slave, 0x01, receiveBuffer, i_get);
        printErrorInfo(error);
        debug_uart("RTU response from lib:\r\n ");
        printAndSendFrameResponse(error, &slave);

    }

      return 0;
}
void init(const uint led_used){
    stdio_init_all();
    uart_init(UART_ID, BAUDRATE);
    gpio_init(led_used);
    gpio_set_dir(led_used, GPIO_OUT);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

}


void printAndSendFrameResponse(ModbusErrorInfo err , const ModbusSlave *slave){
    char *data;
    int length;
	for (int i = 0; i < modbusSlaveGetResponseLength(slave); i++){
        printf("0x%02x ", modbusSlaveGetResponse(slave)[i]);
        data[i] = modbusSlaveGetResponse(slave)[i];
    }
    length = modbusSlaveGetResponseLength(slave); // get the length of frame
    sprintf(data, "%d", length);

}

/*
* goes in this callback when a frame is received
*/
ModbusError registerCallback(const ModbusSlave *slaveID,const ModbusRegisterCallbackArgs *args,ModbusRegisterCallbackResult *result){
	printf(
		"Register query:\n"
		"\tquery: %s\n"
		"\t type: %s\n"
		"\t   id: %d\n"
		"\tvalue: %c\n"
		"\t  fun: %d\n",
		modbusRegisterQueryStr(args->query),
		modbusDataTypeStr(args->type),
		args->index,
		args->value,
		args->function
	);

    debug_uart("inside callback\r\n");
	switch (args->query){
    debug_uart("inside switch\r\n");

		case MODBUS_REGQ_R_CHECK:
            debug_uart("MODBUS_REGQ_R_CHECK\r\n");
			if (args->index < REG_COUNT)
				result->exceptionCode = MODBUS_EXCEP_NONE;
			else	
				result->exceptionCode = MODBUS_EXCEP_ILLEGAL_ADDRESS;
			break;
		case MODBUS_REGQ_W_CHECK:
        debug_uart("MODBUS_REGQ_W_CHECK\r\n");
			if (args->index < REG_COUNT - 2)
				result->exceptionCode = MODBUS_EXCEP_NONE;
			else	
				result->exceptionCode = MODBUS_EXCEP_SLAVE_FAILURE;
			break;

		case MODBUS_REGQ_R:
            debug_uart("MODBUS_REGQ_R\r\n");
			switch (args->type){
				case MODBUS_HOLDING_REGISTER: result->value = regs[args->index]; break;
				case MODBUS_INPUT_REGISTER: result->value = regs[args->index]; break;
				case MODBUS_COIL: result->value = modbusMaskRead(coils, args->index); break;
				case MODBUS_DISCRETE_INPUT: result->value = modbusMaskRead(coils, args->index); break;
			}
			break;


		case MODBUS_REGQ_W:
            debug_uart("MODBUS_REGQ_W\r\n");
			switch (args->type)
			{
				case MODBUS_HOLDING_REGISTER: regs[args->index] = args->value; break;
				case MODBUS_COIL: modbusMaskWrite(coils, args->index, args->value); break;
				default: abort(); break;
			}
			break;
		default: break;
	}
	return MODBUS_OK;
}

ModbusError exceptionCallback(const ModbusSlave *slave,  uint8_t function, ModbusExceptionCode code){
	printf("Slave exception %s (function %d)\n", modbusExceptionCodeStr(code), function);
	return MODBUS_OK;
}

/*
* check if modbus is initialized
*/
void printErrorInfo(ModbusErrorInfo err)
{
	if (modbusIsOk(err)){
		debug_uart("FRAME IS CORRECT\r\n");
    }else{
        debug_uart( "THERE IS A PROBLEM WITH THE FRAME\r\n");
		printf("%s: it comes from the following element : %s",
			modbusErrorSourceStr(modbusGetErrorSource(err)),
			modbusErrorStr(modbusGetErrorCode(err)));
    }
}

/*
* https://rapidscada.net/modbus/
*/
void decodeFrame(char reception[], int size, int sizeOfDataFrame){

    char slaveID;
    char functionCode;
    char addressStart;
    char sizeOfData;
    char byteCount;
    char data[sizeOfDataFrame];
    char checksum[2];
    int j = 0;
    
    slaveID = reception[0];
    functionCode = reception[1];
    addressStart = reception[3];
    sizeOfData = reception[5];
    byteCount = reception[6];
    checksum[0]= reception[size - 2];
    checksum[1] = reception[size - 1];

    for(int i = 7; i < byteCount + 7; i++){
        data[j] = reception[i];
        printf(" %02X", data[j]);
        j++;
    }

   // hexToASCII(data);

    debug_uart("\n");
    debug_uart("frame format : \n");
    debug_uart (" slave address -- function code -- starting address -- quantity -- byte count -- data -- CRC \n");
    printf ("The value of slave ID is %02X \n the value of function code is %02X \n the value of starting address is %02X \n the value of quantity is %02X \n  the value of byte count is %02X \n the CRC bytes are %02X and %02X \n", slaveID, functionCode, addressStart, sizeOfData, byteCount, checksum[0], checksum[1]);
    sleep_ms(2000);
}


void hexToASCII(char usefulData[]){
    char dataInASCII[14];

    for(int i =0; i< strlen(usefulData); i+=2){
        sscanf(&usefulData[i], "%2x", &dataInASCII[i/2]);
    }

    printf("data in interpretable String : ");
    for(int i = 0; i< sizeof(dataInASCII); i++){
        printf("%s", dataInASCII[i]);
    }
    printf("\n");


}