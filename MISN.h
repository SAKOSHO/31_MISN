/* 
 * File:   MISN.h
 * Author: 佐々木翔
 *
 * Created on 2021/06/30, 17:40
 */

#ifndef MISN_H
#define	MISN_H

/*---ADS CAN Message---*/
/* SIDH */
#define SIDH_READ   0b01000000                                                  //Read用
#define SIDH_MODE   0b01000001                                                  //モード情報
#define SIDH_DATA1  0b01000010                                                  //ミッションデータ

/* SIDL */
#define SIDL_W      0b00001001                                                  //Write用
#define SIDL_R      0b00001000                                                  //Read用

/* EID8 */
#define EID8_MODE   0b00000000
#define EID8_DATA1  0b00000000

/* EID0 */
#define EID0_MODE   0b00000001
#define EID0_DATA1  0b00000111

/* Filter */
#define Sub_Filt    0b01000000


/*---Modebit---*/
#define _ChargeMode                          0b00000001
#define _COMMMode                            0b00000010
#define _StanbyMode                          0b00000011
#define _MissionMode                         0b00000100
#define _SafetyMode                          0b00000101


#endif



