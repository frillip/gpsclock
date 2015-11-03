#define EEPROM_RESET		0x00
#define EEPROM_YEAR			0x01
#define EEPROM_MONTH		0x03
#define EEPROM_DAY			0x04
#define EEPROM_HOUR			0x05
#define EEPROM_MINUTE		0x06
#define EEPROM_SECOND		0x07
#define EEPROM_SECOND_100	0x08

boolean negative_flag=FALSE;
boolean overdue_flag=FALSE;
static uint8_t month_days[13]={0,31,28,31,30,31,30,31,31,30,31,30,31};

void wallclock_inc_year()
{
	time.year++;
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
	if((time.day>29)&&(time.month==2)&&((time.year%4==0)&&(time.year%10!=0)))
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
}

void wallclock_inc_second(void)
{
	time.second++;
	time.second_100=0;
	if(time.second>=60)
	{
		time.second=0;
		wallclock_inc_minute();
	}
}

void wallclock_inc_sec_100(void)
{
	time.second_100++;
	if(time.second_100>=100)
	{
		time.second_100=0;
		#IFNDEF PPS
		wallclock_inc_second();
		#ENDIF
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