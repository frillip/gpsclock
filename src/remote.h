void remote_init(void);
void remote_command(void);
void remote_feedback(void);

uint8_t command_offset=0;
char command[128];
char command_buffer[128];

boolean command_incoming=FALSE;
boolean command_waiting=FALSE;
boolean command_complete=FALSE;

void remote_feedback(void)
{
	fprintf(COM1,"\r\n%04lu/%02u/%02u %02u:%02u:%02u\r\n",time.year,time.month,time.day,time.hour,time.minute,time.second);
}

void brightness_feedback(void)
{
	fprintf(COM1,"%02X\r\n",display_brightness);
}

uint8_t strcmp(unsigned char *s1, unsigned char *s2)
{
   for (; *s1 == *s2; s1++, s2++)
      if (*s1 == '\0')
         return(1);
   return(0);
}

uint8_t strncmp(unsigned char *s1, unsigned char *s2, uint8_t n)
{
   for (; n > 0; s1++, s2++, n--)
      if (*s1 != *s2)
         return(0);
      else if (*s1 == '\0')
         return(1);
   return(1);
}

#INT_RDA
void remote_command(void)
{
	command[command_offset]=fgetc(COM1);
	if(command[command_offset]==0xff)
	{
		delay_us(100);
		reset_cpu();
	}
	fprintf(COM1,"%c", command[command_offset]);
	if(command[command_offset]==0xd) fprintf(COM1,"\n");
	if(command_incoming)
	{
		command_buffer[command_offset-3]=command[command_offset];
		if((command[command_offset]==0x00)||(command[command_offset]==0x0d))
		{
			command_incoming=FALSE;
			command_waiting=TRUE;
			command[command_offset]=0x00;
			command_buffer[command_offset-3]=command[command_offset];
			memset(command, 0, sizeof(command));
			command_offset=0;
			return;
		}
		command_offset++;
		if(command_offset==128)
		{
			command_incoming=FALSE;
			memset(command, 0, sizeof(command_buffer));
			memset(command, 0, sizeof(command));
			command_offset=0;
			//fprintf(COM1, "!\r\n");
		}
	}
	else
	{
		if((command_offset==0)&&(command[0]=='A'))
		{
			command_offset++;
		}
		else if((command_offset==1)&&(command[1]=='T'))
		{
			command_offset++;
		}
		else if ((command_offset==2)&&(command[2]=='+'))
		{
			command_offset++;
			command_incoming=TRUE;
		}
		else command_offset=0;
	}
}

void process_command(void)
{
	command_waiting=FALSE;
	command_complete=FALSE;
	if(strcmp(command_buffer,"RESET"))
	{
		fprintf(COM1, "Resetting...\r\n");
		write_eeprom(EEPROM_RESET,0x42);			// Write reset flag
		write_eeprom(EEPROM_YEAR,(time.year>>8));
		write_eeprom(EEPROM_YEAR+1,(time.year&0x00FF));
		write_eeprom(EEPROM_MONTH,time.month);
		write_eeprom(EEPROM_DAY,time.day);
		write_eeprom(EEPROM_HOUR,time.hour);
		write_eeprom(EEPROM_MINUTE,time.minute);
		write_eeprom(EEPROM_SECOND,time.second);	// Write current time to EEPROM
		delay_ms(10);
		reset_cpu();
	}
	else if(strncmp(command_buffer,"TIME",4))
	{
		if(command_buffer[9])
		{
			time.second=(((uint8_t)command_buffer[8]-48)*10)+((uint8_t)command_buffer[9]-48);
			time.second_100=0;
			reset_scheduler();
//			set_timer1(-32768);
		}
		if(command_buffer[7])
		{
			time.minute=(((uint8_t)command_buffer[6]-48)*10)+((uint8_t)command_buffer[7]-48);
			time.hour=(((uint8_t)command_buffer[4]-48)*10)+((uint8_t)command_buffer[5]-48);
			update_display0();
			update_display1();
		}
		remote_feedback();
		command_complete=TRUE;
	}
else if(strncmp(command_buffer,"DATE",4))
	{
		if(command_buffer[11])
		{
			time.day=(((uint8_t)command_buffer[10]-48)*10)+((uint8_t)command_buffer[11]-48);
			time.month=(((uint8_t)command_buffer[8]-48)*10)+((uint8_t)command_buffer[9]-48);
			time.year=(((uint16_t)command_buffer[4]-48)*1000)+(((uint16_t)command_buffer[5]-48)*100)+(((uint16_t)command_buffer[6]-48)*10)+((uint16_t)command_buffer[7]-48);
			update_display0();
			update_display1();
		}
		remote_feedback();
		command_complete=TRUE;
	}
else if(strncmp(command_buffer,"BRIGHT",6))
	{
		if((command_buffer[8])&&(((uint8_t)command_buffer[6]-48)<3))display_brightness=(((uint8_t)command_buffer[6]-48)*100)+(((uint8_t)command_buffer[7]-48)*10)+((uint8_t)command_buffer[8]-48);
		else if(command_buffer[7])display_brightness=(((uint8_t)command_buffer[7]-48)*10)+((uint8_t)command_buffer[8]-48);
		else if(command_buffer[6])display_brightness=(uint8_t)command_buffer[6]-48;
		update_brightness();
		brightness_feedback();
		command_complete=TRUE;
	}
else if(strncmp(command_buffer,"TIMEZONE",8))
	{
		timezone.hour=(((uint8_t)command_buffer[8]-48)*10)+((uint8_t)command_buffer[9]-48);
		if(timezone.hour>14)timezone.hour=0;
		timezone.minute=(((uint8_t)command_buffer[10]-48)*10)+((uint8_t)command_buffer[11]-48);
		if(timezone.minute>45)timezone.minute=0;
		write_eeprom(EEPROM_TZ_HOUR,timezone.hour);
		write_eeprom(EEPROM_TZ_MINUTE,timezone.minute);
		remote_feedback();
		command_complete=TRUE;
	}
	if(command_complete) fprintf(COM1,"OK\r\n");
	memset(command_buffer, 0, sizeof(command_buffer));
}