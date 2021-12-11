/* 
 * File:   main.c
 * Author: 佐々木翔
 *
 * Created on 2021/11/07, 17:13
 * Discription　実装試験3のミッション系のプログラム
 * 編集履歴
 * 2021/11/07：初版
 * 
 */

#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include "spi.h"
#include "MCP2515.h"
#include "MISN.h"

// CONFIG1
#pragma config FOSC  = HS        // Oscillator Selection bits (HS oscillator: High-speed crystal/resonator on RA6/OSC2/CLKOUT and RA7/OSC1/CLKIN)
#pragma config WDTE  = OFF       // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = ON       // Power-up Timer Enable bit (PWRT enabled)
#pragma config MCLRE = ON       // RE3/MCLR pin function select bit (RE3/MCLR pin function is MCLR)
#pragma config CP    = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD   = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = ON       // Brown Out Reset Selection bits (BOR enabled)
#pragma config IESO  = OFF       // Internal External Switchover bit (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is disabled)
#pragma config LVP   = OFF        // Low Voltage Programming Enable bit (RB3 pin has digital I/O, HV on MCLR must be used for programming)

// CONFIG2
#pragma config BOR4V = BOR21V   // Brown-out Reset Selection bit (Brown-out Reset set to 2.1V)
#pragma config WRT   = OFF        // Flash Program Memory Self Write Enable bits (Write protection off)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#define _XTAL_FREQ      20000000
#define _CAN_BAUDRATE   2

/*--Prtotype--*/
void CONFIG();                                                                  //初期設定
void timer_init();                                                             //タイマーの初期設定
void interrupt timer();                                                         //タイマーの割り込み処理
char BtoD(char data);                                                           //2進数を10進数に変換

/*--Grobal Variables--*/
volatile char *rx_data;
volatile char *fram_data;
volatile char rx_status;
volatile char rx_int;
volatile char data[8] = {1, 0, 0, 0, 0, 0, 0, 1};
volatile char mode = _ChargeMode;
volatile char size;
volatile int cnt = 0;
volatile int cnt1 = 0;
volatile int mode_cnt = 0;                                                      //モードカウンタ

void main(void)
{   
    int rate1 = 10;
    int rate2 = 10;
    
    CONFIG();                                                                   //初期設定                      
    __delay_ms(100);
    
    T1CONbits.TMR1ON = 1;                                                       //タイマー1を起動
    
    /*
    RB5 = 0;
    RB6 = 0;
    RB7 = 1;
    */
    
    while(1)
    {
        /* モード遷移処理 */
        rx_int = Read(_CANINTF);                                                //受信フラグ確認
        
        if((rx_int & _Flagbit0) == 0b00000001)
        {   
            rx_data = Read_RX_ID(_F_RXB0SIDH, 13);
            Write(_CANINTF, 0b00000000);
           
           if(rx_data[3] == SIDH_MODE)                                          //モード情報かを確認
           {
                if((rx_data[5] & _ChargeMode) == 0b00000001)                    //充電モードに移行
                {
                    /*
                    RB5 = 0;
                    RB6 = 0;
                    RB7 = 1;
                    */
                    mode = _ChargeMode;
                }
            
                if((rx_data[5] & _COMMMode) == 0b00000010)                      //ダウンリンクモードに移行
                {
                    /*
                    RB5 = 0;
                    RB6 = 1;
                    RB7 = 0;
                    */
                    mode = _COMMMode;
                }
            
                if((rx_data[5] & _StanbyMode) == 0b00000011)                    //待機モードに移行
                {
                    /*
                    RB5 = 1;
                    RB6 = 0; 
                    RB7 = 0;
                    */
                    mode = _StanbyMode;
                }
            
                if((rx_data[5] & _MissionMode) == 0b00000100)                   //ミッションモードに移行
                {
                    /*
                    RB5 = 0;
                    RB6 = 0; 
                    RB7 = 0;
                    */
                    mode = _MissionMode;
                }
            }
        }

        if(mode_cnt <= 2000)
        {
            /* 充電モード */
            if(mode == _ChargeMode)
            {
                /* 1Hz */
                if(cnt >= rate1)                                                       
                {
                    Write(_TXB0DLC , 0b00000001);                                       //メッセージサイズ1byte
                    Load_TX_ID(_F_TXB0SIDH, SIDH_MODE, SIDL_R, 0, 0);                   //モードフラグ読み出し
                    Load_TX_Data(_F_TXB0D0, 1, 0);                                      //読み出しだから，空データ
                    RTS0(_CAN_BAUDRATE);                                                //送信要求

                    Write(_TXB0DLC , 0b00000001);                                       //メッセージサイズ1byte
                    Load_TX_ID(_F_TXB0SIDH, SIDH_MODE, SIDL_W, EID8_MODE, EID0_MODE);   //モード書き込みIDの設定
                    Load_TX_Data(_F_TXB0D0, 1, &mode);                                  //モード書き込み
                    RTS0(_CAN_BAUDRATE);                                                //送信要求

                    cnt = 0;
                }
            }
            
            /* 待機モード */
            if(mode == _StanbyMode)
            {
                /* 1Hz */
                if(cnt >= rate1)
                {
                    Write(_TXB0DLC , 0b00000001);                                       //メッセージサイズ1byte
                    Load_TX_ID(_F_TXB0SIDH, SIDH_MODE, SIDL_R, 0, 0);                   //モード要求
                    Load_TX_Data(_F_TXB0D0, 1, 0);                                      //読み出しだから，空データ
                    RTS0(_CAN_BAUDRATE);                                                //送信要求
                    
                    Write(_TXB0DLC , 0b00000001);                                       //メッセージサイズ1byte
                    Load_TX_ID(_F_TXB0SIDH, SIDH_MODE, SIDL_W, EID8_MODE, EID0_MODE);   //モード書き込みIDの設定
                    Load_TX_Data(_F_TXB0D0, 1, &mode);                                  //モード報告
                    RTS0(_CAN_BAUDRATE);                                                //送信要求

                    cnt = 0;
                }
            }

            /* ミッションモード */
            if(mode == _MissionMode)
            {   
                /*1Hz */
                if(cnt >= rate1)
                {
                    Write(_TXB0DLC , 0b00000001);                                       //メッセージサイズ1byte
                    Load_TX_ID(_F_TXB0SIDH, SIDH_MODE, SIDL_R, 0, 0);                   //モード要求
                    Load_TX_Data(_F_TXB0D0, 1, 0);                                      //読み出しだから，空データ
                    RTS0(_CAN_BAUDRATE);                                                //送信要求
                    
                    Write(_TXB0DLC , 0b00000001);                                       //メッセージサイズ1byte
                    Load_TX_ID(_F_TXB0SIDH, SIDH_MODE, SIDL_W, EID8_MODE, EID0_MODE);   //モード書き込みIDの設定
                    Load_TX_Data(_F_TXB0D0, 1, &mode);                                  //モード報告
                    RTS0(_CAN_BAUDRATE);                                                //送信要求

                    cnt = 0;
                }

                /* 100Hz */
                if(cnt1 >= rate2)
                {
                    Write(_TXB0DLC , 0b00001000);                                       //メッセージサイズ8byte
                    Load_TX_ID(_F_TXB0SIDH, SIDH_DATA1, SIDL_W, EID8_DATA1, EID0_DATA1);//ミッションデータ書き込み
                    Load_TX_Data(_F_TXB0D0, 8, data);                                   //ミッションデータ
                    RTS0(_CAN_BAUDRATE);                                                //送信要求
                    
                    data[1] = data[1] + 1;

                    cnt1 = 0;
                }

            }        
        }
        
        //20秒経ったら通信モードに
        if(mode_cnt > 2000)
        {
                if(cnt >= rate1)
                {
                    Write(_TXB0DLC , 0b00000001);                                       //メッセージサイズ1byte
                    Load_TX_ID(_F_TXB0SIDH, SIDH_MODE, SIDL_R, 0, 0);                   //モード要求
                    Load_TX_Data(_F_TXB0D0, 1, 0);                                      //読み出しだから，空データ
                    RTS0(_CAN_BAUDRATE);                                                //送信要求

                    Write(_TXB0DLC , 0b00000001);                                       //メッセージサイズ1byte
                    Load_TX_ID(_F_TXB0SIDH, SIDH_MODE, SIDL_W, EID8_MODE, EID0_MODE);   //モード書き込みIDの設定
                    Load_TX_Data(_F_TXB0D0, 1, &mode);                                  //モード報告
                    RTS0(_CAN_BAUDRATE);                                                //送信要求

                    cnt = 0;
                }
            
            if(mode_cnt > 3000) mode_cnt = 0;
        }
    }
}

void CONFIG()
{
    OSCCON = 0b01101000;
    ANSEL  = 0b00000000;
    ANSELH = 0b00000000;
    TRISB  = 0b00000000;
    TRISC  = 0b00000000;
    PORTB  = 0b00000000;
    PORTC  = 0b00000000;
    
    INTCON = 0b11000000;                                                        //割り込み設定
    
    spi_init();
    timer_init();
    __delay_ms(100);
    
    MCP2515_init(_CAN_BAUDRATE);                                                //とりあえず，動作している2にした．理解はまだ
    Write(_TXB0DLC , 0b00000001);                                               //メッセージサイズ8byte
    Write(_RXM0SIDH, 0b11111111);                                               //マスク設定上位全一致
    Write(_RXM0SIDL, 0b11111111);                                               //マスク設定下位全一致
    Write(_RXM0EID8, 0b11111111);                                               //マスク設定拡張上位全一致
    Write(_RXM0EID0, 0b11110000);                                               //マスク設定拡張下位上位4bit一致
    Write(_RXF0SIDH, 0b00000000);                                               //受信フィルタ上位ビットの設定
    Write(_RXF0SIDL, 0b00001000);                                               //受信フィルタ下位ビットの設定
    Write(_RXF0EID8, 0b00000000);                                               //受信フィルタ拡張上位ビットの設定
    Write(_RXF0EID0, Sub_Filt);                                                 //受信フィルタ拡張下位ビットの設定
    MCP2515_Open(0);                                                            //とりあえず，0にした．理解はまだ
}

void timer_init()
{
    T1CON = 0b00000000;                                                         //プリスケーラ1:1
    TMR1H = 0b00111100;                                                         //500000でオーバーフローさせる
    TMR1L = 0b10110000;
    PIE1bits.TMR1IE = 1;
    PIR1bits.TMR1IF = 0;
}

void interrupt timer()
{
    PIR1bits.TMR1IF = 0;
    TMR1H = 0b00111100;                                                         //500000でオーバーフローさせる
    TMR1L = 0b10110000;
    
    cnt++;                                                                      //カウンタ0のインクリメント
    cnt1++;                                                                     //カウンタ1のインクリメント
    mode_cnt++;                                                                 //モードのカウンタ
}

char BtoD(char data)
{
    char  binary;
    char decimal = 0;
    char bas = 1;
            
    binary = data & 0b00001111;
    
    while(binary>0)
    {
        decimal = decimal + (binary % 10) * bas;
        binary = binary / 10;
        bas = bas * 2;
    }
    
    return decimal;
}

