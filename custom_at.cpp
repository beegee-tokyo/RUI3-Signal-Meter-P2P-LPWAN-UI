/**
 * @file custom_at.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Custom AT commands for the application
 * @version 0.1
 * @date 2023-12-29
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "app.h"

#if defined(_VARIANT_RAK3172_) || defined(_VARIANT_RAK3172_SIP_)
#define AT_PRINTF(...)              \
	do                              \
	{                               \
		Serial.printf(__VA_ARGS__); \
		Serial.printf("\r\n");      \
	} while (0);                    \
	delay(100)
#else // RAK4630 || RAK11720
#define AT_PRINTF(...)               \
	do                               \
	{                                \
		Serial.printf(__VA_ARGS__);  \
		Serial.printf("\r\n");       \
		Serial6.printf(__VA_ARGS__); \
		Serial6.printf("\r\n");      \
	} while (0);                     \
	delay(100)
#endif

/** Custom flash parameters */
custom_param_s g_custom_parameters;

// Forward declarations
int interval_send_handler(SERIAL_PORT port, char *cmd, stParam *param);
int status_handler(SERIAL_PORT port, char *cmd, stParam *param);
int test_mode_handler(SERIAL_PORT port, char *cmd, stParam *param);
int custom_pckg_handler(SERIAL_PORT port, char *cmd, stParam *param);

/**
 * @brief Add send interval AT command
 *
 * @return true if success
 * @return false if failed
 */
bool init_interval_at(void)
{
	return api.system.atMode.add((char *)"SENDINT",
								 (char *)"Set/Get the interval sending time values in seconds 0 = off, max 2,147,483 seconds",
								 (char *)"SENDINT", interval_send_handler,
								 RAK_ATCMD_PERM_WRITE | RAK_ATCMD_PERM_READ);
}

/**
 * @brief Handler for send interval AT command
 *
 * @param port Serial port used
 * @param cmd char array with the received AT command
 * @param param char array with the received AT command parameters
 * @return int result of command parsing
 * 			AT_OK AT command & parameters valid
 * 			AT_PARAM_ERROR command or parameters invalid
 */
int interval_send_handler(SERIAL_PORT port, char *cmd, stParam *param)
{
	if (param->argc == 1 && !strcmp(param->argv[0], "?"))
	{
		AT_PRINTF("%s=%ld", cmd, g_custom_parameters.send_interval / 1000);
	}
	else if (param->argc == 1)
	{
		MYLOG("AT_CMD", "param->argv[0] >> %s", param->argv[0]);
		for (int i = 0; i < strlen(param->argv[0]); i++)
		{
			if (!isdigit(*(param->argv[0] + i)))
			{
				MYLOG("AT_CMD", "%d is no digit", i);
				return AT_PARAM_ERROR;
			}
		}

		uint32_t new_send_freq = strtoul(param->argv[0], NULL, 10);

		MYLOG("AT_CMD", "Requested interval %ld", new_send_freq);

		g_custom_parameters.send_interval = new_send_freq * 1000;

		MYLOG("AT_CMD", "New interval %ld", g_custom_parameters.send_interval);
		// Stop the timer
		api.system.timer.stop(RAK_TIMER_0);
		if (g_custom_parameters.send_interval != 0)
		{
			// Restart the timer
			api.system.timer.start(RAK_TIMER_0, g_custom_parameters.send_interval, NULL);
		}
		// Save custom settings
		save_at_setting();
	}
	else
	{
		return AT_PARAM_ERROR;
	}

	return AT_OK;
}

/**
 * @brief Add test mode AT command
 *
 * @return true if success
 * @return false if failed
 */
bool init_test_mode_at(void)
{
	return api.system.atMode.add((char *)"MODE",
								 (char *)"Set/Get the test mode. 0 = LPWAN LinkCheck, 1 = LoRa P2P, 2 = Field Tester",
								 (char *)"MODE", test_mode_handler,
								 RAK_ATCMD_PERM_WRITE | RAK_ATCMD_PERM_READ);
}

/**
 * @brief Handler for test mode AT command
 *
 * @param port Serial port used
 * @param cmd char array with the received AT command
 * @param param char array with the received AT command parameters
 * @return int result of command parsing
 * 			AT_OK AT command & parameters valid
 * 			AT_PARAM_ERROR command or parameters invalid
 */
int test_mode_handler(SERIAL_PORT port, char *cmd, stParam *param)
{
	if (param->argc == 1 && !strcmp(param->argv[0], "?"))
	{
		AT_PRINTF("%s=%d", cmd, g_custom_parameters.test_mode);
	}
	else if (param->argc == 1)
	{
		MYLOG("AT_CMD", "param->argv[0] >> %s", param->argv[0]);
		for (int i = 0; i < strlen(param->argv[0]); i++)
		{
			if (!isdigit(*(param->argv[0] + i)))
			{
				MYLOG("AT_CMD", "%d is no digit", i);
				return AT_PARAM_ERROR;
			}
		}

		uint32_t new_mode = strtoul(param->argv[0], NULL, 10);
		uint8_t old_mode = g_custom_parameters.test_mode;

		MYLOG("AT_CMD", "Requested mode %ld", new_mode);

		if (new_mode > 2)
		{
			return AT_PARAM_ERROR;
		}

		if (new_mode != old_mode)
		{
			bool restart = true;
			if (((old_mode == 0) && (new_mode == 1)) || ((old_mode == 1) && (new_mode == 0)))
			{
				MYLOG("AT_CMD", "Switch within LPWAN modes");
				restart = false;
			}

			g_custom_parameters.test_mode = new_mode;
			MYLOG("AT_CMD", "New test mode %ld", g_custom_parameters.test_mode);

			// Save custom settings
			save_at_setting();

			// Switch mode
			switch (g_custom_parameters.test_mode)
			{
			case MODE_LINKCHECK:
				set_linkcheck();
				break;
			case MODE_P2P:
				set_p2p();
				break;
			case MODE_FIELDTESTER:
				set_field_tester();
				break;
			}

			// If switching between LoRaWAN and LoRa P2P the device needs to restart
			if (restart)
			{
				AT_PRINTF("+EVT:RESTART_FOR_MODE_CHANGE");
				delay(5000);
				api.system.reboot();
			}
		}
	}
	else
	{
		return AT_PARAM_ERROR;
	}

	return AT_OK;
}

/**
 * @brief Add custom packet AT command
 *
 * @return true if success
 * @return false if failed
 */
bool init_custom_pckg_at(void)
{
	return api.system.atMode.add((char *)"PCKG",
								 (char *)"Set/Get a custom packet (max 64 bytes)",
								 (char *)"PCKG", custom_pckg_handler,
								 RAK_ATCMD_PERM_WRITE | RAK_ATCMD_PERM_READ);
}

/**
 * @brief Handler for custom packet AT command
 *
 * @param port Serial port used
 * @param cmd char array with the received AT command
 * @param param char array with the received AT command parameters
 * @return int result of command parsing
 * 			AT_OK AT command & parameters valid
 * 			AT_PARAM_ERROR command or parameters invalid
 */
int custom_pckg_handler(SERIAL_PORT port, char *cmd, stParam *param)
{
	char temp_str[257];
	if (param->argc == 1 && !strcmp(param->argv[0], "?"))
	{
		if (g_custom_parameters.custom_packet_len == 0)
		{
			AT_PRINTF("%s=01020304", cmd);
		}
		else
		{
			atcmd_printf("%s=", cmd);
			for (uint8_t i = 0; i < g_custom_parameters.custom_packet_len; i++)
			{
				atcmd_printf("%02X", g_custom_parameters.custom_packet[i]);
			}
			atcmd_printf("\r\n");
		}
	}
	else if (param->argc == 1)
	{
		uint32_t len = strlen(param->argv[0]);
		MYLOG("AT_CMD", "param->argv[0] >> %s", param->argv[0]);
		if (0 != at_check_hex_param(param->argv[0], len, g_custom_parameters.custom_packet))
		{
			MYLOG("AT_CMD", "Invalid HEX ASCII string");
			return AT_PARAM_ERROR;
		}

		g_custom_parameters.custom_packet_len = len / 2;

		// Save custom settings
		save_at_setting();
	}
	else
	{
		return AT_PARAM_ERROR;
	}

	return AT_OK;
}

/**
 * @brief Add custom Status AT command
 *
 * @return true AT command were added
 * @return false AT command couldn't be added
 */
bool init_status_at(void)
{
	return api.system.atMode.add((char *)"STATUS",
								 (char *)"Get device information",
								 (char *)"STATUS", status_handler,
								 RAK_ATCMD_PERM_READ);
}

/** Regions as text array */
char *g_regions_list[] = {"EU433", "CN470", "RU864", "IN865", "EU868", "US915", "AU915", "KR920", "AS923", "AS923-2", "AS923-3", "AS923-4", "LA915"};
/** Network modes as text array*/
char *nwm_list[] = {"P2P", "LoRaWAN", "FSK"};
/** Available test modes as text array */
char *test_mode_list[] = {"LinkCheck", "Confirmed Packet", "LoRa P2P"};
/**
 * @brief Print device status over Serial
 *
 * @param port Serial port used
 * @param cmd char array with the received AT command
 * @param param char array with the received AT command parameters
 * @return int result of command parsing
 * 			AT_OK AT command & parameters valid
 * 			AT_PARAM_ERROR command or parameters invalid
 */
int status_handler(SERIAL_PORT port, char *cmd, stParam *param)
{
	String value_str = "";
	int nw_mode = 0;
	int region_set = 0;
	uint8_t key_eui[16] = {0}; // efadff29c77b4829acf71e1a6e76f713

	if ((param->argc == 1 && !strcmp(param->argv[0], "?")) || (param->argc == 0))
	{
		AT_PRINTF("Device Status:");
		AT_PRINTF("Test Mode: %s", test_mode_list[g_custom_parameters.test_mode]);
		value_str = api.system.hwModel.get();
		value_str.toUpperCase();
		AT_PRINTF("Module: %s", value_str.c_str());
		AT_PRINTF("Version: %s", api.system.firmwareVer.get().c_str());
		AT_PRINTF("Send time: %d s", g_custom_parameters.send_interval / 1000);
		/// \todo
		nw_mode = api.lorawan.nwm.get();
		AT_PRINTF("Network mode %s", nwm_list[nw_mode]);
		if (nw_mode == 1)
		{
			AT_PRINTF("Network %s", api.lorawan.njs.get() ? "joined" : "not joined");
			region_set = api.lorawan.band.get();
			AT_PRINTF("Region: %d", region_set);
			AT_PRINTF("Region: %s", g_regions_list[region_set]);
			if (api.lorawan.njm.get())
			{
				AT_PRINTF("OTAA mode");
				api.lorawan.deui.get(key_eui, 8);
				AT_PRINTF("DevEUI=%02X%02X%02X%02X%02X%02X%02X%02X",
						  key_eui[0], key_eui[1], key_eui[2], key_eui[3],
						  key_eui[4], key_eui[5], key_eui[6], key_eui[7]);
				api.lorawan.appeui.get(key_eui, 8);
				AT_PRINTF("AppEUI=%02X%02X%02X%02X%02X%02X%02X%02X",
						  key_eui[0], key_eui[1], key_eui[2], key_eui[3],
						  key_eui[4], key_eui[5], key_eui[6], key_eui[7]);
				api.lorawan.appkey.get(key_eui, 16);
				AT_PRINTF("AppKey=%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
						  key_eui[0], key_eui[1], key_eui[2], key_eui[3],
						  key_eui[4], key_eui[5], key_eui[6], key_eui[7],
						  key_eui[8], key_eui[9], key_eui[10], key_eui[11],
						  key_eui[12], key_eui[13], key_eui[14], key_eui[15]);
			}
			else
			{
				AT_PRINTF("ABP mode");
				api.lorawan.appskey.get(key_eui, 16);
				AT_PRINTF("AppsKey=%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
						  key_eui[0], key_eui[1], key_eui[2], key_eui[3],
						  key_eui[4], key_eui[5], key_eui[6], key_eui[7],
						  key_eui[8], key_eui[9], key_eui[10], key_eui[11],
						  key_eui[12], key_eui[13], key_eui[14], key_eui[15]);
				api.lorawan.nwkskey.get(key_eui, 16);
				AT_PRINTF("NwksKey=%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
						  key_eui[0], key_eui[1], key_eui[2], key_eui[3],
						  key_eui[4], key_eui[5], key_eui[6], key_eui[7],
						  key_eui[8], key_eui[9], key_eui[10], key_eui[11],
						  key_eui[12], key_eui[13], key_eui[14], key_eui[15]);
				api.lorawan.daddr.get(key_eui, 4);
				AT_PRINTF("DevAddr=%02X%02X%02X%02X",
						  key_eui[0], key_eui[1], key_eui[2], key_eui[3]);
			}
		}
		else if (nw_mode == 0)
		{
			AT_PRINTF("Frequency = %d", api.lora.pfreq.get());
			AT_PRINTF("SF = %d", api.lora.psf.get());
			AT_PRINTF("BW = %d", api.lora.pbw.get());
			AT_PRINTF("CR = %d", api.lora.pcr.get());
			AT_PRINTF("Preamble length = %d", api.lora.ppl.get());
			AT_PRINTF("TX power = %d", api.lora.ptp.get());
		}
		else
		{
			AT_PRINTF("Frequency = %d", api.lora.pfreq.get());
			AT_PRINTF("Bitrate = %d", api.lora.pbr.get());
			AT_PRINTF("Deviaton = %d", api.lora.pfdev.get());
		}
		AT_PRINTF("Custom settings");
		AT_PRINTF("Testmode = %d", g_custom_parameters.test_mode);
		AT_PRINTF("Display saver %s", g_custom_parameters.display_saver ? "On" : "off");
		atcmd_printf("Custom Packet = ");
		for (uint8_t i = 0; i < g_custom_parameters.custom_packet_len; i++)
		{
			atcmd_printf("%02X", g_custom_parameters.custom_packet[i]);
		}
		atcmd_printf("\r\n");
	}
	else
	{
		return AT_PARAM_ERROR;
	}
	return AT_OK;
}

/**
 * @brief Get setting from flash
 *
 * @return false read from flash failed or invalid settings type
 */
bool get_at_setting(void)
{
	bool found_problem = false;

	custom_param_s temp_params;
	uint8_t *flash_value = (uint8_t *)&temp_params.valid_flag;
	if (!api.system.flash.get(0, flash_value, sizeof(custom_param_s)))
	{
		MYLOG("AT_CMD", "Failed to read send interval from Flash");
		return false;
	}
	MYLOG("AT_CMD", "Got flag: %02X", temp_params.valid_flag);
	MYLOG("AT_CMD", "Got send interval: %08X", temp_params.send_interval);
	if (flash_value[0] != 0xAA)
	{
		MYLOG("AT_CMD", "No valid settings found, set to default, read 0X%08X", temp_params.send_interval);
		g_custom_parameters.send_interval = 0;
		g_custom_parameters.test_mode = 0;
		g_custom_parameters.display_saver = false;
		g_custom_parameters.location_on = false;
		g_custom_parameters.custom_packet[0] = 0x01;
		g_custom_parameters.custom_packet[1] = 0x02;
		g_custom_parameters.custom_packet[2] = 0x03;
		g_custom_parameters.custom_packet[3] = 0x04;
		g_custom_parameters.custom_packet_len = 4;
		save_at_setting();
		return false;
	}
	g_custom_parameters.send_interval = temp_params.send_interval;

	if (temp_params.test_mode > 2)
	{
		MYLOG("AT_CMD", "Invalid test mode found %d", temp_params.test_mode);
		g_custom_parameters.test_mode = 0;
		save_at_setting();
	}
	else
	{
		g_custom_parameters.test_mode = temp_params.test_mode;
	}

	if (temp_params.display_saver > 1)
	{
		MYLOG("AT_CMD", "Invalid display mode found %d", temp_params.display_saver);
		g_custom_parameters.display_saver = false;
		save_at_setting();
	}
	else
	{
		g_custom_parameters.display_saver = temp_params.display_saver;
	}

	if (temp_params.location_on > 1)
	{
		MYLOG("AT_CMD", "Invalid location mode found %d", temp_params.location_on);
		g_custom_parameters.location_on = false;
		save_at_setting();
	}
	else
	{
		g_custom_parameters.location_on = temp_params.location_on;
	}

	if (temp_params.custom_packet_len > 128)
	{
		MYLOG("AT_CMD", "Invalid packet_len found %d", temp_params.custom_packet_len);
		g_custom_parameters.custom_packet[0] = 0x00;
		g_custom_parameters.custom_packet_len = 0;
	}
	else
	{
		g_custom_parameters.custom_packet_len = temp_params.custom_packet_len;
		memcpy(g_custom_parameters.custom_packet, temp_params.custom_packet, g_custom_parameters.custom_packet_len);
	}

	MYLOG("AT_CMD", "Send interval found %ld", g_custom_parameters.send_interval);
	MYLOG("AT_CMD", "Test mode found %d", g_custom_parameters.test_mode);
	MYLOG("AT_CMD", "Display mode found %s", g_custom_parameters.display_saver ? "On" : "Off");
	MYLOG("AT_CMD", "Location mode found %s", g_custom_parameters.test_mode ? "On" : "Off");

	char temp[258] = {0x00};
	for (uint8_t i = 0; i < g_custom_parameters.custom_packet_len; i++)
	{
		sprintf(&temp[i * 2], "%02X", g_custom_parameters.custom_packet[i]);
	}
	MYLOG("AT_CMD", "Custom packet %s", temp);
	MYLOG("AT_CMD", "Custom packet len %d", g_custom_parameters.custom_packet_len);
	return true;
}

/**
 * @brief Save setting to flash
 *
 * @return true write to flash was successful
 * @return false write to flash failed or invalid settings type
 */
bool save_at_setting(void)
{
	custom_param_s temp_params;
	uint8_t *flash_value = (uint8_t *)&temp_params.valid_flag;
	temp_params.send_interval = g_custom_parameters.send_interval;
	temp_params.test_mode = g_custom_parameters.test_mode;
	temp_params.display_saver = g_custom_parameters.display_saver;
	temp_params.location_on = g_custom_parameters.location_on;
	memcpy(temp_params.custom_packet, g_custom_parameters.custom_packet, g_custom_parameters.custom_packet_len);
	temp_params.custom_packet_len = g_custom_parameters.custom_packet_len;

	bool wr_result = false;
	MYLOG("AT_CMD", "Writing flag: %02X", temp_params.valid_flag);
	MYLOG("AT_CMD", "Writing send interval 0X%08X ", temp_params.send_interval);
	MYLOG("AT_CMD", "Writing test mode %d ", temp_params.test_mode);
	MYLOG("AT_CMD", "Writing display mode %s ", temp_params.display_saver ? "On" : "Off");
	MYLOG("AT_CMD", "Writing location mode %s ", temp_params.location_on ? "On" : "Off");
	char temp[258] = {0x00};
	for (uint8_t i = 0; i < temp_params.custom_packet_len; i++)
	{
		sprintf(&temp[i * 2], "%02X", temp_params.custom_packet[i]);
	}
	MYLOG("AT_CMD", "Writing custom packet %s", temp);
	MYLOG("AT_CMD", "Writing custom packet len %d", temp_params.custom_packet_len);

	wr_result = api.system.flash.set(0, flash_value, sizeof(custom_param_s));
	if (!wr_result)
	{
		// Retry
		wr_result = api.system.flash.set(0, flash_value, sizeof(custom_param_s));
	}
	return wr_result;
}
