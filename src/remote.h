void remote_init(void);
void remote_command(void);
void remote_feedback(void);

uint8_t command_offset=0;
char command[16];
char command_buffer[13];

boolean command_incoming=FALSE;
boolean command_waiting=FALSE;
boolean command_complete=FALSE;

void remote_feedback(void)
{
	fprintf(COM1,"\r\n%02u:%02u:%02u\r\n",time.hour,time.minute,time.second);
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
		delay_ms(10);
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
		if(command_offset==16)
		{
			command_incoming=FALSE;
			memset(command, 0, sizeof(command_buffer));
			memset(command, 0, sizeof(command));
			command_offset=0;
			fprintf(COM1, "Overflow!\r\n");
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
		write_eeprom(EEPROM_YEAR+1,(time.year&&0x00FF));
		write_eeprom(EEPROM_MONTH,time.month);
		write_eeprom(EEPROM_DAY,time.day);
		write_eeprom(EEPROM_HOUR,time.hour);
		write_eeprom(EEPROM_MINUTE,time.minute);
		write_eeprom(EEPROM_SECOND,time.second);	// Write current time to EEPROM
		delay_ms(10);
		reset_cpu();
	}
	if(command_complete) fprintf(COM1,"OK\r\n");
	memset(command_buffer, 0, sizeof(command_buffer));
}