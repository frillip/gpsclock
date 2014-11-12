#include "18F26K80.h"
#device PASS_STRINGS = IN_RAM
#fuses NOPROTECT,SOSC_LOW,HSH,PLLEN,WDT,WDT128
#use delay(clock=64000000,crystal=16000000,restart_wdt)
#use rs232(baud=115200,parity=N,xmit=PIN_C6,rcv=PIN_C7,bits=8,ERRORS,stream=COM1,restart_wdt)	// TTL serial for AT commands
#use rs232(baud=115200,parity=N,xmit=PIN_B6,rcv=PIN_B7,bits=8,ERRORS,stream=COM2,restart_wdt)	// TTL serial for GPS
#include "stdint.h"				// Standard int types

#define PPS				// We have a PPS input attached to PIN_B0 (INT_EXT0)
#define DISP0_SS PIN_B4	// SS pin for display 0
#define DISP1_SS PIN_B5	// SS pin for display 1

#include "wallclock.h"
#include "scheduler.h"
#include "display.h"
#include "remote.h"
#include "gps.h"

char timestr[9]=__TIME__;

void main(void)
{
	setup_adc(ADC_OFF);							// No analogue inputs
	setup_wdt(WDT_ON);							// WDT on
	setup_timer_2(T2_DIV_BY_1, 0x28, 1);		// Our SPI 7 segment displays only operate up to 250kHz, so we have to set up the SPI clock using timer2
	setup_timer_3(T3_INTERNAL | T3_DIV_BY_8);	// Set up scheduler timer
#IFDEF PPS
	enable_interrupts(INT_EXT_L2H);
#ENDIF
	enable_interrupts(INT_RDA);					// Enable AT command serial interrupt
	enable_interrupts(INT_RDA2);				// Enable GPS serial interrupt
	enable_interrupts(INT_TIMER3);				// Enable scheduler timer interrupt
	enable_interrupts(GLOBAL);					// Enable interrupts globally

	if(read_eeprom(EEPROM_RESET)==0x42)
	{
		time.hour=read_eeprom(EEPROM_HOUR);
		time.minute=read_eeprom(EEPROM_MINUTE);
		time.second=read_eeprom(EEPROM_SECOND);
		write_eeprom(EEPROM_RESET,0x00);
	}
	else
	{
		time.second=(((uint8_t)timestr[6]-48)*10)+((uint8_t)timestr[7]-46);
		time.minute=(((uint8_t)timestr[3]-48)*10)+((uint8_t)timestr[4]-48);
		time.hour=(((uint8_t)timestr[0]-48)*10)+((uint8_t)timestr[1]-48);		// Parse timestr to time struct
	}
	memset(command_buffer, 0, sizeof(command_buffer));
	memset(command, 0, sizeof(command));
	
	restart_wdt();
	init_display();

	fprintf(COM1, "HELLO!\r\n");	// Say hello!

	t10ms=0;
	t100ms=0;
	t100ms0=0;
	t1s0=0;
	set_timer3(-20000);			// Reset and set scheduler
	while(TRUE)
	{
		restart_wdt();
		if(t10ms0==1)
		{
			t10ms0=0;
			if(command_waiting) process_command();
			if(gpzda_waiting) process_gpzda();
			wallclock_inc_sec_100();
			if(display_mode==0) update_display1();
		}
		if(t100ms0==1)
		{
			t100ms0=0;
		}
		if(t100ms1==5)
		{
			t100ms1=0;
		}
		if(t1s0==1)
		{
			t1s0=0;
			toggle_colon();
			update_display0();
			update_display1();
		}
		if(t1s1==5)
		{
			t1s1=0;
			display_mode=!display_mode;
			update_display0();
			update_display1();
		}
	}
}
