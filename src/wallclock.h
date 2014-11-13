#define EEPROM_RESET		0x00
#define EEPROM_YEAR			0x01
#define EEPROM_MONTH		0x03
#define EEPROM_DAY			0x04
#define EEPROM_HOUR			0x05
#define EEPROM_MINUTE		0x06
#define EEPROM_SECOND		0x07
#define EEPROM_SECOND_100	0x08

void wallclock_inc_year()
{
	time.year++;
	if(display_mode==1) update_display0();
}

void wallclock_inc_month()
{
	time.month++;
	if(time.month>12)
	{
		time.month=1;
		wallclock_inc_year();
	}
}

void wallclock_inc_day(void)
{
	time.day++;
	if(time.day>31)
	{
		time.day=1;
		wallclock_inc_month();
		return;
	}
	if((time.day>30)&&((time.month==4)||(time.month==6)||(time.month==9)||(time.month==11)))
	{
		time.day=1;
		wallclock_inc_month();
		return;
	}
	if((time.day>29)&&(time.month==2)&&((time.year%4==0)||(time.year%10==0)))
	{
		time.day=1;
		wallclock_inc_month();
		return;
	}
	if((time.day>28)&&(time.month==2)&&((time.year%4!=0)||(time.year%10==0)))
	{
		time.day=1;
		wallclock_inc_month();
		return;
	}
	if(display_mode==1) update_display1();
}

void wallclock_inc_hour(void)
{
	time.hour++;
	if(time.hour>=24)
	{
		time.hour=0;
		wallclock_inc_day();
	}
}

void wallclock_inc_minute(void)
{
	time.minute++;
	if(time.minute>=60)
	{
		time.minute=0;
		wallclock_inc_hour();
	}
	if(display_mode==0) update_display0();
}

void wallclock_inc_second(void)
{
	time.second++;
	if(time.second>=60)
	{
		time.second=0;
		wallclock_inc_minute();
	}
	if(display_mode==0) update_display1();
}

void wallclock_inc_sec_100(void)
{
	time.second_100++;
	if(display_mode==0) update_display1();
}