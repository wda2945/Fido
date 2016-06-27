/*
 * scanner.h
 *
 *  Created on: Jul 11, 2014
 *      Author: martin
 */

#ifndef SCANNER_H_
#define SCANNER_H_

#include "behavior/behavior_enums.h"

//scanner task
int ScannerInit();

void ScannerProcessMessage(psMessage_t *msg);

ActionResult_enum proximityStatus(ProxSectorMask_enum _sectors,  ProxStatusMask_enum _status);

#endif
