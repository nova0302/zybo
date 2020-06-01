/******************************************************************************
 *
 * Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * Use of the Software is limited solely to applications:
 * (a) running on a Xilinx device, or
 * (b) that interact with a Xilinx device through a bus or interconnect.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the Xilinx shall not be used
 * in advertising or otherwise to promote the sale, use or other dealings in
 * this Software without prior written authorization from Xilinx.
 *
 ******************************************************************************/

/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"

#include "sleep.h"
#include "xiicps.h"
#include "xgpiops.h"

//http://todotani.cocolog-nifty.com/blog/2016/09/zybopsi2c-615d.html
// I2C parameters
#define IIC_SCLK_RATE       100000  // clock 100KHz
#define TMP102_ADDRESS      0x48    // 7bit address
#define IIC_DEVICE_ID       XPAR_XIICPS_0_DEVICE_ID

XIicPs Iic;

int Init() {
	int Status;
	XIicPs_Config *Config; /**< configuration information for the device */

	Config = XIicPs_LookupConfig(IIC_DEVICE_ID);
	if (Config == NULL) {
		printf("Error: XIicPs_LookupConfig()\n");
		return XST_FAILURE;
	}

	Status = XIicPs_CfgInitialize(&Iic, Config, Config->BaseAddress);
	if (Status != XST_SUCCESS) {
		printf("Error: XIicPs_CfgInitialize()\n");
		return XST_FAILURE;
	}

	Status = XIicPs_SelfTest(&Iic);
	if (Status != XST_SUCCESS) {
		printf("Error: XIicPs_SelfTest()\n");
		return XST_FAILURE;
	}

	XIicPs_SetSClk(&Iic, IIC_SCLK_RATE);
	printf("I2C configuration done.\n");

	return XST_SUCCESS;
}

int i2c_write(XIicPs *Iic, u8 command, u16 i2c_adder) {
	int Status;
	u8 buffer[4];
	buffer[0] = command;

	Status = XIicPs_MasterSendPolled(Iic, buffer, 1, i2c_adder);

	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	// Wait until bus is idle to start another transfer.
	while (XIicPs_BusIsBusy(Iic)) {
		/* NOP */
	}

	return XST_SUCCESS;
}

int i2c_read(XIicPs *Iic, u8* buff, u32 len, u16 i2c_adder) {
	int Status;

	Status = XIicPs_MasterRecvPolled(Iic, buff, len, i2c_adder);

	if (Status == XST_SUCCESS)
		return XST_SUCCESS;
	else
		return -1;
}

void myDelay() {
	int counter = 0;
	while (counter++ < 20000000)
		;
}

int main() {
	int Status, delay, count;
	XGpioPs_Config *ConfigPtr;
	XGpioPs psGpio;
	u32 ledState = 0, Led = 7, Btn4 = 50;

	init_platform();

	for (int i = 0; i < 8; i++)
		printf("%d: Hello World\n\r", i);

	ConfigPtr = XGpioPs_LookupConfig(XPAR_PS7_GPIO_0_DEVICE_ID);

	Status = XGpioPs_CfgInitialize(&psGpio, ConfigPtr, ConfigPtr->BaseAddr);

	XGpioPs_SetDirectionPin(&psGpio, Led, 1);
	XGpioPs_SetOutputEnablePin(&psGpio, Led, 1);

	XGpioPs_SetDirectionPin(&psGpio, Btn4, 0);

	while (1) {
		while (XGpioPs_ReadPin(&psGpio, Btn4))
			;
		myDelay();
		XGpioPs_WritePin(&psGpio, Led, ledState);
		ledState = !ledState;
	}

	cleanup_platform();
	return 0;
}

