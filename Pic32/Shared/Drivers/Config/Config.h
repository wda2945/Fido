/* 
 * File:   Config.h
 * Author: martin
 *
 * Created on April 18, 2014, 8:58 PM
 */

#ifndef CONFIG_H
#define	CONFIG_H

#ifdef	__cplusplus
extern "C" {
#endif

    void SendBinaryOptionConfig(char *name, int *var, int requestor);
    void SendOptionConfig(char *name, int *var, int minV, int maxV, int requestor);
    void SendSettingConfig(char *name, float *var, float minV, float maxV, int requestor);
    
    void SetOption(psMessage_t *msg, char *name, int *var, int minV, int maxV);
    void NewSetting(psMessage_t *msg, char *name, float *var, float minV, float maxV);

    void SetOptionByName(char *name, int value);
    
    void SendIntValue(char *name, int value);
    void SendFloatValue(char *name, int value);

    extern int configCount;
    
    
//Options
#define optionmacro(name, var, minv, maxv, def) extern int var;
#include "Options.h"
#undef optionmacro

//settings
#define settingmacro(name, var, minv, maxv, def) extern float var;
#include "Settings.h"
#undef settingmacro

#ifdef	__cplusplus
}
#endif

#endif	/* CONFIG_H */

