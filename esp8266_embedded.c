/*
 * Copyright (c) 2017 Jonaias Projetos e Tecnologia Ltda
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above
 *   copyright notice, this list of conditions and the following disclaimer
 *   in the documentation and/or other materials provided with the
 *   distribution.
 * * Neither the name of the  nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <inttypes.h>
#include <math.h>
#include <stdio.h>

#include "esp8266_embedded.h"
#include "lpc13xx_uart.h"

uint8_t rxBuffer[SERIAL_RX_BUFFER_SIZE];


void initEspInputBuffer(){
	circularBufferInit(&serialRxBuffer, rxBuffer, SERIAL_RX_BUFFER_SIZE);
}

uint32_t getCurrentMS (void){
	return delay_GetTickCounter();
}

void delayMS(int ms){
    uint32_t start = getCurrentMS();
    while (getCurrentMS() - start < ms);
}


//#define DEBUG
void espPrintln(int fd, const char *buf, int len){
    //len+=2;
#ifdef DEBUG
    printf("espPrintln>>");
    for(int i=0; i<len; i++){
        printf("[%x]", buf[i]);
    }
    printf("\n");
#endif
    //write(fd, buf, len);
    UART_Send(LPC_UART, buf, len, BLOCKING);
    //write(2, buf, len);
}

ssize_t espRead (int __fd, void *__buf, size_t __nbytes){
	if (__fd == SERIAL_ESP8266_FD_NUM){
		uint32_t availableBytes = circularBufferUsedElementsNum(&serialRxBuffer);
		if ( __nbytes > availableBytes){
			circularBufferGetMultiple(&serialRxBuffer, __buf, availableBytes);
			//trace_write(__buf, availableBytes);
			return availableBytes;
		}
		else{
			circularBufferGetMultiple(&serialRxBuffer, __buf, __nbytes);
			//trace_write(__buf, __nbytes);
			return __nbytes;
		}

	}
   #ifdef DEBUG
   printf("[espRead]Asked for %d, got %d=", __nbytes, num);
   for (int i = 0; i < num; ++i) {
       printf("%c[%x]",((char*)__buf)[i], ((char*)__buf)[i]);
   }
   printf("\n");
   #endif
   return 0;
}
