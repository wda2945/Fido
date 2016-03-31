/*
 * agent.h
 *
 *  Created on: Aug 7, 2015
 *      Author: martin
 */

#ifndef AGENT_AGENT_H_
#define AGENT_AGENT_H_

int AgentInit();

bool AgentProcessMessage(psMessage_t *msg);

extern bool agentOnline;

#endif /* AGENT_AGENT_H_ */
