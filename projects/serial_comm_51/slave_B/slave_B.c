#include <reg52.h>
#include <stdio.h>

sbit K1   = P3^2;
sbit LED1 = P2^0;
sbit LED2 = P2^1;

#define XINGMING "齐继浩"
#define XUEHAO   "094424128"

void Uart_Init(void)
{
    SCON = 0x50;
    TMOD |= 0x20;
    TH1  = 0xFD;
    TL1  = 0xFD;
    TR1  = 1;
    EA   = 1;
    ES   = 1;
}

void Send_Byte(unsigned char dat)
{
    SBUF = dat;
    while(!TI);
    TI = 0;
}
char putchar(char c)
{
    Send_Byte(c);
    return c;
}

void Key1_Send(void)
{
    if(K1 == 0)
    {
        unsigned int t;
        for(t=0;t<5000;t++);
        if(K1 == 0)
        {
            printf("Hello, %s + %s\r\n",XINGMING,XUEHAO);
            while(K1==0);
        }
    }
}

unsigned char dat_buf[2],num=0;
void Uart_Int() interrupt 4
{
    if(RI)
    {
        RI=0;
        dat_buf[num++]=SBUF;
        if(num>=2)
        {
            num=0;
            if(dat_buf[0]=='1'&&dat_buf[1]=='1') LED1=0;
            if(dat_buf[0]=='1'&&dat_buf[1]=='0') LED1=1;
            if(dat_buf[0]=='2'&&dat_buf[1]=='1') LED2=0;
            if(dat_buf[0]=='2'&&dat_buf[1]=='0') LED2=1;
        }
    }
}

void main(void)
{
    Uart_Init();
    LED1=1;LED2=1;
    while(1)
    {
        Key1_Send();
    }
}
