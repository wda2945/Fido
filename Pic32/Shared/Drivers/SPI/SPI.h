/* 
 * File:   SPI.h
 * Author: martin
 *
 * Created on April 19, 2014, 4:58 PM
 */

#ifndef _SPI_H
#define	_SPI_H


#ifdef	__cplusplus
extern "C" {
#endif

void SPIPut(BYTE v);
BYTE SPIGet(void);


#ifdef	__cplusplus
}
#endif

#endif	/* _SPI_H */