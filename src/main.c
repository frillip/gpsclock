#include "18F26K80.h"
#device PASS_STRINGS = IN_RAM
#fuses NOPROTECT,HSH,PLLEN,WDT,WDT128 //,SOSC_LOW
#use delay(clock=64000000,crystal=16000000)
#use rs232(baud=115200,parity=N,xmit=PIN_C6,rcv=PIN_C7,bits=8,ERRORS,stream=COM1,restart_wdt)	// TTL serial for AT commands
#use rs232(baud=9600,parity=N,xmit=PIN_B6,rcv=PIN_B7,bits=8,ERRORS,stream=COM2,restart_wdt)	// TTL serial for GPS
#include "stdint.h"				// Standard int types

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
	setup_timer_2(T2_DIV_BY_1, 0x20, 1);		// Our SPI 7 segment displays only operate up to 250kHz, so we have to set up the SPI clock using timer2
	setup_timer_3(T3_INTERNAL | T3_DIV_BY_8);	// Set up scheduler timer
	enable_interrupts(INT_EXT_L2H);				// PPS interrupt
	enable_interrupts(INT_RDA);					// Enable AT command serial interrupt
	enable_interrupts(INT_RDA2);				// Enable GPS serial interrupt
	enable_interrupts(INT_TIMER3);				// Enable scheduler timer interrupt
	enable_interrupts(GLOBAL);					// Enable interrupts globally

	restore_time();

	clear_cmd_buffers();

	init_display();

	init_gps_uart();

	update_display();
	update_brightness();

	restart_wdt();

	fprintf(COM1, "HELLO!\r\n");	// Say hello!

	reset_scheduler();			// Reset and set scheduler
//	set_timer1(-32768);			// Begin timekeeping
	while(TRUE)
	{
		restart_wdt();
		if(t10ms0==1)
		{
			t10ms0=0;
			if(gpgga_waiting) process_gpgga();
			if(utc.second_100==0&&toggle_waiting)
			{
				toggle_colon();
				toggle_waiting=FALSE;
			}
			wallclock_inc_sec_100();
			if(inc_minute_flag)
			{
				wallclock_inc_minute();
				inc_minute_flag=FALSE;
			}
			if(display_mode==0)
			{
				update_display0();
				update_display1();
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
		}
		if(t100ms1==5)
		{
			t100ms1=0;
		}
		if(t1s0==1)
		{
			t1s0=0;
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
				reset_display();
				delay_us(10);
				toggle_colon();
			}
			update_display0();
			update_display1();
		}
	}
}
