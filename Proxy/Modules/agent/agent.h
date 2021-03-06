/*
 * agent.h
 *
 *  Created on: Aug 7, 2015
 *      Author: martin
 */

#ifndef AGENT_AGENT_H_
#define AGENT_AGENT_H_

#include <stdbool.h>
#include "pubsubdata.h"

int AgentInit();
void SendAgentStats();

bool AgentProcessMessage(psMessage_t *msg);

int pingServer(char *target);

extern int agentOnline;

#endif /* AGENT_AGENT_H_ */
