#include "18F26K80.h"
#device PASS_STRINGS = IN_RAM
#fuses NOPROTECT,HSH,PLLEN,WDT,WDT128 //,SOSC_LOW
#use delay(clock=64000000,crystal=16000000)
#use rs232(baud=115200,parity=N,xmit=PIN_C6,rcv=PIN_C7,bits=8,ERRORS,stream=COM1,restart_wdt)	// TTL serial for AT commands
#use rs232(baud=9600,parity=N,xmit=PIN_B6,rcv=PIN_B7,bits=8,ERRORS,stream=COM2,restart_wdt)	// TTL serial for GPS
#include "stdint.h"				// Standard int types

#define PPS				// We have a PPS input attached to PIN_B0 (INT_EXT0)
//#define OUTPUT_PPS
#define OUTPUT_FEEDBACK
//#define OUTPUT_NMEA		// Output recieved GPZDA and GPGGA messages
//#define OUTPUT_ALL_GPS	// Output EVERYTHING recieved on the GPS UART
#define DISP0_SS PIN_B4	// SS pin for display 0
#define DISP1_SS PIN_B5	// SS pin for display 1

// Compile date and time
char timestr[9]=__TIME__;
char datestr[10]=__DATE__;

typedef struct {
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
	uint8_t second_100;
} time_struct;

typedef struct {
	uint8_t minus_flag;
	uint8_t hour;
	uint8_t minute;
} offset;

time_struct utc = {2015,6,30,0,0,0,0};
time_struct local = {2015,6,30,0,0,0,0};
offset timezone = {0,0};

uint8_t gps_fix=0;
uint8_t satellite_count=0;
boolean first_fix=TRUE;

#include "display.h"
#include "wallclock.h"
#include "scheduler.h"
#include "remote.h"
#include "gps.h"

void main(void)
{
	setup_adc(ADC_OFF);							// No analogue inputs
	setup_wdt(WDT_ON);							// WDT on
	setup_timer_2(T2_DIV_BY_1, 0x1F, 1);		// Our SPI 7 segment displays only operate up to 250kHz, so we have to set up the SPI clock using timer2
	setup_timer_1(T1_INTERNAL | T1_DIV_BY_8);
	setup_timer_3(T3_INTERNAL | T3_DIV_BY_8);	// Set up scheduler timer
	#IFDEF PPS
	enable_interrupts(INT_EXT_L2H);				// PPS interrupt
	#ENDIF
//	enable_interrupts(INT_EXT1);				// GPS fix interrupt
	enable_interrupts(INT_RDA);					// Enable AT command serial interrupt
	enable_interrupts(INT_RDA2);				// Enable GPS serial interrupt
//	enable_interrupts(INT_TIMER1);				// Enable timekeeping timer interrupt
	enable_interrupts(INT_TIMER3);				// Enable scheduler timer interrupt
	enable_interrupts(GLOBAL);					// Enable interrupts globally

	if(read_eeprom(EEPROM_RESET)==0x42)
	{
		utc.year=((uint16_t)read_eeprom(EEPROM_YEAR)<<8)|((uint16_t)read_eeprom(EEPROM_YEAR+1));
		utc.month=read_eeprom(EEPROM_MONTH);
		utc.day=read_eeprom(EEPROM_DAY);
		utc.hour=read_eeprom(EEPROM_HOUR);
		utc.minute=read_eeprom(EEPROM_MINUTE);
		utc.second=read_eeprom(EEPROM_SECOND);
		write_eeprom(EEPROM_RESET,0x00);
	}
	else
	{
		utc.second=(((uint8_t)timestr[6]-48)*10)+((uint8_t)timestr[7]-46);
		utc.minute=(((uint8_t)timestr[3]-48)*10)+((uint8_t)timestr[4]-48);
		utc.hour=(((uint8_t)timestr[0]-48)*10)+((uint8_t)timestr[1]-48);		// Parse timestr to time struct

		utc.day=(((uint8_t)datestr[0]-48)*10)+((uint8_t)datestr[1]-48);

		switch (datestr[3])
		{
			case 'J' :
				if(datestr[4]=='A') utc.month=1;
				else if(datestr[5]=='N')utc.month=6;
				else utc.month=7;
				break;
			case 'F' :
				utc.month=2;
				break;
			case 'M' :
				if(datestr[5]=='R') utc.month=3;
				else utc.month=5;
				break;
			case 'A' :
				if(datestr[4]=='P') utc.month=4;
				else utc.month=8;
				break;
			case 'S' :
				utc.month=9;
				break;
			case 'O' :
				utc.month=10;
				break;
			case 'N' :
				utc.month=11;
				break;
			case 'D' :
				utc.month=12;
				break;
			default  :
				utc.month=10;
				break;
		}
		utc.year=2000+(((uint8_t)datestr[7]-48)*10)+((uint8_t)datestr[8]-48);
	}

	timezone.minus_flag=read_eeprom(EEPROM_TZ_MINUS_FLAG);
	if(timezone.minus_flag>1)
	{
		timezone.minus_flag=0;
		write_eeprom(EEPROM_TZ_MINUS_FLAG,timezone.minus_flag);
	}
	timezone.hour=read_eeprom(EEPROM_TZ_HOUR);
	if(timezone.hour>14)
	{
		timezone.hour=0;
		write_eeprom(EEPROM_TZ_HOUR,timezone.hour);
	}
	timezone.minute=read_eeprom(EEPROM_TZ_MINUTE);
	if(timezone.minute>45)
	{
		timezone.minute=0;
		write_eeprom(EEPROM_TZ_MINUTE,timezone.minute);
	}

	memset(command_buffer, 0, sizeof(command_buffer));
	memset(command, 0, sizeof(command));
	
	set_uart_speed(9600,COM2);
	
	delay_ms(200);
	
	fprintf(COM2,"$PMTK251,115200*1F\r\n");
	
	delay_ms(200);
	
	set_uart_speed(115200,COM2);

	fprintf(COM2,"$PMTK314,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0*28\r\n");

	restart_wdt();
	init_display();

	fprintf(COM1, "HELLO!\r\n");	// Say hello!



	reset_scheduler();			// Reset and set scheduler
//	set_timer1(-32768);			// Begin timekeeping
	while(TRUE)
	{
		restart_wdt();
		if(t10ms0==1)
		{
			t10ms0=0;
			if(utc.second_100==0&&toggle_waiting)
			{
				toggle_colon();
				toggle_waiting=FALSE;
			}
			wallclock_inc_sec_100();
			if(display_mode==0)
			{
				update_display0();
				update_display1();
			}
			if(inc_minute_flag)
			{
				wallclock_inc_minute();
				inc_minute_flag=FALSE;
			}
			#IFDEF OUTPUT_FEEDBACK
			if(pps_waiting) pps_feedback();
			#ENDIF
		}
		if(t100ms0==1)
		{
			t100ms0=0;
			if(command_waiting) process_command();
			if(gpzda_waiting) process_gpzda();
			if(gpgga_waiting) process_gpgga();
		}
		if(t100ms1==5)
		{
			t100ms1=0;
		}
		if(t1s0==1)
		{
			t1s0=0;
			#IFDEF PPS
			//if((gps_fix==0)||(satellite_count<4)) wallclock_inc_second();
			#ELSE
			wallclock_inc_second();
			#ENDIF
			update_display0();
			update_display1();
			#IFDEF OUTPUT_FEEDBACK
			if(pps_done==FALSE) pps_feedback();
			pps_done=FALSE;
			#ENDIF
			if(display_mode==0)
			{
				if((utc.second==10)||(utc.second==30)||(utc.second==50)) mode_switch=TRUE;
			}
			else if (display_mode==1)
			{
				if((utc.second==15)||(utc.second==35)||(utc.second==55)) mode_switch=TRUE;
			}
			if(mode_switch)
			{
				display_mode=!display_mode;
				mode_switch=false;
				output_low(DISP0_SS);
				output_low(DISP1_SS);
				#asm nop #endasm
				spi_write(0x76);
				#asm nop #endasm
				output_high(DISP0_SS);
				output_high(DISP1_SS);
				delay_us(10);
				toggle_colon();
				update_display0();
				update_display1();
			}
		}
	}
}
