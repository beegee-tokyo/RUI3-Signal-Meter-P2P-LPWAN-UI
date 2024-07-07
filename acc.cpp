/**
 * @file acc.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Lis3DH acc sensor functions
 * @version 0.1
 * @date 2024-06-24
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "app.h"

void acc_int_callback(void);

/** The LIS3DH sensor */
LIS3DH acc_sensor(I2C_MODE, 0x18);

/**
 * @brief Initialize LIS3DH 3-axis
 * acceleration sensor
 *
 * @return true If sensor was found and is initialized
 * @return false If sensor initialization failed
 */
bool init_acc(bool active)
{
	// Setup interrupt pin
	pinMode(ACC_INT_PIN, INPUT);

	Wire.begin();

	if (active)
	{
		acc_sensor.settings.accelSampleRate = 10; // Hz.  Can be: 0,1,10,25,50,100,200,400,1600,5000 Hz
		acc_sensor.settings.accelRange = 2;		  // Max G force readable.  Can be: 2, 4, 8, 16

		acc_sensor.settings.adcEnabled = 0;
		acc_sensor.settings.tempEnabled = 0;
		acc_sensor.settings.xAccelEnabled = 1;
		acc_sensor.settings.yAccelEnabled = 1;
		acc_sensor.settings.zAccelEnabled = 1;
	}
	else
	{
		acc_sensor.settings.accelSampleRate = 0; // Hz.  Can be: 0,1,10,25,50,100,200,400,1600,5000 Hz
		acc_sensor.settings.accelRange = 2;		  // Max G force readable.  Can be: 2, 4, 8, 16

		acc_sensor.settings.adcEnabled = 0;
		acc_sensor.settings.tempEnabled = 0;
		acc_sensor.settings.xAccelEnabled = 0;
		acc_sensor.settings.yAccelEnabled = 0;
		acc_sensor.settings.zAccelEnabled = 0;
	}

	if (acc_sensor.begin() != 0)
	{
		MYLOG("ACC", "ACC sensor initialization failed");
		return false;
	}

	uint8_t data_to_write = 0;
	if (active)
	{
		// Enable interrupts
		data_to_write |= 0x20;									  // Z high
		data_to_write |= 0x08;									  // Y high
		data_to_write |= 0x02;									  // X high
		acc_sensor.writeRegister(LIS3DH_INT1_CFG, data_to_write); // Enable interrupts on high tresholds for x, y and z

		// Set interrupt trigger range
		data_to_write = 0;
		// data_to_write |= 0x10;									  // 1/8 range
		// acc_sensor.writeRegister(LIS3DH_INT1_THS, data_to_write); // 1/8th range
		data_to_write |= 0x08;									  // 1/16 range
		acc_sensor.writeRegister(LIS3DH_INT1_THS, data_to_write); // 1/16th range

		// Set interrupt signal length
		data_to_write = 0;
		data_to_write |= 0x01; // 1 * 1/50 s = 20ms
		acc_sensor.writeRegister(LIS3DH_INT1_DURATION, data_to_write);

		acc_sensor.readRegister(&data_to_write, LIS3DH_CTRL_REG5);
		data_to_write &= 0xF3;									   // Clear bits of interest
		data_to_write |= 0x08;									   // Latch interrupt (Cleared by reading int1_src)
		acc_sensor.writeRegister(LIS3DH_CTRL_REG5, data_to_write); // Set interrupt to latching

		// Select interrupt pin 1
		data_to_write = 0;
		data_to_write |= 0x40; // AOI1 event (Generator 1 interrupt on pin 1)
		data_to_write |= 0x20; // AOI2 event ()
		acc_sensor.writeRegister(LIS3DH_CTRL_REG3, data_to_write);

		// No interrupt on pin 2
		acc_sensor.writeRegister(LIS3DH_CTRL_REG6, 0x00);

		// Enable high pass filter
		acc_sensor.writeRegister(LIS3DH_CTRL_REG2, 0x01);
	}

	// Set low power mode
	data_to_write = 0;
	acc_sensor.readRegister(&data_to_write, LIS3DH_CTRL_REG1);
	data_to_write |= 0x08;
	acc_sensor.writeRegister(LIS3DH_CTRL_REG1, data_to_write);
	delay(100);

	data_to_write = 0;
	acc_sensor.readRegister(&data_to_write, 0x1E);
	data_to_write |= 0x90;
	acc_sensor.writeRegister(0x1E, data_to_write);
	delay(100);

	if (active)
	{
		clear_acc_int();

		// Set the interrupt callback function
		attachInterrupt(ACC_INT_PIN, acc_int_callback, RISING);

		read_acc();
	}
	else
	{
		// Power-Down mode
		acc_sensor.readRegister(&data_to_write, LIS3DH_CTRL_REG1);
		data_to_write &= 0x40;
		acc_sensor.writeRegister(LIS3DH_CTRL_REG1, data_to_write);
	}
	return true;
}

void read_acc(void)
{
	int16_t acc_x = (int16_t)(acc_sensor.readFloatAccelX() * 1000.0);
	int16_t acc_y = (int16_t)(acc_sensor.readFloatAccelY() * 1000.0);
	int16_t acc_z = (int16_t)(acc_sensor.readFloatAccelZ() * 1000.0);

	MYLOG("ACC", "X %.3f %.3f %d", acc_sensor.readFloatAccelX(), acc_sensor.readFloatAccelX() * 1000.0, acc_x);
	MYLOG("ACC", "Y %.3f %.3f %d", acc_sensor.readFloatAccelY(), acc_sensor.readFloatAccelY() * 1000.0, acc_y);
	MYLOG("ACC", "Z %.3f %.3f %d", acc_sensor.readFloatAccelZ(), acc_sensor.readFloatAccelZ() * 1000.0, acc_z);

	// g_tracker_data.acc_x_1 = (int8_t)(acc_x >> 8);
	// g_tracker_data.acc_x_2 = (int8_t)(acc_x);
	// g_tracker_data.acc_y_1 = (int8_t)(acc_y >> 8);
	// g_tracker_data.acc_y_2 = (int8_t)(acc_y);
	// g_tracker_data.acc_z_1 = (int8_t)(acc_z >> 8);
	// g_tracker_data.acc_z_2 = (int8_t)(acc_z);
}

/**
 * @brief ACC interrupt handler
 * @note gives semaphore to wake up main loop
 *
 */
void acc_int_callback(void)
{
	// g_task_event_type |= ACC_TRIGGER;
	// xSemaphoreGiveFromISR(g_task_sem, pdFALSE);
}

/**
 * @brief Clear ACC interrupt register to enable next wakeup
 *
 */
void clear_acc_int(void)
{
	uint8_t data_read;
	acc_sensor.readRegister(&data_read, LIS3DH_INT1_SRC);
	if (data_read & 0x40)
		MYLOG("ACC", "Interrupt Active 0x%X\n", data_read);
	if (data_read & 0x20)
		MYLOG("ACC", "Z high");
	if (data_read & 0x10)
		MYLOG("ACC", "Z low");
	if (data_read & 0x08)
		MYLOG("ACC", "Y high");
	if (data_read & 0x04)
		MYLOG("ACC", "Y low");
	if (data_read & 0x02)
		MYLOG("ACC", "X high");
	if (data_read & 0x01)
		MYLOG("ACC", "X low");
}
