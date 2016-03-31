/* 
 * File:   PMIC_msg.h
 * Author: martin
 *
 * Created on Feb 2, 2015
 */

#ifndef PMIC_MSG_H
#define	PMIC_MSG_H

//PMIC message codes

#define PMIC_I2C_ADDRESS     0x26

typedef enum {
    PMIC_REPORT_STATE, PMIC_SET_ACTIVE, PMIC_SET_STANDBY, PMIC_SET_OFF}
PMICcommand_enum;

typedef enum {
  PMIC_UNKNOWN, PMIC_POWER_OFF, PMIC_STANDBY, PMIC_ACTIVE, PMIC_POWERING_OFF}
PMICstate_enum;

#endif