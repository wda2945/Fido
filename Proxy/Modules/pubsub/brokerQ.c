/*
 * brokerQ.c
 *
 * Internal message queues for the broker process
 *
 *  Created on: Jul 8, 2014
 *      Author: martin
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

#include "pubsubdata.h"
#include "brokerq.h"
#include "syslog/syslog.h"
#include "broker_debug.h"


BrokerQueueEntry_t *freelist = NULL;					//common free list
pthread_mutex_t	freeMtx = PTHREAD_MUTEX_INITIALIZER;	//freelist mutex

//private
BrokerQueueEntry_t *NewQueueEntry();
void AddToFreelist(BrokerQueueEntry_t *e);
BrokerQueueEntry_t *GetFreeEntry();

//pre-allocate some entries on init
int BrokerQueueInit(int pre)
{
	int i;
	for (i=0; i<pre; i++)
	{
		BrokerQueueEntry_t *entry = NewQueueEntry();
		if (entry == NULL) return -1;
		AddToFreelist(entry);
	}
	return 0;
}
//add a new message to a queue
int CopyMessageToQ(BrokerQueue_t *q, psMessage_t *msg)
{
	BrokerQueueEntry_t *e = GetFreeEntry();

	if (e != NULL)
	{
		memcpy(&e->msg, msg, sizeof(psMessage_t));
		AppendQueueEntry(q, e);
		return 0;
	}
	else return -1;
}

//append an existing messageQ entry to a queue
void AppendQueueEntry(BrokerQueue_t *q, BrokerQueueEntry_t *e)
{

	int QOS = psQOS[e->msg.header.messageType];

	e->next = NULL;

	//critical section
	int s = pthread_mutex_lock(&q->mtx);
	if (s != 0)
	{
		ERRORPRINT("brokerQ: mutex lock %i", s);
	}

	int wake = 0;
	if (q->qHead[QOS] == NULL)
	{
		//Q empty
		q->qHead[QOS] = q->qTail[QOS] = e;
		wake = 1;
	}
	else
	{
		q->qTail[QOS]->next = e;
		q->qTail[QOS] = e;
	}
	q->queueCount++;

	s = pthread_mutex_unlock(&q->mtx);
	if (s != 0)
	{
		ERRORPRINT("brokerQ: mutex unlock %i", s);
	}
	//end critical section

	if (wake) pthread_cond_signal(&q->cond);

}
bool isQueueEmpty(BrokerQueue_t *q)
{
	return (q->queueCount == 0);
}

//get the first message or wait
//returns a reference to the message
psMessage_t *GetNextMessage(BrokerQueue_t *q)
{
	int i;
	BrokerQueueEntry_t *e;

	//critical section
	int s = pthread_mutex_lock(&q->mtx);
	if (s != 0)
	{
		ERRORPRINT("brokerQ: mutex lock %i", s);
	}
	//empty wait case
	while (q->queueCount == 0) pthread_cond_wait(&q->cond, &q->mtx);

	for (i=0; i < 3; i++)
	{
		if (q->qHead[i] == NULL) continue;

		e = q->qHead[i];
		q->qHead[i] = e->next;
		e->next = NULL;
		if (q->qHead[i] == NULL)
		{
			//end of queue
			q->qTail[i] = NULL;
		}
		break;
	}
	q->queueCount--;

	s = pthread_mutex_unlock(&q->mtx);
	if (s != 0)
	{
		ERRORPRINT("brokerQ: mutex unlock %i", s);
	}
	//end critical section

	return (psMessage_t*) e;
}

//release a message queue entry when done
void DoneWithMessage(psMessage_t *msg)
{
	AddToFreelist((BrokerQueueEntry_t *)msg);
}

//allocate a queue entry
BrokerQueueEntry_t *NewQueueEntry()
{
	BrokerQueueEntry_t *e = calloc(1, sizeof(BrokerQueueEntry_t));
	if (e == NULL)
	{
		LogError("brokerQ: no memory");
	}
	return e;
}

//free a used entry
void AddToFreelist(BrokerQueueEntry_t *e)
{
	//critical section
	int s = pthread_mutex_lock(&freeMtx);
	if (s != 0)
	{
		ERRORPRINT("brokerQ: freelist mutex lock %i", s);
	}
	e->next = freelist;
	freelist = e;

	s = pthread_mutex_unlock(&freeMtx);
	if (s != 0)
	{
		ERRORPRINT("brokerQ: freelist mutex unlock %i", s);
	}
	//end critical section
}

//get next free entry
BrokerQueueEntry_t *GetFreeEntry()
{
	BrokerQueueEntry_t *e;

	//critical section
	int s = pthread_mutex_lock(&freeMtx);
	if (s != 0)
	{
		ERRORPRINT("brokerQ: freelist mutex lock %i", s);
	}

	if (freelist)
	{
		e = freelist;
		freelist = e->next;
		e->next = NULL;
	}
	else
	{
		e = NewQueueEntry();
	}

	s = pthread_mutex_unlock(&freeMtx);
	if (s != 0)
	{
		ERRORPRINT("brokerQ: freelist mutex unlock %i", s);
	}
	//end critical section

	return e;
}
