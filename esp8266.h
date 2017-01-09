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
#ifndef ESP8266_H
#define ESP8266_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CMD_BUFFER_SIZE 256

#define CIRCULAR_BUFFER_SIZE 512

#define MAX_NUMBER_OF_CLIENT 4
#define IP_BUFFER_SIZE 16

//bool debug= false;

/* Esp mode*/
typedef enum{
        MODE_STA = 1,
        MODE_AP = 2,
        MODE_STA_AP = 3
}EspMode;



/* Encryption modes */
enum wl_enc_type {
    ENC_TYPE_NONE = 0,
    ENC_TYPE_WEP = 1,
    ENC_TYPE_WPA_PSK = 2,
    ENC_TYPE_WPA2_PSK = 3,
    ENC_TYPE_WPA_WPA2_PSK = 4
};

typedef enum {TCP_MODE, UDP_MODE, SSL_MODE} ProtocolMode;


void espEmptyBuf(int fd);
bool espDriverInit(int fd);
bool espWifiConnect(int fd, const char* ssid, const char *passphrase);
bool espDriverMode(int fd, EspMode mode);
/* Tx power [0,82] each step 0.25 dBm.. not very precise according to documentation */
bool espTxPower(int fd, int txPower);
bool espDHCP(int fd, EspMode mode, int enabled);
void espGetIpAddress(int fd);
void espGetIPAddressAP(int fd);
bool espStartAP(int fd, char* ssid, const char* pwd, uint8_t channel, uint8_t enc, bool hidden);
bool espStartUDPServer(int fd, uint8_t conn_id, const char* dest, uint16_t remotePort, uint16_t localPort);
bool espStartTCPConnection(int fd, uint8_t conn_id, const char* dest, uint16_t remotePort);
bool espSendTCPData(int fd, uint8_t conn_id, const char *data, int dataLen);
bool espWaitForData(int fd, unsigned int timeout, char *host, char *data, uint32_t *receivedLen);
bool espSendData(int fd, uint8_t conn_id, const char* dest, uint16_t remotePort, const char *data, int dataLen);
bool espSetIPRangeDHCP(int fd, const char *startIP, const char *endIP);
bool espSetSoftApIP(int fd, const char *softApIP);
int espGetConnectedClients(int fd);
char* espGetConnectedClient(int clientNum);
int espGetNumConnectedClients();
bool espGetConnectedAP(int fd, char *data, uint32_t size);
bool espCloseConnection(int fd, uint8_t conn_id);

#ifdef __cplusplus
}
#endif

#endif // ESP8266_H
