/* 
 * File:   main.c
 * Author: ���X����
 *
 * Created on 2021/11/07, 11:09
 * Discription ���������̋��L�n�̃v���O����V2
 * �ҏW����
 * 2021/11/25�FACK��Ԃ��v���O�����̎���
 * 2021/12/04�F���M�v����̑҂�����0us��ǉ�
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

#define _XTAL_FREQ      20000000                                                //PIC�̎��g����20MHz�ɐݒ�
#define _CAN_BAUDRATE   2                                                       //�{�[���[�g

/*Prtotype*/
void CONFIG();                                                                  //�����ݒ�
char BtoD(char data);                                                           //2�i����10�i���ɕϊ�

char end_flag = 0;          
char *rx_data;                                                                  //��M�f�[�^�i�[�ϐ�
char *fram_data;                                                                //fram�̃f�[�^�i�[�ϐ�
char rx_status;                                                                 //��M���
char rx_int;                                                                    //��M���荞��
char mode = _ChargeMode;                                                        //�������[�h
char size;                                                                      //�T�C�Y�ϐ�
char TEC;
char REC;

void main(void)
{
    CONFIG();                                                                   //�����ݒ�
    __delay_ms(100);
    
    Wren();                                                         //�������݋���
    Fram_Write(0, 0, &mode, 1);          //�������ւ̏�������
    
    while(1)
    {
        /* ��M���� */
        rx_int = Read(_CANINTF);                                                //��M�t���O�m�F
        TEC    = Read(_TEC);                                                    //���M�G���[�J�E���^�ǂݍ���
        REC    = Read(_REC);                                                    //��M�G���[�J�E���^�ǂݍ���
        
        if((rx_int & _Flagbit0) == 0b00000001)                                  //��M����
        {   
            rx_data = Read_RX_ID(_F_RXB0SIDH, 13);                              //��M���W�X�^�̓ǂݍ���
            size = BtoD(rx_data[4]);                                            //�������̏������݁E�ǂݏo���ϐ��Ƃ��Ĉ������߂ɁC�f�[�^�T�C�Y��2��10�i���ɕϊ�
            Write(_CANINTF, 0b00000000);                                        //���荞�݃t���O�̃N���A
            
            if((rx_data[1] & _Flagbit0) == 0b00000001)                          //Write�i�������ւ̏������݁j����
            {
                Wren();                                                         //�������݋���
                Fram_Write(rx_data[2], rx_data[3], &rx_data[5], size);          //�������ւ̏�������
                rx_data[6] = TEC;                                               //�f�[�^�t�B�[���h[6]�ɑ��M�G���[�J�E���^�̒l����
                rx_data[7] = REC;
                Write(_TXB0DLC , rx_data[4]);                                   //���b�Z�[�W�T�C�Y8byte
                Load_TX_ID(_F_TXB0SIDH, 0b00000000, 0b00001001, 0, rx_data[0]); //���M���ʎq�i�g���t���[���j�̐ݒ�
                Load_TX_Data(_F_TXB0D0, size, &rx_data[5]);                     //���M�f�[�^�i�[
                RTS0_CSS(_CAN_BAUDRATE);                                        //���M�v��
            }
            
            if(!((rx_data[1] & _Flagbit0) == 0b00000001))                       //Read(����������ǂݏo��)����
            {
                fram_data = Fram_Read(rx_data[2], rx_data[3], size);            //fram���烂�[�h�t���O�f�[�^��ǂݍ���
                Write(_TXB0DLC , rx_data[4]);                                   //���b�Z�[�W�T�C�Y8byte
                Load_TX_ID(_F_TXB0SIDH, 0b00000000, 0b00001000, 0, rx_data[0]); //���M���ʎq�i�g���t���[���j�̐ݒ�
                Load_TX_Data(_F_TXB0D0, size, &fram_data[0]);                       //���M�f�[�^�i�[
                RTS0_CSS(_CAN_BAUDRATE);                                        //���M�v��
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
    
    MCP2515_init(_CAN_BAUDRATE);                                                //�Ƃ肠�����C���삵�Ă���2�ɂ����D�����͂܂�
    Write(_TXB0DLC , 0b00001000);                                               //���b�Z�[�W�T�C�Y8byte
    MCP2515_Open(0);                                                            //�Ƃ肠�����C0�ɂ����D�����͂܂�
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
