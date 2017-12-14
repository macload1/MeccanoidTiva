

#ifndef __SERVO_H__
#define __SERVO_H__

//*****************************************************************************
//
// PWM arm definitions.
//
//*****************************************************************************
#define PWM_HAND		PWM_OUT_7		// PK5
#define	PWM_ARM2		PWM_OUT_6		// PK4
#define	PWM_ARM3		PWM_OUT_5		// PG1
#define	PWM_ARM4		PWM_OUT_4		// PG0
#define	PWM_ARM5		PWM_OUT_3		// PF3
#define	PWM_ARM6		PWM_OUT_2		// PF2
#define	PWM_ARM7		PWM_OUT_1		// PF1
#define	PWM_ARM8		PWM_OUT_0		// PF0

// Initialise PWM peripheral
void initPWM(void);




#endif // __SERVO_H__
