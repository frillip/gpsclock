#define EEPROM_RESET		0x00
#define EEPROM_YEAR			0x01
#define EEPROM_MONTH		0x03
#define EEPROM_DAY			0x04
#define EEPROM_HOUR			0x05
#define EEPROM_MINUTE		0x06
#define EEPROM_SECOND		0x07
#define EEPROM_SECOND_100	0x08

typedef struct {
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
	uint8_t second_100;
} elapsed;

typedef struct {
	int8_t hour;
	int8_t minute;
} offset;

elapsed time = {0,0,0,0,0,0,0};
offset timezone = {0,0};

void wallclock_inc_sec_100(void)
{
	time.second_100++;
}