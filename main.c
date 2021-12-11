/* 
 * File:   main.c
 * Author: 佐々木翔
 *
 * Created on 2021/11/07, 11:09
 * Discription 実装試験の共有系のプログラムV2
 * 編集履歴
 * 2021/11/25：ACKを返すプログラムの実装
 * 2021/12/04：送信要求後の待ち時間0usを追加
 * 
 */

#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include "spi.h"
#include "MCP2515.h"
#include "CSS.h"
#include "fram.h"

// CONFIG1
#pragma config FOSC  = HS       // Oscillator Selection bits (HS oscillator: High-speed crystal/resonator on RA6/OSC2/CLKOUT and RA7/OSC1/CLKIN)
#pragma config WDTE  = OFF      // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = ON       // Power-up Timer Enable bit (PWRT enabled)
#pragma config MCLRE = ON       // RE3/MCLR pin function select bit (RE3/MCLR pin function is MCLR)
#pragma config CP    = OFF      // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD   = OFF      // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = ON       // Brown Out Reset Selection bits (BOR enabled)
#pragma config IESO  = OFF      // Internal External Switchover bit (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is disabled)
#pragma config LVP   = OFF      // Low Voltage Programming Enable bit (RB3 pin has digital I/O, HV on MCLR must be used for programming)

// CONFIG2
#pragma config BOR4V = BOR21V   // Brown-out Reset Selection bit (Brown-out Reset set to 2.1V)
#pragma config WRT   = OFF      // Flash Program Memory Self Write Enable bits (Write protection off)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#define _XTAL_FREQ      20000000                                                //PICの周波数を20MHzに設定
#define _CAN_BAUDRATE   2                                                       //ボーレート

/*Prtotype*/
void CONFIG();                                                                  //初期設定
char BtoD(char data);                                                           //2進数を10進数に変換

char end_flag = 0;          
char *rx_data;                                                                  //受信データ格納変数
char *fram_data;                                                                //framのデータ格納変数
char rx_status;                                                                 //受信状態
char rx_int;                                                                    //受信割り込み
char mode = _ChargeMode;                                                        //初期モード
char size;                                                                      //サイズ変数
char TEC;
char REC;

void main(void)
{
    CONFIG();                                                                   //初期設定
    __delay_ms(100);
    
    Wren();                                                         //書き込み許可
    Fram_Write(0, 0, &mode, 1);          //メモリへの書き込み
    
    while(1)
    {
        /* 受信処理 */
        rx_int = Read(_CANINTF);                                                //受信フラグ確認
        TEC    = Read(_TEC);                                                    //送信エラーカウンタ読み込み
        REC    = Read(_REC);                                                    //受信エラーカウンタ読み込み
        
        if((rx_int & _Flagbit0) == 0b00000001)                                  //受信処理
        {   
            rx_data = Read_RX_ID(_F_RXB0SIDH, 13);                              //受信レジスタの読み込み
            size = BtoD(rx_data[4]);                                            //メモリの書き込み・読み出し変数として扱うために，データサイズを2→10進数に変換
            Write(_CANINTF, 0b00000000);                                        //割り込みフラグのクリア
            
            if((rx_data[1] & _Flagbit0) == 0b00000001)                          //Write（メモリへの書き込み）処理
            {
                Wren();                                                         //書き込み許可
                Fram_Write(rx_data[2], rx_data[3], &rx_data[5], size);          //メモリへの書き込み
                rx_data[6] = TEC;                                               //データフィールド[6]に送信エラーカウンタの値を代入
                rx_data[7] = REC;
                Write(_TXB0DLC , rx_data[4]);                                   //メッセージサイズ8byte
                Load_TX_ID(_F_TXB0SIDH, 0b00000000, 0b00001001, 0, rx_data[0]); //送信識別子（拡張フレーム）の設定
                Load_TX_Data(_F_TXB0D0, size, &rx_data[5]);                     //送信データ格納
                RTS0_CSS(_CAN_BAUDRATE);                                        //送信要求
            }
            
            if(!((rx_data[1] & _Flagbit0) == 0b00000001))                       //Read(メモリから読み出し)処理
            {
                fram_data = Fram_Read(rx_data[2], rx_data[3], size);            //framからモードフラグデータを読み込む
                Write(_TXB0DLC , rx_data[4]);                                   //メッセージサイズ8byte
                Load_TX_ID(_F_TXB0SIDH, 0b00000000, 0b00001000, 0, rx_data[0]); //送信識別子（拡張フレーム）の設定
                Load_TX_Data(_F_TXB0D0, size, &fram_data[0]);                       //送信データ格納
                RTS0_CSS(_CAN_BAUDRATE);                                        //送信要求
            }
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
    
    spi_init();
    __delay_ms(100);
    
    MCP2515_init(_CAN_BAUDRATE);                                                //とりあえず，動作している2にした．理解はまだ
    Write(_TXB0DLC , 0b00001000);                                               //メッセージサイズ8byte
    MCP2515_Open(0);                                                            //とりあえず，0にした．理解はまだ
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
