uint8_t t10ms=0;
uint8_t t10ms0=0;
uint8_t t100ms=0;
uint8_t t100ms0=0;
uint8_t t100ms1=0;
uint8_t t1s0=0;
uint8_t t1s1=0;

#INT_TIMER3
void scheduler(void)
{
	set_timer3(-20000);
	t10ms++;
	t10ms0++;
	if(t10ms==10)
	{
		t10ms=0;
		t100ms++;
		t100ms0++;
		t100ms1++;
		if(t100ms==10)
		{
			t100ms=0;
			t1s0++;
			t1s1++;
		}
	}
}

void reset_scheduler(void)
{
	t10ms=0;
	t10ms0=0;
	t100ms=0;
	t100ms0=0;
	t100ms1=0;
	t1s0=0;
	t1s1=0;
	set_timer3(-20000);
}