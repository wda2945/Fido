/* 
 * File:   options.h
 * Author: martin
 *
 * MCP
 *
 * Created on April 15, 2014, 11:41 AM
 */


//optionmacro(name, var, min, max, def)
//processor power
optionmacro("Power Motors", motPower, 0, 1, 1)          //name used in PowerControl & Main_Task
optionmacro("Power Overmind", ovmPower, 0, 1, 1)        //name used in PowerControl & Main_Task

//proximity sensor active
optionmacro("Power Sonar Prox", sonarProxEnable, 0, 1, 0)//name used in PowerControl
optionmacro("Power IR Prox", irProxEnable, 0, 1, 0)      //name used in PowerControl
optionmacro("Power PIR Prox", pirProxEnable, 0, 1, 0)

//sensor reporting
optionmacro("Raw Prox", rawProxReport, 0, 1, 0)
optionmacro("Raw Analog", rawAnalogReport, 0, 1, 0)

//logging to App
optionmacro("Comm Stats", commStats, 0, 1, 1)
optionmacro("Comm BLE", useBLE, 0, 1, 1)

//BBB control
optionmacro("OVM PwrBtn Start", bbbPwrBtnStart, 0, 1, 1)  //use PWR_BTN to start
optionmacro("OVM PwrBtn Press", bbbPwrBtn, 0, 1, 0)     //press the PWR_BTN
optionmacro("OVM PwrBtn Stop", bbbPwrBtnStop, 0, 1, 1)  //use PWR_BTN to stop
optionmacro("OVM Start T/O", bbbStartTO, 0, 1, 0)   //use startup Timeout

//other
optionmacro("Lights Nav", navLights, 0, 1, 0)
optionmacro("Lights Front", frontLights, 0, 1, 0)
optionmacro("Lights Rear", rearLights, 0, 1, 0)
optionmacro("Lights Strobe", strobeLights, 0, 1, 1)
        
//-----Individual Sonar enables
//sonar sensors
optionmacro("Enable Sonar L", sonarL, 0, 1, 1)
optionmacro("Enable Sonar R", sonarR, 0, 1, 1)

//proximity IR sensors        
optionmacro("Enable Prox FL", proxFL, 0, 1, 1)
optionmacro("Enable Prox FC", proxFC, 0, 1, 1)
optionmacro("Enable Prox FR", proxFR, 0, 1, 1)
optionmacro("Enable Prox Rear", proxRear, 0, 1, 1)
optionmacro("Enable Prox L", proxL, 0, 1, 1)
optionmacro("Enable Prox R", proxR, 0, 1, 1)

//bumpers
optionmacro("Enable Bump FL", bumpFL, 0, 1, 1)
optionmacro("Enable Bump FR", bumpFR, 0, 1, 1)
optionmacro("Enable Bump RL", bumpRL, 0, 1, 1)
optionmacro("Enable Bump RR", bumpRR, 0, 1, 1)

//PIR
optionmacro("Enable Pir L", pirL, 0, 1, 1)
optionmacro("Enable Pir R", pirR, 0, 1, 1)
