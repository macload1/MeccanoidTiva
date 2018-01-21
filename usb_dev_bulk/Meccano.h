
/* I/O Port Definitions */
#define SYSCTL_MECCANO_GPIO     SYSCTL_PERIPH_GPIOP
#define GPIO_MECCANO_BASE       GPIO_PORTP_BASE
#define GPIO_MECCANO_PIN        GPIO_PIN_4

/* Delay Definitions */
#define TIMER_LOADVALUE_417US	49736


#define TIMER_LOADVALUE_500US	59634
#define TIMER_LOADVALUE_1500US	178903
#define TIMER_LOADVALUE_3000US	357806
#define TIMER_LOADVALUE_10MS	1192686


/* Meccano Line Definitions */
#define	MECCANO_HEAD			0x00
#define	MECCANO_LEFT_ARM		0x01
#define	MECCANO_RIGHT_ARM		0x02


//    {"mecled",   CMD_mec_led,   " : set mecled [r] [g] [b] [t] => [0..7]"},
//    {"mecsled",  CMD_mec_sled,  " : set mecsled [n] [clr] => n = [0..3], clr = [240..247]"},
//    {"mecspos",  CMD_mec_spos,  " : set mecspos [n] [pos] => n = [0..3], pos = [0..239]"},



void MeccanoInit(void);
void setMeccanoLEDColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t fadetime);
void setMeccanoServoColor(uint8_t servoNum, uint8_t color);
void setMeccanoServoPosition(uint8_t servoNum, uint8_t pos);
void setMeccanoServotoLIM(uint8_t servoNum);
uint8_t getMeccanoServoPosition(uint8_t servoNum);
uint8_t calculateCheckSum(uint8_t Data1, uint8_t Data2, uint8_t Data3, uint8_t Data4);
