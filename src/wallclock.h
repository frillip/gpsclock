#define EEPROM_RESET			0x00
#define EEPROM_YEAR				0x01
#define EEPROM_MONTH			0x03
#define EEPROM_DAY				0x04
#define EEPROM_HOUR				0x05
#define EEPROM_MINUTE			0x06
#define EEPROM_SECOND			0x07
#define EEPROM_SECOND_100		0x08
#define EEPROM_TZ_MINUS_FLAG	0x09
#define EEPROM_TZ_HOUR			0x10
#define EEPROM_TZ_MINUTE		0x11

boolean negative_flag=FALSE;
boolean overdue_flag=FALSE;
boolean inc_minute_flag=FALSE;
static uint8_t month_days[13]={0,31,28,31,30,31,30,31,31,30,31,30,31};

void calc_local_time(void)
{
	local.minute=utc.minute;
	local.hour=utc.hour;
	local.day=utc.day;
	local.month=utc.month;
	local.year=utc.year;
	if(timezone.hour||timezone.minute)
	{
		if(timezone.minus_flag)
		{
			local.minute=local.minute-timezone.minute;
			if(local.minute>60)
			{
				local.minute=local.minute+60;
				local.hour--;
			}
			local.hour=local.hour-timezone.hour;
			if(local.hour>24)
			{
				local.hour=local.hour+24;
				local.day--;
			}
			if(local.day>31)
			{
				local.month--;
				if(local.month>12)
				{
					local.month=local.month+12;
					local.year--;
				}
				if((local.month==2)&&((local.year%4!=0)||(local.year%10==0))) local.day=local.day+28;
				else if ((local.month==2)&&((local.year%4==0)&&(local.year%10!=0))) local.day=local.day+29;
				else if ((local.month==4)||(local.month==6)||(local.month==9)||(local.month==11)) local.day=local.day+30;
				else local.day=local.day+31;
			}
		}
		else
		{
			local.minute=local.minute+timezone.minute;
			if(local.minute>60)
			{
				local.minute=local.minute-60;
				local.hour++;
			}
			local.hour=local.hour+timezone.hour;
			if(local.hour>24)
			{
				local.hour=local.hour-24;
				local.day++;
			}
			if((local.day>28)&&(local.month==2)&&((local.year%4!=0)||(local.year%10==0)))
			{
				local.day=1;
				local.month++;
			}
			else if((local.day>29)&&(local.month==2)&&((local.year%4==0)&&(local.year%10!=0)))
			{
				local.day=1;
				local.month++;
			}
			else if((local.day>30)&&((local.month==4)||(local.month==6)||(local.month==9)||(local.month==11)))
			{
				local.day=1;
				local.month++;
			}
			else if(local.day>31)
			{
				local.day=1;
				local.month++;
			}
			if(local.month>12)
			{
				local.year++;
			}	
		}
	}
}

void wallclock_inc_year()
{
	utc.year++;
}

void wallclock_inc_month()
{
	utc.month++;
	if(utc.month>12)
	{
		utc.month=1;
		wallclock_inc_year();
	}
}

void wallclock_inc_day(void)
{
	utc.day++;
	if(utc.day>31)
	{
		utc.day=1;
		wallclock_inc_month();
		return;
	}
	if((utc.day>30)&&((utc.month==4)||(utc.month==6)||(utc.month==9)||(utc.month==11)))
	{
		utc.day=1;
		wallclock_inc_month();
		return;
	}
	if((utc.day>29)&&(utc.month==2)&&((utc.year%4==0)&&(utc.year%10!=0)))
	{
		utc.day=1;
		wallclock_inc_month();
		return;
	}
	if((utc.day>28)&&(utc.month==2)&&((utc.year%4!=0)||(utc.year%10==0)))
	{
		utc.day=1;
		wallclock_inc_month();
		return;
	}
}

void wallclock_inc_hour(void)
{
	utc.hour++;
	if(utc.hour>=24)
	{
		utc.hour=0;
		wallclock_inc_day();
	}
}

void wallclock_inc_minute(void)
{
	utc.minute++;
	if(utc.minute>=60)
	{
		utc.minute=0;
		wallclock_inc_hour();
	}
	calc_local_time();
}

void wallclock_inc_second(void)
{
	utc.second++;
	utc.second_100=0;
	if(utc.second>=60)
	{
		utc.second=0;
		inc_minute_flag=TRUE;
	}
}

void wallclock_inc_sec_100(void)
{
	utc.second_100++;
	if(utc.second_100>=100)
	{
		utc.second_100=0;
	}
}


void calc_diff(time_struct target, time_struct current, time_struct result) {
     // Copy struct
     if(negative_flag){
          result=current;
          current=target;
     }
     else {
          result=target;
     }

     //Calc second
     if(result.second<current.second) {
          result.minute--;
          result.second+=60;
     }
     result.second-=current.second;
     
     //Calc minute
     if(result.minute<current.minute) {
          result.hour--;
          result.minute+=60;
     }
     result.minute-=current.minute;
     
     //Calc hour
     if(result.hour<current.hour) {
          result.day--;
          result.hour+=24;
     }
     result.hour-=current.hour;

     //Calc day
     if(result.day<current.day) {
          result.month--;
          result.day+=month_days[current.month];
          if((current.month==2)&&(current.year%10!=0)&&(current.year%4==0)) result.day++; // Leap years SUCK!!!
     }
     result.day-=current.day;

     //Calc month
     if(result.month<current.month) {
          result.year--;
          result.month+=12;
     }
     result.month-=current.month;

     //Calc year
     if(result.year<current.year) {
          //result.minute--;
          //result.second+=60;
          negative_flag=TRUE;
          overdue_flag=TRUE;
     }
     else {
          negative_flag=FALSE;
     }
     result.year-=current.year;
     
     if(negative_flag) calc_diff(target, current, result);
     else {
          if(overdue_flag) printf("OVER: ");
          else printf("DUE:  ");
          if(result.year){
              printf("%lu year",result.year);
              if(result.year>1)printf("s");
              printf(", ");
          }
          if(result.month){
              printf("%u month",result.month);
              if(result.month>1)printf("s");
              printf(", ");
          }
          if(result.day){
              printf("%u day",result.day);
              if(result.day>1)printf("s");
              printf(", ");
          }
          if(result.hour){
              printf("%u hour",result.hour);
              if(result.hour>1)printf("s");
              printf(", ");
          }
          if(result.minute){
              printf("%u minute",result.minute);
              if(result.minute>1)printf("s");
              printf(", ");
          }
          if(result.second==1){
              printf("%u second.",result.second);
          }
          else printf("%u seconds.",result.second);
     }
}

void restore_time(void)
{
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
}