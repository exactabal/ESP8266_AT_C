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

#include "esp8266.h"
#include "circular_buffer.h"

#include <stdio.h>
#include <stdarg.h>


extern void delayMS(int ms);
extern void espPrintln(int fd, char *buf, int len);
extern int espRead (int __fd, void *__buf, size_t __nbytes);
extern uint32_t getCurrentMS (void);


static uint8_t buffer[CIRCULAR_BUFFER_SIZE];
static CircularBuffer circularBuffer;

char fwVersion[128] = {0};

int numClients=0;
char clients[MAX_NUMBER_OF_CLIENT][IP_BUFFER_SIZE];


const char* ESPTAGS[] =
{
    "\r\nOK\r\n",
    "\r\nERROR\r\n",
    "\r\nFAIL\r\n",
    "\r\nSEND OK\r\n",
    "ALREADY CONNECTED\r\n",
    "WIFI CONNECTED\r\n"
};


typedef enum{
    TAG_OK,
    TAG_ERROR,
    TAG_FAIL,
    TAG_SENDOK,
    TAG_ALREADY_CONNECTED,
    TAG_WIFI_CONNECTED,
    NUMESPTAGS
} TagsEnum;





bool circularBufferEndWith(const char *tag){
    uint8_t circularBufferEndWithBuff[CMD_BUFFER_SIZE];

    uint32_t len_to_peek = strlen(tag);
    if (circularBufferUsedElementsNum(&circularBuffer) < len_to_peek){
        return false;
    }


    circularBufferPeekFromEndMultiple(&circularBuffer, circularBufferEndWithBuff, len_to_peek);
    circularBufferEndWithBuff[len_to_peek] ='\0';

    //printf("[%s] <> [%s]\n",tag, buf);
    //for(int i=0;i<len_to_peek;i++){
    //    printf("t %c[%x] <> r %c[%x]\n", tag[i], tag[i], buf[i], buf[i]);
    //}

    return memcmp(tag, circularBufferEndWithBuff, len_to_peek) == 0;
}


void espEmptyBuf(int fd)
{

    char espEmptyBufBuff[50];

    int rdlen;

    while((rdlen = espRead(fd, espEmptyBufBuff, sizeof(espEmptyBufBuff) - 1)) > 0){
        espEmptyBufBuff[rdlen] = '\0';
        //printf("Discarded = [\n%s]\n", buf);
    }
    //printf("clr\n");
}


// Read from serial until one of the tags is found
// Returns:
//   the index of the tag found in the ESPTAGS array
//   -1 if no tag was found (timeout)
int espReadUntil(int fd, unsigned int timeout, const char* tag, bool findTags)
{


    circularBufferClear(&circularBuffer);

    unsigned long start = getCurrentMS();
    int ret = -1;


    while ((getCurrentMS() - start < timeout) && ret<0) {
        char c;

        //Read one char
        if(espRead(fd, &c, sizeof(c)) > 0){

            circularBufferPut(&circularBuffer, c);

            if (tag!=NULL) {
                if (circularBufferEndWith(tag)){
                    ret = NUMESPTAGS;
                }
            }
            if(findTags)
            {
                for(int i=0; i<NUMESPTAGS; i++)
                {
                    if (circularBufferEndWith(ESPTAGS[i]))
                    {
                        ret = i;
                        break;
                    }
                }
            }
        }
    }

    if (getCurrentMS() - start >= timeout)
    {
        printf("espReadUntil TIMEOUT!\n");
    }

    return ret;
}

/*
* Sends the AT command and returns the id of the TAG.
* The additional arguments are formatted into the command using sprintf.
* Return -1 if no tag is found.
*/
int espSendCmd(int fd, const char* cmd, int timeout, ...)
{

    char tmpBuf[CMD_BUFFER_SIZE];

    va_list args;
    va_start (args, timeout);
    vsnprintf(tmpBuf, CMD_BUFFER_SIZE, (char*)cmd, args);
    va_end (args);

    espPrintln(fd, tmpBuf, strlen(tmpBuf));
    //printf("espSendCmd>>%s\n",tmpBuf);

    int idx = espReadUntil(fd, timeout, NULL, true);

    return idx;
}


bool espWifiConnect(int fd, const char* ssid, const char *passphrase) {

    // TODO
    // Escape character syntax is needed if "SSID" or "password" contains
    // any special characters (',', '"' and '/')

    // connect to access point, use CUR mode to avoid connection at boot
    int ret = espSendCmd(fd, "AT+CWJAP_CUR=\"%s\",\"%s\"\r\n", 20000, ssid, passphrase);

    if (ret==TAG_WIFI_CONNECTED)
    {
        printf("Connected to %s\n", ssid);
    }
    else{
        printf("Not connected to %s\n", ssid);
        return false;
    }

    ret = espReadUntil(fd, 5000, "WIFI GOT IP\r\n", false);

    if (ret==NUMESPTAGS)
    {
        printf("Got IP from %s\n", ssid);
    }

    ret = espReadUntil(fd, 5000, NULL, true);

    if (ret==TAG_OK)
    {
        printf("Connected and RDY!\n");
        return true;
    }

    printf("Failed connecting to %s\n", ssid);

    // clean additional messages logged after the FAIL tag
    delayMS(1000);

    return false;
}



/*
* Sends the AT command and stops if any of the TAGS is found.
* Extract the string enclosed in the passed tags and returns it in the outStr buffer.
* Returns true if the string is extracted, false if tags are not found of timed out.
*/
bool espSendCmdGet(int fd, const char* cmd, const char* startTag, const char* endTag, char* outStr, int outStrLen)
{
    int idx;
    bool ret = false;

    outStr[0] = '\0';

    if (cmd){

        // send AT command to ESP
        espPrintln(fd, cmd, strlen(cmd));
    }

    // read result until the startTag is found
    idx = espReadUntil(fd, 1000, startTag, true);

    if(idx==NUMESPTAGS)
    {

        // start tag found, search the endTag
        idx = espReadUntil(fd, 500, endTag, true);

        if(idx==NUMESPTAGS)
        {
            // end tag found
            // copy result to output buffer avoiding overflow
            //ringBuf.getStrN(outStr, strlen(endTag), outStrLen-1);

            //circularBufferDiscardMultiple(&circularBuffer, strlen(endTag));
            int copied_chars = circularBufferUsedElementsNum(&circularBuffer);
            if (outStrLen<copied_chars){
                copied_chars = outStrLen;
            }
            //circularBufferGetMultiple(&circularBuffer, outStr, outStrLen-1);
            //-1 to allow space for null terminating
            circularBufferGetMultiple(&circularBuffer, outStr, copied_chars-1);
            outStr[copied_chars] = '\0';

            // read the remaining part of the response
            espReadUntil(fd, 2000, NULL, true);

            ret = true;
        }
        else
        {
            printf("End tag not found\n");
        }
    }
    else if(idx>=0 && idx<NUMESPTAGS)
    {
        // the command has returned but no start tag is found
        printf("No start tag found: %d\n", idx);
    }
    else
    {
        // the command has returned but no tag is found
        printf("No tag found\n");
    }

    return ret;
}


char* espFwVersion(int fd) {
    printf("> getFwVersion");

    espSendCmdGet(fd,"AT+GMR\r\n", "SDK version:", "\r\n", fwVersion, sizeof(fwVersion));

    return fwVersion;
}


void espReset(int fd)
{
    //printf("> reset\n");

    //TODO: Uncomment here or better to use an IO to reset ESP8266 module
    //espSendCmd(fd, "AT+RST\r\n", 1000);
    //delayMS(3000);
    delayMS(1000);
    espEmptyBuf(fd);  // empty dirty characters from the buffer

    // disable echo of commands
    espSendCmd(fd, "ATE0\r\n", 1000);

    // set station mode
    espSendCmd(fd,"AT+CWMODE=1\r\n", 1000);
    delayMS(200);

    // set multiple connections mode
    espSendCmd(fd,"AT+CIPMUX=1\r\n", 1000);

    // Show remote IP and port with "+IPD"
    espSendCmd(fd,"AT+CIPDINFO=1\r\n", 1000);

    // Disable autoconnect
    // Automatic connection can create problems during initialization phase at next boot
    espSendCmd(fd,"AT+CWAUTOCONN=0\r\n", 1000);

    // enable DHCP
    espSendCmd(fd,"AT+CWDHCP=1,1\r\n", 1000);
    delayMS(200);
}


bool espDriverInit(int fd){


    circularBufferInit(&circularBuffer, buffer, CIRCULAR_BUFFER_SIZE);

    espSendCmd(fd, "ATE0\r\n", 1000);

    bool initOK = false;

    for(int i=0; i<5; i++)
    {
        if (espSendCmd(fd, "AT\r\n", 1000) == TAG_OK)
        {
            initOK=true;
            break;
        }
        delayMS(1000);
    }

    if (!initOK)
    {
        printf("Cannot initialize ESP module\n");
        return false;
    }

    espReset(fd);

    // check firmware version - Maybe a different function
    espFwVersion(fd);

    // prints a warning message if the firmware is not 1.X
    if (fwVersion[0] != '1' || fwVersion[1] != '.') {
        printf("Warning: Unsupported firmware %s\n", fwVersion);
    }
    else
    {
        printf("Initilization successful %s\n", fwVersion);
    }
    return true;
}


bool espDriverMode(int fd, EspMode mode){

    circularBufferInit(&circularBuffer, buffer, CIRCULAR_BUFFER_SIZE);

    if ( espSendCmd(fd, "AT+CWMODE=%d\r\n", 1000, mode)  == TAG_OK){
        printf("Current mode is %d\n",mode);
        return true;
    }
    else{
        printf("Cannot set mode to %d\n",mode);
        return false;
    }

}


//<mode>
//0 : set ESP8266 soft-AP
//1 : set ESP8266 station
// 2 : set both softAP and station
//<en>
//0 : Disable DHCP
//1 : Enable DHCP

bool espDHCP(int fd, EspMode mode, int enabled){
    // from 0 to 2 here
    mode-=1;

    circularBufferInit(&circularBuffer, buffer, CIRCULAR_BUFFER_SIZE);

    if ( espSendCmd(fd, "AT+CWDHCP_DEF=%d,%d\r\n", 1000, mode, enabled)  == TAG_OK){
        printf("Current DHCP mode is %d,%d\n",mode,enabled);
        return true;
    }
    else{
        printf("Cannot set DHCP mode to %d,%d\n",mode,enabled);
        return false;
    }

}


bool espSetIPRangeDHCP(int fd, const char *startIP, const char *endIP){
    circularBufferInit(&circularBuffer, buffer, CIRCULAR_BUFFER_SIZE);

    int leaseTime = 300;

    if ( espSendCmd(fd, "AT+CWDHCPS_DEF=1,%d,\"%s\",\"%s\"\r\n", 1000, leaseTime, startIP, endIP)  == TAG_OK){
        printf("DHCP IP range set from %s to %s\n", startIP, endIP);
        return true;
    }
    else{
        printf("Cannot set DHCP IP range\n");
        return false;
    }
}

bool espSetSoftApIP(int fd, const char *softApIP){
    circularBufferInit(&circularBuffer, buffer, CIRCULAR_BUFFER_SIZE);

    if ( espSendCmd(fd, "AT+CIPAP_DEF=\"%s\",\"%s\",\"255.255.255.0\"\r\n", 1000, softApIP, softApIP)  == TAG_OK){
        printf("SoftAP IP set to %s\n", softApIP);
        return true;
    }
    else{
        printf("Cannot set IP\n");
        return false;
    }
}


void espGetIpAddress(int fd)
{

    char buf[20];
    if (espSendCmdGet(fd,"AT+CIFSR\r\n", ":STAIP,\"", "\"\r\n", buf, sizeof(buf)))
    {
        printf("Client IP:%s\n",buf);
//		char* token;

//		token = strtok(buf, ".");
//		_localIp[0] = atoi(token);
//		token = strtok(NULL, ".");
//		_localIp[1] = atoi(token);
//		token = strtok(NULL, ".");
//		_localIp[2] = atoi(token);
//		token = strtok(NULL, ".");
//		_localIp[3] = atoi(token);

//		ip = _localIp;
    }
}


void espGetIPAddressAP(int fd)
{

    char buf[20];
    if (espSendCmdGet(fd,"AT+CIPAP?\r\n", "+CIPAP:ip:\"", "\"\r\n", buf, sizeof(buf)))
    {
        printf("AP IP:%s\n",buf);
//        char* token;

//        token = strtok(buf, ".");
//        _localIp[0] = atoi(token);
//        token = strtok(NULL, ".");
//        _localIp[1] = atoi(token);
//        token = strtok(NULL, ".");
//        _localIp[2] = atoi(token);
//        token = strtok(NULL, ".");
//        _localIp[3] = atoi(token);

//        ip = _localIp;
    }
}


bool espStartAP(int fd, char* ssid, const char* pwd, uint8_t channel, uint8_t enc, bool hidden)
{
    // TODO
    // Escape character syntax is needed if "SSID" or "password" contains
    // any special characters (',', '"' and '/')

    // start access point
    int ret = espSendCmd(fd,"AT+CWSAP_DEF=\"%s\",\"%s\",%d,%d,%d,%d\r\n", 10000, ssid, pwd, channel, enc, 4, hidden);

    if (ret!=TAG_OK){
        printf("Failed to start AP with ssid:%s\n",ssid);
        return false;
    }
    else{
        printf("Started AP with ssid:%s\n",ssid);
        return true;
    }
}

bool espStartUDPServer(int fd, uint8_t conn_id, const char* dest, uint16_t remotePort, uint16_t localPort)
{

    int ret = espSendCmd(fd,"AT+CIPSTART=%d,\"UDP\",\"%s\",%u,%u,2\r\n", 1000, conn_id, dest, remotePort, localPort);

    if (ret==TAG_OK) {
        printf("UDP Server open at port %u\n", localPort);
        return true;
    }
    else if (ret==TAG_ALREADY_CONNECTED) {
        printf("UDP Server already open at port %u, cleaning ERROR msg\n", localPort);
        espReadUntil(fd, 200, NULL, true);
        return true;
    }
    else{
        return false;
    }

}


bool espStartTCPConnection(int fd, uint8_t conn_id, const char* dest, uint16_t remotePort){
    int ret = espSendCmd(fd,"AT+CIPSTART=%d,\"TCP\",\"%s\",%u\r\n", 1000, conn_id, dest, remotePort);

    if (ret==TAG_OK) {
        printf("TCP connected at port %u\n", remotePort);
        return true;
    }
    else if (ret==TAG_ALREADY_CONNECTED) {
        printf("TCP already connected at port %u, cleaning ERROR msg\n", remotePort);
        espReadUntil(fd, 200, NULL, true);
        return true;
    }
    else{
        return false;
    }
}

bool espSendTCPData(int fd, uint8_t conn_id, const char *data, int dataLen){
    char cmdBuf[128];

    snprintf(cmdBuf,128, "AT+CIPSEND=%d,%d\r\n", conn_id, dataLen);

    espPrintln(fd, cmdBuf, strlen(cmdBuf));


    int idx = espReadUntil(fd, 2000, ">", false);
    if(idx!=NUMESPTAGS)
    {
        printf("Data packet send error (1)\n");
        return false;
    }

    espPrintln(fd, data, dataLen);

    idx = espReadUntil(fd, 2000, NULL, true);
    if(idx!=TAG_SENDOK){
        printf("Data packet send error (2)\n");
        return false;
    }

    return true;
}

bool espWaitForData(int fd, unsigned int timeout, const char *host, const char *data, uint32_t *receivedLen){

    char espWaitForDataBuff[512];

    unsigned long start = getCurrentMS();

    bool foundipd = false;
    while ((getCurrentMS() - start < timeout) && !foundipd) {
        char c;

        //Read one char
        if(espRead(fd, &c, sizeof(c)) > 0){
            circularBufferPut(&circularBuffer, c);
            foundipd = circularBufferEndWith("+IPD,");
        }
    }

    if (!foundipd){
        return false;
    }

    circularBufferClear(&circularBuffer);

    start = getCurrentMS();
    bool foundstart = false;
    while ((getCurrentMS() - start < timeout) && !foundstart) {
        char c;

        //Read one char
        if(espRead(fd, &c, sizeof(c)) > 0){
            circularBufferPut(&circularBuffer, c);
            foundstart = circularBufferEndWith(":");
        }
    }

    if (!foundstart){
        return false;
    }


    int len = circularBufferUsedElementsNum(&circularBuffer);
    circularBufferGetMultiple(&circularBuffer, espWaitForDataBuff, len);
    //Remove ":"
    espWaitForDataBuff[len-1] = '\0';
    circularBufferClear(&circularBuffer);


    int conn_id=0;
    int data_len=0;
    int ip0, ip1, ip2, ip3, port;

    sscanf(espWaitForDataBuff,"%d,%d,%d.%d.%d.%d,%d", &conn_id, &data_len, &ip0, &ip1, &ip2, &ip3, &port);

    //Read data
    //int bytes_readen = espRead(fd, buff, data_len);


    int bytes_readen = 0;
    start = getCurrentMS();

    while (((getCurrentMS() - start) < timeout) && (bytes_readen != data_len)) {
		char c;

		//Read one char
		if(espRead(fd, &c, sizeof(c)) > 0){
			*(espWaitForDataBuff+bytes_readen) = c;
			bytes_readen++;
		}
	}

    //printf("\n%d %d %d.%d.%d.%d:%d ", conn_id, data_len, ip0, ip1, ip2, ip3, port);
    //buff[bytes_readen] = '\0';
    //printf("Data:%s\n",buff);

    if (host){
        sprintf(host,"%d.%d.%d.%d",ip0, ip1, ip2, ip3);
    }

    if (data){
        memcpy(data, espWaitForDataBuff, bytes_readen);
    }

    if (receivedLen){
        *receivedLen = bytes_readen;
    }

    return true;
}



bool espSendData(int fd, uint8_t conn_id, const char* dest, uint16_t remotePort, const char *data, int dataLen){

    char cmdBuf[50];
    snprintf(cmdBuf,50, "AT+CIPSEND=%d,%d,\"%s\",%d\r\n", conn_id, dataLen, dest, remotePort);

    espPrintln(fd, cmdBuf, strlen(cmdBuf));


    int idx = espReadUntil(fd, 200, ">", false);
    if(idx!=NUMESPTAGS)
    {
        printf("Data packet send error (1)\n");
        return false;
    }

    espPrintln(fd, data, dataLen);

    idx = espReadUntil(fd, 200, NULL, true);
    if(idx!=TAG_SENDOK){
        printf("Data packet send error (2)\n");
        return false;
    }

    return true;
}


int espGetConnectedClients(int fd){

    char cmdBuf[] = "AT+CWLIF\r\n";
    espPrintln(fd, cmdBuf, strlen(cmdBuf));

    int idx;
    numClients = 0;
    do{
        idx = espReadUntil(fd, 200, ",", true);

        if (idx == NUMESPTAGS){
            /* Do not get "," therefore -1 */
            int numElem = circularBufferUsedElementsNum(&circularBuffer)-1;

            /* Force max ip size to IP_BUFFER_SIZE */
            numElem = numElem>IP_BUFFER_SIZE?IP_BUFFER_SIZE:numElem;

            circularBufferGetMultiple(&circularBuffer, clients[numClients], numElem);
            clients[numClients][numElem] = '\0';
            numClients++;

            /* Consume line upto end */
            idx = espReadUntil(fd, 200, "\r\n", false);
        }
    } while(idx==NUMESPTAGS);

    //printf("Found %d clients\n", numClients);
    //for (int i =0;i<numClients;i++){
    //    printf("Client %d: %s\n",i, clients[i]);
    //}

    return numClients;
}

char* espGetConnectedClient(int clientNum){
    return clients[clientNum];
}

int espGetNumConnectedClients(){
    return numClients;
}


bool espGetConnectedAP(int fd, char *ssid, uint32_t len){
    char espGetConnectedAPBuff[200];

    if (espSendCmdGet(fd,"AT+CWJAP_CUR?\r\n", "+CWJAP_CUR:", "\r\n", espGetConnectedAPBuff, sizeof(espGetConnectedAPBuff)))
    {
        for( int i=1; i <sizeof(espGetConnectedAPBuff); i++ ){
            if (espGetConnectedAPBuff[i]=='\"'){
                strncpy(ssid, espGetConnectedAPBuff+1, i-1);
                ssid[i-1] = '\0';
                return true;
            }
        }
    }
    return false;
}
