/*
 * agent.c
 *
 *  Created on: Aug 7, 2015
 *      Author: martin
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
//#include <stropts.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include "softwareProfile.h"
#include "pubsubdata.h"
#include "pubsub/pubsub.h"
#include "syslog/syslog.h"
#include "helpers.h"
#include "pubsubparser.h"

#include "agent/agent.h"

FILE *agentDebugFile;

#ifdef AGENT_DEBUG
#define DEBUGPRINT(...) fprintf(stdout, __VA_ARGS__);fprintf(agentDebugFile, __VA_ARGS__);fflush(agentDebugFile);
#else
#define DEBUGPRINT(...) fprintf(agentDebugFile, __VA_ARGS__);fflush(agentDebugFile);
#endif

#define ERRORPRINT(...) fprintf(stdout, __VA_ARGS__);fprintf(agentDebugFile, __VA_ARGS__);fflush(agentDebugFile);

#define MAX_AGENT_CONNECTIONS 10

int agentOnline = 0;

bool connected[MAX_AGENT_CONNECTIONS];				//whether channel is in use
int rxSocket[MAX_AGENT_CONNECTIONS];				//socket used by rx thread
int txSocket[MAX_AGENT_CONNECTIONS];				//socket used by tx thread

pthread_mutex_t	agentMtx = PTHREAD_MUTEX_INITIALIZER;

//queue for outgoing messages
BrokerQueue_t agentQueue = BROKER_Q_INITIALIZER;	//queue for messages to be sent

//Agent threads
void *AgentListenThread(void *arg);					//listen thread spawns tx & rx threads for each connect
pthread_t listenThread;
pthread_t txThread;
void *AgentRxThread(void *arg);						//rx thread function
void *AgentTxThread(void *arg);						//tx thread function

#define LISTEN_PORT_NUMBER 50000

int AgentInit()
{
    {
        char buff[100];
        sprintf(buff, "%s%s", LOGFILE_FOLDER, "agent.log");
        
        agentDebugFile = fopen(buff, "w");
    }
    
	DEBUGPRINT("agent Logfile opened\n");

	//create agent Listen thread
	int s = pthread_create(&listenThread, NULL, AgentListenThread, NULL);
	if (s != 0) {
		LogError("agent Listen thread create failed. %i\n", s);
		return -1;
	}

	//create agent Tx thread
	s = pthread_create(&txThread, NULL, AgentTxThread, NULL);
	if (s != 0) {
		LogError("agent Tx create failed. %s\n", strerror(s));
		return -1;
	}

	return 0;
}

//thread to listen for connect requests
void *AgentListenThread(void *arg)
{
	pthread_t rxThread[MAX_AGENT_CONNECTIONS];

	struct sockaddr client_address;
	socklen_t client_address_len;

	//initialize the data structures
	{
		for (int i=0; i<MAX_AGENT_CONNECTIONS; i++)
		{
			rxSocket[i] = -1;
			txSocket[i] = -1;
			rxThread[i] = (pthread_t) -1;
			connected[i] = false;
		}
	}
	DEBUGPRINT("agent Listen thread\n");

	//create listen socket
	int listenSocket;

	while ((listenSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		ERRORPRINT("socket() error: %s\n", strerror(errno));
		sleep(1);
	}
	int optval = 1;
	setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &optval, 4);

	DEBUGPRINT("agent listen socket created\n");

	//bind socket address
	struct sockaddr_in my_address;
	memset(&my_address, 0, sizeof(my_address));
	my_address.sin_family = AF_INET;
	my_address.sin_port = htons(LISTEN_PORT_NUMBER);
	my_address.sin_addr.s_addr = INADDR_ANY;

	while (bind(listenSocket, (struct sockaddr*) &my_address, sizeof(my_address)) == -1)
	{
		ERRORPRINT("bind() error: %s\n", strerror(errno));

		if (errno == EADDRINUSE) sleep(10);

		sleep(1);
	}

	DEBUGPRINT("agent listen socket ready\n");

	while(1)
	{
		//wait for connect
		while(listen(listenSocket, 2) != 0)
		{
			ERRORPRINT("listen() error %s\n", strerror(errno));
			sleep(1);
			//ignore errors, just retry
		}

		client_address_len = sizeof(client_address);
		int acceptSocket = accept(listenSocket, &client_address, &client_address_len);

		if (acceptSocket >= 0)
		{
			//print the address
			struct sockaddr_in *client_address_in = (struct sockaddr_in *) &client_address;

			uint8_t addrBytes[4];
			memcpy(addrBytes, &client_address_in->sin_addr.s_addr, 4);

			DEBUGPRINT("agent connect from %i.%i.%i.%i\n", addrBytes[0], addrBytes[1], addrBytes[2], addrBytes[3]);

			//find a free thread

			int i;
			int channel = -1;
			for (i=0; i<MAX_AGENT_CONNECTIONS; i++)
			{
				if (!connected[i])
				{
					channel = i;
					break;
				}
			}

			if (channel < 0)
			{
    			pthread_mutex_unlock(&agentMtx);

				DEBUGPRINT("agent listen: no available server context\n");
			}
			else
			{

				rxSocket[channel] = acceptSocket;
				txSocket[channel] = dup(acceptSocket);	//separate rx & tx sockets

 				//create agent Rx thread
				int s;
				do {
					s = pthread_create(&rxThread[channel], NULL, AgentRxThread, (void*) channel);
					if (s == EAGAIN) sleep(1);
				}
				while (s == EAGAIN);
				if (s != 0) {
					LogError("agent %i Rx create failed - %s\n", channel, strerror(s));

					close(rxSocket[channel]);
					close(txSocket[channel]);
				}
				else
				{
					DEBUGPRINT("agent %i Rx thread created.\n", channel);
				}

			}
		}
	}
	return 0;
}

//thread function for Rx
void *AgentRxThread(void *arg)
{
	int mychan = (int) arg;
	int socket = rxSocket[mychan];

	psMessage_t rxMessage;
	status_t parseStatus;

	DEBUGPRINT("agent %i Rx thread: fd= %i\n",mychan, socket);

    parseStatus.noCRC       = 0; ///< Do not expect a CRC, if > 0
    parseStatus.noSeq       = 0; ///< Do not check seq #s, if > 0
    parseStatus.noLength2   = 0; ///< Do not check for duplicate length, if > 0
    parseStatus.noTopic     = 1; ///< Do not check for topic ID, if > 0

    ResetParseStatus(&parseStatus);

	agentOnline++;
	connected[mychan] = true;

    while(connected[mychan])
    {
    	uint8_t readByte;
    	int result;

    	do {
    		if (recv(socket, &readByte, 1, 0) <= 0)
    		{
    			//quit on failure, EOF, etc.
    			ERRORPRINT("agent Rx %i recv() fd=%i error: %s\n", mychan, socket, strerror(errno));

    			pthread_mutex_lock(&agentMtx);

    			close(socket);
    			rxSocket[mychan] = -1;

    		    close(txSocket[mychan]);
    		    txSocket[mychan] = -1;
    			connected[mychan] = false;

       			agentOnline--;

    			pthread_mutex_unlock(&agentMtx);

    		    pthread_exit(NULL);

    			return 0;
    		}
    		result = ParseNextCharacter(readByte, &rxMessage, &parseStatus);
    	} while (result == 0);

    	if (rxMessage.header.messageType == KEEPALIVE)
    	{
    		if (loopback)
    		{
    			rxMessage.header.source = 0;
    			CopyMessageToQ(&agentQueue, &rxMessage);
    		}
    		else
    		{
    			XBeeBrokerProcessMessage(&rxMessage);
    		}
    	}
    	else
    	{
			DEBUGPRINT("agent %i. %s received from App\n", mychan, psLongMsgNames[rxMessage.header.messageType]);

			rxMessage.header.source = APP_XBEE;
			RouteMessage(&rxMessage);
    	}
    }
    //Tx disconnected
    DEBUGPRINT("agent %i Rx fd=%i exiting.\n", mychan, socket);

	pthread_mutex_lock(&agentMtx);

    close(socket);
    rxSocket[mychan] = -1;

	agentOnline--;

	pthread_mutex_unlock(&agentMtx);

    pthread_exit(NULL);

    return 0;
}

//Queue a copy of a message for each active channel
bool AgentProcessMessage(psMessage_t *msg)
{
	if ((msg->header.source == APP_XBEE) || (msg->header.source == APP_OVM) || (msg->header.source == ROBO_APP)) return false;

	//filter?
	CopyMessageToQ(&agentQueue, msg);

	return true;
}

//Tx thread convenience function
int writeToSocket(int socket, uint8_t c, int *checksum, int *error)
{
	ssize_t reply;
	do {
		reply = send(socket, &c, 1, 0);
	} while (reply == 0);

	if (reply == 1)
	{
		*checksum += c;
		return 0;
	}
	else
	{
		*error = (int)reply;
		return (int) reply;
	}
}

//Tx thread function
void *AgentTxThread(void *arg)
{
	int errorReply = 0;

	int checksum;
	uint8_t msgSequence[MAX_AGENT_CONNECTIONS];

	for (int i=0; i<MAX_AGENT_CONNECTIONS; i++) msgSequence[i] = 0;

	//loop
	while (1)
	{
		//wait for next message
		psMessage_t *txMessage = GetNextMessage(&agentQueue);

		for (int i=0; i<MAX_AGENT_CONNECTIONS; i++)
		{

			if (connected[i])
			{

    			pthread_mutex_lock(&agentMtx);

				int socket = txSocket[i];

				//send STX
				writeToSocket( socket, STX_CHAR, &checksum, &errorReply);
				checksum = 0; //checksum starts from here
				//send header
				writeToSocket( socket, txMessage->header.length, &checksum, &errorReply);
				writeToSocket( socket, ~txMessage->header.length, &checksum, &errorReply);
				writeToSocket( socket, msgSequence[i]++, &checksum, &errorReply);
				writeToSocket( socket, txMessage->header.source, &checksum, &errorReply);
				writeToSocket( socket, txMessage->header.messageType, &checksum, &errorReply);
				//send payload
				uint8_t *buffer = (uint8_t *) &txMessage->packet;
				uint8_t size = txMessage->header.length;

				if (size > sizeof(psMessage_t) - SIZEOF_HEADER)
				{
					size = txMessage->header.length = sizeof(psMessage_t) - SIZEOF_HEADER;
				}

				while (size) {
					writeToSocket( socket, *buffer, &checksum, &errorReply);
					buffer++;
					size--;
				}
				//write checksum
				writeToSocket( socket, (checksum & 0xff), &checksum, &errorReply);

				if (errorReply != 0)
				{
					ERRORPRINT("agent Tx (fd:%i) send error: %s\n", socket, strerror(errno));
					close(socket);
					txSocket[i] = -1;
					connected[i] = false;
				}
				else
				{
					DEBUGPRINT("agent: %s sent to App\n", psLongMsgNames[txMessage->header.messageType]);
				}

    			pthread_mutex_unlock(&agentMtx);

			}
		}
		DoneWithMessage(txMessage);
	}
	return 0;
}

