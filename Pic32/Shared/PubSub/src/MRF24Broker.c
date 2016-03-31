/*
 * File:   MRF24Broker.c
 * Author: martin
 *
 * Created on November 22, 2013
 */

//Forwards PubSub messages via Radio to other MCUs

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#define _SUPPRESS_PLIB_WARNING
#include "plib.h"

#include "FreeRTOS.h"
#include "task.h"

#include "SoftwareProfile.h"
#include "HardwareProfile.h"

#include "SysLog/SysLog.h"
#include "PubSub/PubSub.h"
#include "PubSub/PubSubData.h"

#include "WirelessProtocols/MCHP_API.h"

static void MRF24Task(void *pvParameters);

PubSubQueue_t mrfQueue; //Queue for messages to be sent over the radio

extern RECEIVED_MESSAGE rxMessage;

void MRF24BrokerInit() {

    while ((mrfQueue = psNewPubSubQueue(MRF24_QUEUE_LENGTH)) == NULL) {
        LogError("MRF24 Q");
        vTaskDelay(10000);
    }

    /* Create the Tx task */
    if (xTaskCreate(MRF24Task, /* The function that implements the task. */
            (signed char *) "MRF24", /* The text name assigned to the task - for debug only as it is not used by the kernel. */
            MRF24_TASK_STACK_SIZE, /* The size of the stack to allocate to the task. */
            (void *) 0, /* The parameter passed to the task. */
            MRF24_TASK_PRIORITY, /* The priority assigned to the task. */
            NULL) /* The task handle is not required, so NULL is passed. */
            != pdPASS) {
        LogError("MRF24 Task");
        for (;;);
    }
}

void MRF24Task(void *pvParameters) {
    //Init MiWi
    /*******************************************************************/
    // Function MiApp_ProtocolInit initialize the protocol stack. The
    // only input parameter indicates if previous network configuration
    // should be restored. In this simple example, we assume that the
    // network starts from scratch.
    /*******************************************************************/
    MiApp_ProtocolInit(FALSE);

    //TODO: manage channels & connections
    //TODO: scan for peers
    //TODO: scan for lowest noise

    // Set default channel
    MiApp_SetChannel(0);

    /*******************************************************************/
    // Function MiApp_ConnectionMode defines the connection mode. The
    // possible connection modes are:
    //  ENABLE_ALL_CONN:    Enable all kinds of connection
    //  ENABLE_PREV_CONN:   Only allow connection already exists in
    //                      connection table
    //  ENABL_ACTIVE_SCAN_RSP:  Allow response to Active scan
    //  DISABLE_ALL_CONN:   Disable all connections.
    /*******************************************************************/
    MiApp_ConnectionMode(ENABLE_ALL_CONN);

    /*******************************************************************/
    // Function MiApp_EstablishConnection try to establish a new
    // connection with a peer device.
    // The first parameter is the index to the active scan result,
    //      which is acquired by discovery process (active scan). If
    //      the value of the index is 0xFF, try to establish a
    //      connection with any peer.
    // The second parameter is the mode to establish connection,
    //      either direct or indirect. Direct mode means connection
    //      within the radio range; indirect mode means connection
    //      may or may not in the radio range.
    /*******************************************************************/
    int i = MiApp_EstablishConnection(0xFF, CONN_MODE_DIRECT);

    if (i == 0xFF) {
        /*******************************************************************/
        // If no network can be found and join, we need to start a new
        // network by calling function MiApp_StartConnection
        //
        // The first parameter is the mode of start connection. There are
        // two valid connection modes:
        //   - START_CONN_DIRECT        start the connection on current
        //                              channel
        //   - START_CONN_ENERGY_SCN    perform an energy scan first,
        //                              before starting the connection on
        //                              the channel with least noise
        //   - START_CONN_CS_SCN        perform a carrier sense scan
        //                              first, before starting the
        //                              connection on the channel with
        //                              least carrier sense noise. Not
        //                              supported for current radios
        //
        // The second parameter is the scan duration, which has the same
        //     definition in Energy Scan. 10 is roughly 1 second. 9 is a
        //     half second and 11 is 2 seconds. Maximum scan duration is
        //     14, or roughly 16 seconds.
        //
        // The third parameter is the channel map. Bit 0 of the
        //     double word parameter represents channel 0. For the 2.4GHz
        //     frequency band, all possible channels are channel 11 to
        //     channel 26. As the result, the bit map is 0x07FFF800. Stack
        //     will filter out all invalid channels, so the application
        //     only needs to pay attention to the channels that are not
        //     preferred.
        /*******************************************************************/
        MiApp_StartConnection(START_CONN_DIRECT, 10, 0);
    }

    //send a subscribe msg
//    psSubscribeToTopic(ANNOUNCEMENTS_TOPIC, mrfQueue);

    for (;;) {
        psMessage_t msg;
        char serialMsg[MAX_MRF24_MESSAGE];
        char *buffer;
        int size;

        LogRoutine("MRF Task Up");

        //wait for a message
        if (xQueueReceive(mrfQueue, &msg, 200) == pdTRUE) {
            //tx message received
            msgToText(&msg, serialMsg, MAX_MRF24_MESSAGE);

            switch (msg.header.messageType) {
                default:
                    LogRoutine("MRF Tx: %s",
                            psMsgNames[msg.header.messageType]);

                case SYSLOG_MSG: //don't log log messages!!

                    buffer = (char*) &serialMsg;
                    size = sizeof (serialMsg);
                    MiApp_FlushTx();

                    while (size) {
                        MiApp_WriteData(*buffer);
                        buffer++;
                        size--;
                    }
                    MiApp_BroadcastPacket(0);

                    break;

                    //TODO: Housekeeping messages such as power save
            }
        } else {
            BYTE *payload;
            int payloadSize, msgSize;
            //check for rx message
            if (MiApp_MessageAvailable()) {

                payload = rxMessage.Payload;
                payloadSize = rxMessage.PayloadSize;

                textToMsg(payload, msg);

                if (msg.header.messageType != SYSLOG_MSG) //don't log log messages!!
                    LogRoutine("MRF Rx: %s", psMsgNames[msg.header.messageType]);

                //prepare the message for forwarding
                 psSendMessage(msg);

            }
        }
    }
}
