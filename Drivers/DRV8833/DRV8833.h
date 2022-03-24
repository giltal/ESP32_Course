
#ifndef DRV8833_h
#define DRV8833_h

enum motorDir {FORWARD = 0, BACKWARD = 1};
enum motorIndex { MOTOR_A = 0, MOTOR_B = 1 };

class DRV8833
{
public:
	DRV8833(unsigned char A_IN1pin, unsigned char A_IN2pin, unsigned char B_IN1pin, unsigned char B_IN2pin, unsigned char A_PWM_Channel, unsigned char B_PWM_Channel, 
			void (*sleepFunc)(bool))
	{ 
		_sleep = sleepFunc;
		motorA_IN1pin = A_IN1pin;
		motorA_IN2pin = A_IN2pin;
		motorB_IN1pin = B_IN1pin;
		motorB_IN2pin = B_IN2pin;
		motorA_PWMch  = A_PWM_Channel;
		motorB_PWMch  = B_PWM_Channel;
		pinMode(motorA_IN1pin, OUTPUT);
		pinMode(motorA_IN2pin, OUTPUT);
		pinMode(motorB_IN1pin, OUTPUT);
		pinMode(motorB_IN2pin, OUTPUT);
		_sleep(true);

		motorAdir = FORWARD;
		motorBdir = FORWARD;
		ledcSetup(A_PWM_Channel, 100, 8);
		ledcAttachPin(A_IN1pin, A_PWM_Channel);
		digitalWrite(A_IN2pin, LOW);
		ledcWrite(A_PWM_Channel, 0);

		ledcSetup(B_PWM_Channel, 100, 8);
		ledcAttachPin(B_IN1pin, B_PWM_Channel);
		digitalWrite(B_IN2pin, LOW);
		ledcWrite(B_PWM_Channel, 0);
	};
	~DRV8833() {};
	void sleep(bool enable)
	{
		_sleep(enable);
	}
	void driveMotor(motorIndex motIndex, motorDir dir, unsigned char speed)
	{
		if (motIndex == MOTOR_A)
		{
			if (motorAdir == dir)
			{
				ledcWrite(motorA_PWMch, speed);
			}
			else
			{
				if (dir == FORWARD) // motor is spinning bakward
				{
					digitalWrite(motorA_IN1pin, LOW);
					ledcWrite(motorA_PWMch, 0);
					// switch direction
					ledcDetachPin(motorA_IN2pin);
					ledcAttachPin(motorA_IN1pin, motorA_PWMch);
					digitalWrite(motorA_IN2pin, LOW);
					ledcWrite(motorA_PWMch, speed);
				}
				else // motor is spinning forward
				{
					// Stop the motor
					digitalWrite(motorA_IN2pin, LOW);
					ledcWrite(motorA_PWMch, 0);
					// switch direction
					ledcDetachPin(motorA_IN1pin);
					ledcAttachPin(motorA_IN2pin, motorA_PWMch);
					digitalWrite(motorA_IN1pin, LOW);
					ledcWrite(motorA_PWMch, speed);
				}
				motorAdir = dir;
			}
		}
		if (motIndex == MOTOR_B)
		{
			if (motorBdir == dir)
			{
				ledcWrite(motorB_PWMch, speed);
			}
			else
			{
				if (dir == FORWARD) // motor is spinning bakward
				{
					digitalWrite(motorB_IN1pin, LOW);
					ledcWrite(motorB_PWMch, 0);
					// switch direction
					ledcDetachPin(motorB_IN2pin);
					ledcAttachPin(motorB_IN1pin, motorB_PWMch);
					digitalWrite(motorB_IN2pin, LOW);
					ledcWrite(motorB_PWMch, speed);
				}
				else // motor is spinning forward
				{
					// Stop the motor
					digitalWrite(motorB_IN2pin, LOW);
					ledcWrite(motorB_PWMch, 0);
					// switch direction
					ledcDetachPin(motorB_IN1pin);
					ledcAttachPin(motorB_IN2pin, motorB_PWMch);
					digitalWrite(motorB_IN1pin, LOW);
					ledcWrite(motorB_PWMch, speed);
				}
				motorBdir = dir;
			}
		}

	}

private:
	void (*_sleep)(bool);
	unsigned char motorA_IN1pin, motorA_IN2pin, motorB_IN1pin, motorB_IN2pin, motorA_PWMch, motorB_PWMch;
	motorDir motorAdir, motorBdir;
};
#endif