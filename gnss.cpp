/**
 * @file gnss.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief GNSS module functions
 * @version 0.1
 * @date 2024-06-24
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "app.h"

// The GNSS object
SFE_UBLOX_GNSS my_gnss;

// GNSS functions
#define NO_GNSS_INIT 0
#define RAK1910_GNSS 1
#define RAK12500_GNSS 2

// Fake GPS Enable (1) Disable (0)
#define FAKE_GPS 0

/** The GPS module to use */
uint8_t g_gnss_option = 0;

/** Flag if location was found */
volatile bool last_read_ok = false;

/** Latitude */
int64_t latitude = 0;
/** Longitude */
int64_t longitude = 0;
/** Altitude */
int32_t altitude = 0;
/** Accuracy HDOP */
float accuracy = 0;
/** Number of satellites */
uint8_t satellites = 0;

/** Last latitude for global use */
volatile float g_last_lat = 0.0;
/** Last longitude for global use */
volatile float g_last_long = 0.0;
/** Last accuracy for global use */
volatile float g_last_accuracy = 0.0;
/** Last altitude for global use */
volatile uint32_t g_last_altitude = 0;
/** Last number of satellites */
volatile uint8_t g_last_satellites = 0;

/** Counter for GNSS readings */
uint16_t check_gnss_counter = 0;
/** Max number of GNSS readings before giving up */
uint16_t check_gnss_max_try = 0;

/** Max number of satellites seen */
uint8_t max_sat = 0;
/** Number of checks with unchanged number of satellites seen */
uint8_t max_sat_unchanged = 0;

/**
 * @brief Initialize the GNSS
 *
 */
bool init_gnss(bool active)
{
	// Power on the GNSS module
	digitalWrite(WB_IO2, HIGH);

	// Give the module some time to power up
	delay(500);

	if (g_gnss_option == NO_GNSS_INIT)
	{
		if (!my_gnss.begin())
		{
			MYLOG("GNSS", "UBLOX did not answer on I2C");
			return false;
		}

		g_gnss_option = RAK12500_GNSS;
		MYLOG("GNSS", "UBLOX found on I2C");
		my_gnss.setI2COutput(COM_TYPE_UBX); // Set the I2C port to output UBX only (turn off NMEA noise)

		if (active)
		{
			my_gnss.enableGNSS(true, SFE_UBLOX_GNSS_ID_GPS);
			my_gnss.enableGNSS(true, SFE_UBLOX_GNSS_ID_GALILEO);
			my_gnss.enableGNSS(true, SFE_UBLOX_GNSS_ID_GLONASS);
			my_gnss.enableGNSS(true, SFE_UBLOX_GNSS_ID_SBAS);
			my_gnss.enableGNSS(true, SFE_UBLOX_GNSS_ID_BEIDOU);
			my_gnss.enableGNSS(true, SFE_UBLOX_GNSS_ID_IMES);
			my_gnss.enableGNSS(true, SFE_UBLOX_GNSS_ID_QZSS);

			if (g_custom_parameters.location_on)
			{
				my_gnss.setNavigationFrequency(5); // Produce two solutions per second
				my_gnss.setAutoPVT(true, false);   // Tell the GNSS to "send" each solution and the lib not to update stale data implicitly
			}
			else
			{
				my_gnss.setMeasurementRate(500);
			}

			my_gnss.saveConfiguration(); // Save the current settings to flash and BBR
		}
		else
		{
			my_gnss.powerOff(0xFFFFFFFF);
			delay(250);
		}

		// Keep GNSS active if forced in setup ==> Leads to faster battery drainage!
		if (g_custom_parameters.test_mode == MODE_FIELDTESTER)
		{
			if (!g_custom_parameters.location_on)
			{
				digitalWrite(WB_IO2, LOW);
			}
		}
		else
		{
			// Power down module
			digitalWrite(WB_IO2, LOW);
		}
	}
	else
	{
		if (!my_gnss.begin())
		{
			MYLOG("GNSS", "Restart UBLOX failed");
			return false;
		}
		MYLOG("GNSS", "Restarted UBLOX");

		my_gnss.setI2COutput(COM_TYPE_UBX); // Set the I2C port to output UBX only (turn off NMEA noise)

		my_gnss.enableGNSS(true, SFE_UBLOX_GNSS_ID_GPS);
		my_gnss.enableGNSS(true, SFE_UBLOX_GNSS_ID_GALILEO);
		my_gnss.enableGNSS(true, SFE_UBLOX_GNSS_ID_GLONASS);
		my_gnss.enableGNSS(true, SFE_UBLOX_GNSS_ID_SBAS);
		my_gnss.enableGNSS(true, SFE_UBLOX_GNSS_ID_BEIDOU);
		my_gnss.enableGNSS(true, SFE_UBLOX_GNSS_ID_IMES);
		my_gnss.enableGNSS(true, SFE_UBLOX_GNSS_ID_QZSS);

		if (g_custom_parameters.location_on)
		{
			my_gnss.setNavigationFrequency(5); // Produce two solutions per second
			my_gnss.setAutoPVT(true, false);   // Tell the GNSS to "send" each solution and the lib not to update stale data implicitly
		}
		else
		{
			my_gnss.setMeasurementRate(500);
		}
		my_gnss.saveConfiguration(); // Save the current settings to flash and BBR
	}

	return true;
}

/**
 * @brief Check GNSS module for position
 *
 * @return true Valid position found
 * @return false No valid position
 */
bool poll_gnss(void)
{
	last_read_ok = false;

	latitude = 0;
	longitude = 0;
	altitude = 0;
	accuracy = 0;
	satellites = 0;

	bool has_pos = false;
	bool has_alt = false;

	char fix_type_str[32] = {0};
	sprintf(fix_type_str, "No Fix");
	byte fix_type;

	if (g_custom_parameters.location_on)
	{
		// GNSS is active all time, just check HDOP and number of satellites
		latitude = my_gnss.getLatitude();
		longitude = my_gnss.getLongitude();
		altitude = my_gnss.getAltitude();
		accuracy = my_gnss.getHorizontalDOP();
		satellites = my_gnss.getSIV();
		fix_type = my_gnss.getFixType(); // Get the fix type
		if (fix_type == 1)
			sprintf(fix_type_str, "Dead reckoning");
		else if (fix_type == 2)
			sprintf(fix_type_str, "Fix type 2D");
		else if (fix_type == 3)
			sprintf(fix_type_str, "Fix type 3D");
		else if (fix_type == 4)
			sprintf(fix_type_str, "GNSS fix");
		else if (fix_type == 5)
			sprintf(fix_type_str, "Time fix");
		else
		{
			sprintf(fix_type_str, "No Fix");
			fix_type = 0;
		}

		MYLOG("GNSS", "Sat: %d Fix: %s", satellites, fix_type_str);
		MYLOG("GNSS", "Lat: %.4f Lon: %.4f", latitude / 10000000.0, longitude / 10000000.0);
		MYLOG("GNSS", "Alt: %.2f", altitude / 1000.0);
		MYLOG("GNSS", "HDOP: %.2f ", accuracy / 100.0);

		if ((accuracy < 300) && (satellites > 5))
		{
			last_read_ok = true;
		}
	}
	else
	{
		if (my_gnss.getGnssFixOk())
		{
			digitalWrite(LED_BLUE, HIGH);
			fix_type = my_gnss.getFixType(); // Get the fix type
			if (fix_type == 0)
				sprintf(fix_type_str, "No Fix");
			else if (fix_type == 1)
				sprintf(fix_type_str, "Dead reck");
			else if (fix_type == 2)
				sprintf(fix_type_str, "Fix 2D");
			else if (fix_type == 3)
				sprintf(fix_type_str, "Fix 3D");
			else if (fix_type == 4)
				sprintf(fix_type_str, "GNSS fix");
			else if (fix_type == 5)
				sprintf(fix_type_str, "Time fix");

			satellites = my_gnss.getSIV();

			bool satisfied = false;

			// When in cold start, wait for max satellites
			if (satellites == max_sat)
			{
				max_sat_unchanged++;
			}
			if (satellites > max_sat)
			{
				max_sat = satellites;
			}
			if ((fix_type >= 3) && (max_sat_unchanged >= 2)) /** Fix type 3D and number of satellites not growing */
			{
				satisfied = true;
			}

			if (satisfied)
			// if ((fix_type >= 3) && (max_sat_unchanged >= 4)) /** Fix type 3D and number of satellites not growing */
			// if ((fix_type >= 3) && (satellites >= 8)) /** Fix type 3D and at least 8 satellites */
			// if (fix_type >= 3) /** Fix type 3D */
			{
				last_read_ok = true;
				latitude = my_gnss.getLatitude();
				longitude = my_gnss.getLongitude();
				altitude = my_gnss.getAltitude();
				accuracy = my_gnss.getHorizontalDOP();

				// MYLOG("GNSS", "Fixtype: %d %s", my_gnss.getFixType(), fix_type_str);
				// MYLOG("GNSS", "Lat: %.4f Lon: %.4f", latitude / 10000000.0, longitude / 10000000.0);
				// MYLOG("GNSS", "Alt: %.2f", altitude / 1000.0);
				// MYLOG("GNSS", "Acy: %.2f ", accuracy / 100.0);
			}
		}
	}

	if (last_read_ok)
	{
		if ((latitude == 0) && (longitude == 0))
		{
			last_read_ok = false;
			return false;
		}

		g_solution_data.addGNSS_T(latitude, longitude, altitude / 1000, accuracy, satellites);

		g_last_lat = latitude / 10000000.0;
		g_last_long = longitude / 10000000.0;
		g_last_accuracy = accuracy;
		g_last_altitude = altitude / 1000;
		g_last_satellites = satellites;

		return true;
	}
	else
	{
		// No location found
#if FAKE_GPS > 0
		MYLOG("GNSS", "Faking GPS");
		// 14.4213730, 121.0069140, 35.000
		latitude = 144213730;
		longitude = 1210069140;
		altitude = 35000;
		accuracy = 1;
		satellites = 5;

		g_solution_data.addGNSS_T(latitude, longitude, altitude, accuracy, satellites);

		last_read_ok = true;

		g_last_lat = latitude / 10000000.0;
		g_last_long = longitude / 10000000.0;
		g_last_accuracy = accuracy;
		g_last_altitude = altitude / 1000;
		g_last_satellites = satellites;

		return true;
#else
		g_last_lat = 0;
		g_last_long = 0;
		g_last_accuracy = 1;
		g_last_altitude = 0;
		g_last_satellites = 0;
#endif
	}

	// MYLOG("GNSS", "No valid location found");
	last_read_ok = false;
	return false;
}

/**
 * @brief GNSS location aqcuisition
 * Called every 2.5 seconds by timer 1
 * Gives up after 1/2 of send frequency
 * or when location was aquired
 *
 */
void gnss_handler(void *)
{
	digitalWrite(LED_GREEN, HIGH);
	bool finished_poll = false;
	if (poll_gnss())
	{
		// Keep GNSS active if forced in setup ==> Leads to faster battery drainage!
		if (!g_custom_parameters.location_on)
		{
			// Power down the module
			digitalWrite(WB_IO2, LOW);
		}
		gnss_active = false;
		delay(100);
		MYLOG("GNSS", "Got location");
		api.system.timer.stop(RAK_TIMER_3);
		if (has_oled && !g_settings_ui)
		{
			oled_clear();
			oled_add_line((char *)"Location:");
			sprintf(line_str, "La %.4f Lo %.4f", g_last_lat, g_last_long, g_last_altitude);
			oled_add_line(line_str);
			sprintf(line_str, "HDOP %.2f Sat: %d", g_last_accuracy, g_last_satellites );
			oled_add_line(line_str);
		}
		finished_poll = true;
		// Always send confirmed packet to make sure a reply is received
		if (!api.lorawan.send(g_solution_data.getSize(), g_solution_data.getBuffer(), 1, true, 7))
		{
			tx_active = false;
			MYLOG("APP", "LoRaWAN send returned error");
		}
		else
		{
			tx_active = true;
		}
	}
	else
	{
		if (check_gnss_counter >= check_gnss_max_try)
		{
			// Keep GNSS active until we get a valid location!
			delay(100);
			gnss_active = false;
			tx_active = false;

			MYLOG("GNSS", "Location timeout");
			api.system.timer.stop(RAK_TIMER_3);
			// If no location found, Field Tester does not send data
			if (has_oled && !g_settings_ui)
			{
				sprintf(line_str, "No valid location found");
				oled_add_line(line_str);
			}
			finished_poll = true;
			if (forced_tx)
			{
				forced_tx = false;
				// If forced TX, send whether we have location or not 143050416, 1206306357
				g_solution_data.addGNSS_T(0, 0, 0, 1, 0);
				if (!api.lorawan.send(g_solution_data.getSize(), g_solution_data.getBuffer(), 1, true, 7))
				{
					tx_active = false;
					MYLOG("APP", "LoRaWAN send returned error");
				}
				else
				{
					tx_active = true;
				}
			}
		}
	}
	if (has_oled && !finished_poll && !g_settings_ui)
	{
		oled_clear();
		line_str[0] = 0x00;
		if (g_custom_parameters.location_on)
		{
			oled_add_line((char *)"Warm start acquistion");
		}
		else
		{
			oled_add_line((char *)"Acquistion ongoing");
		}
		if (satellites == 0)
		{
			for (int idx = 0; idx < check_gnss_counter; idx++)
			{
				line_str[idx] = '*';
				line_str[idx + 1] = 0x00;
			}
		}
		else
		{
			sprintf(line_str, "# Sat = %d", satellites);
		}
		oled_add_line(line_str);
		oled_display();
	}
	check_gnss_counter++;
	digitalWrite(LED_GREEN, LOW);
}
