/**
 * @file button.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Button handler
 * @version 0.1
 * @date 2024-06-23
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "app.h"
#include "udrv_dfu.h"

volatile static time_t pressTime = 0;

volatile uint8_t pressCount = 0;

bool g_settings_ui = false;
uint8_t sel_menu = 0;
bool restart_required = false;
custom_param_s g_last_settings;
uint8_t ui_last_dr = 0;
uint8_t ui_max_dr = 0;
uint8_t ui_min_dr = 0;
uint8_t ui_last_band = 0;
uint8_t ui_last_adr = 0;
uint8_t ui_last_tx = 0;
uint8_t ui_max_tx = 0;
uint8_t ui_min_tx = 0;
uint32_t ui_p2p_freq = 0;
uint8_t ui_p2p_sf = 0;
uint8_t ui_p2p_bw = 0;
uint8_t ui_p2p_cr = 0;
uint8_t ui_p2p_tx = 0;

char *top_menu[] = {"Back", "Info", "Device Settings", "Mode", "LoRa Setting"};
uint8_t top_menu_len = 5;

char *back_menu[] = {"Back"};
uint8_t back_menu_len = 1;

char *settings_menu[] = {"Back", "Interval", "Location", "Display Saver"};
uint8_t settings_menu_len = 4;

char *mode_menu[] = {"Back", "LPW LinkCheck", "LPW Confirmed", "P2P", "Field Tester"};
uint8_t mode_menu_len = 5;

char *settings_p2p_menu[] = {"Back", "Freq", "SF", "BW", "CR", "TX"};
uint8_t settings_p2p_menu_len = 6;

char *settings_lpw_menu[] = {"Back", "ADR", "DR", "TX", "Region"};
uint8_t settings_lpw_menu_len = 5;

// char *p_sf_menu[] = {"5", "6", "7", "8", "9", "10", "11", "12"};
// uint8_t p_sf_menu_len = 8;

char *p_bw_menu[] = {"500", "250", "125", "62.5", "41.67", "31.25", "20.83", "15.63", "10.4", "7.8"};
uint8_t p_bw_menu_len = 10;

bool buttonInit(void)
{
	pinMode(BUTTON_INT_PIN, INPUT_PULLUP);
	attachInterrupt(BUTTON_INT_PIN, buttonIntHandle, FALLING);

	mtmMain.Register(handle_button, 100); // Process button data every 100ms.

	return true;
}

static time_t firstPressTime = 0;

void buttonIntHandle(void)
{
	pressTime = millis();
	if ((millis() - firstPressTime) > 200) // for button debounce.
	{
		firstPressTime = millis();
		pressCount += 1;
	}
	MYLOG("BTN", "pressCount = %d", pressCount);
}

uint8_t getButtonStatus(void)
{
	uint8_t lowCount = 0;
	if (((millis() - pressTime) > 4000) && (digitalRead(BUTTON_INT_PIN) == LOW) && (pressCount >= 1))
	{
		// pressCount = 0;
		// pressTime = 0;

		return LONG_PRESS;
	}

	if (((millis() - pressTime) >= 400) && (pressCount == 1) && (digitalRead(BUTTON_INT_PIN) == HIGH))
	{
		uint8_t highCount = 0;
		for (uint8_t i; i < 200; i++)
		{
			if (digitalRead(BUTTON_INT_PIN) == HIGH)
				highCount++;
			delay(1);
		}
		if (highCount > 198) // Have no option but to.
		{
			// pressCount = 0;
			// pressTime = 0;
			return SINGLE_CLICK;
		}
	}

	if (((millis() - pressTime) >= 400) && (pressCount == 2) && (digitalRead(BUTTON_INT_PIN) == HIGH))
	{
		uint8_t highCount = 0;
		for (uint8_t i; i < 200; i++)
		{
			if (digitalRead(BUTTON_INT_PIN) == HIGH)
				highCount++;
			delay(1);
		}
		if (highCount > 198) // Have no option but to.
		{
			// pressCount = 0;
			// pressTime = 0;
			return DOUBLE_CLICK;
		}
	}

	if (((millis() - pressTime) >= 600) && (pressCount == 3))
	{
		uint8_t highCount = 0;
		for (uint8_t i; i < 200; i++)
		{
			if (digitalRead(BUTTON_INT_PIN) == HIGH)
				highCount++;
			delay(1);
		}
		if (highCount > 198) // Have no option but to.
		{
			// pressCount = 0;
			// pressTime = 0;
			return TRIPPLE_CLICK;
		}
	}

	if (((millis() - pressTime) >= 800) && (pressCount == 4))
	{
		uint8_t highCount = 0;
		for (uint8_t i; i < 200; i++)
		{
			if (digitalRead(BUTTON_INT_PIN) == HIGH)
				highCount++;
			delay(1);
		}
		if (highCount > 198) // Have no option but to.
		{
			// pressCount = 0;
			// pressTime = 0;
			return QUAD_CLICK;
		}
	}

	if (((millis() - pressTime) >= 1200) && (pressCount == 5))
	{
		uint8_t highCount = 0;
		for (uint8_t i; i < 200; i++)
		{
			if (digitalRead(BUTTON_INT_PIN) == HIGH)
				highCount++;
			delay(1);
		}
		if (highCount > 198) // Have no option but to.
		{
			// pressCount = 0;
			// pressTime = 0;
			return FIVE_CLICK;
		}
	}

	if (((millis() - pressTime) >= 1400) && (pressCount == 6))
	{
		uint8_t highCount = 0;
		for (uint8_t i; i < 200; i++)
		{
			if (digitalRead(BUTTON_INT_PIN) == HIGH)
				highCount++;
			delay(1);
		}
		if (highCount > 198) // Have no option but to.
		{
			// pressCount = 0;
			// pressTime = 0;
			return SIX_CLICK;
		}
	}
	return BUTTONSTATE_NONE;
}

void save_n_reboot(void)
{
	// Test mode changed, force a reboot
	if (g_last_settings.test_mode != g_custom_parameters.test_mode)
	{
		oled_clear();
		oled_write_header("REBOOT");
		g_custom_parameters.send_interval = g_last_settings.send_interval;
		g_custom_parameters.test_mode = g_last_settings.test_mode;
		g_custom_parameters.display_saver = g_last_settings.display_saver;
		g_custom_parameters.location_on = g_last_settings.location_on;
		save_at_setting();
		delay(5000);
		// settings changed, reboot
		api.system.reboot();
	}

	// Other setting changed, save them or apply them
	if ((g_last_settings.send_interval != g_custom_parameters.send_interval) ||
		(g_last_settings.display_saver != g_custom_parameters.display_saver) ||
		(g_last_settings.location_on != g_custom_parameters.location_on))
	{
		g_custom_parameters.send_interval = g_last_settings.send_interval;
		g_custom_parameters.display_saver = g_last_settings.display_saver;
		g_custom_parameters.location_on = g_last_settings.location_on;
		save_at_setting();
	}
	if (api.lorawan.nwm.get())
	{
		// LoRaWAN settings changed
		if (ui_last_adr != api.lorawan.adr.get())
		{
			api.lorawan.adr.set(ui_last_adr);
		}
		if (ui_last_dr != api.lorawan.dr.get())
		{
			api.lorawan.dr.set(ui_last_dr);
		}
		if (ui_last_tx != api.lorawan.txp.get())
		{
			api.lorawan.txp.set(ui_last_tx);
		}
		if (ui_last_band != api.lorawan.band.get())
		{
			api.lorawan.band.set(ui_last_band);
			api.lorawan.join(1, 1, 10, 50);
		}
	}
	else
	{
		// LoRa P2P settings changed
		api.lora.pfreq.set(ui_p2p_freq);
		if (ui_p2p_freq != api.lora.pfreq.get())
		{
			api.lora.pfreq.set(ui_p2p_freq);
		}
		if (ui_p2p_sf != api.lora.psf.get())
		{
			api.lora.psf.set(ui_p2p_sf);
		}
		if (ui_p2p_bw != api.lora.pbw.get())
		{
			api.lora.pbw.set(ui_p2p_bw);
		}
		if (ui_p2p_cr != api.lora.pcr.get())
		{
			api.lora.pcr.set(ui_p2p_cr);
		}
		if (ui_p2p_tx != api.lora.ptp.get())
		{
			api.lora.ptp.set(ui_p2p_tx);
		}
		// enable receive again
		api.lora.precv(65533);
	}
	oled_clear();

	switch (g_custom_parameters.test_mode)
	{
	case MODE_LINKCHECK:
		oled_add_line((char *)"LinkCheck mode");
		break;
	case MODE_CFM:
		oled_add_line((char *)"Confirmed Pckg mode");
		break;
	case MODE_P2P:
		oled_add_line((char *)"P2P mode");
		break;
	case MODE_FIELDTESTER:
		oled_add_line((char *)"Field Tester mode");
		break;
	}
	sprintf(line_str, "Test interval %lds", g_custom_parameters.send_interval / 1000);
	oled_add_line(line_str);

	/** \todo implement location on/off */
	if (g_last_settings.location_on)
	{
	}
	else
	{
	}

	if (g_custom_parameters.send_interval != 0)
	{
		api.system.timer.start(RAK_TIMER_0, g_custom_parameters.send_interval, NULL);
	}

	if (g_custom_parameters.display_saver)
	{
		api.system.timer.start(RAK_TIMER_2, 60000, NULL);
	}
}

void handle_button(void)
{
	uint8_t selected_item = 0;
	// char line_str[32];

	switch (getButtonStatus())
	{
	case LONG_PRESS:
	{
		pressCount = 0;
		pressTime = 0;
		MYLOG("BTN", "LongPress");
		if (display_power)
		{
			oled_power(false);
		}
		else
		{
			oled_power(true);
		}

		break;
	}
	case SIX_CLICK:
	{
		pressCount = 0;
		pressTime = 0;
		// MYLOG("BTN", "Six Clicks");
		if (g_settings_ui)
		{
			switch (sel_menu)
			{
			case T_LORAP2P_MENU:
				sel_menu = S_P2P_TX;
				selected_item = ui_p2p_tx;
				display_show_menu(back_menu, back_menu_len, sel_menu, selected_item, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			}
			MYLOG("BTN", "6x Menu Level %d", sel_menu);
		}
		break;
	}
	case FIVE_CLICK:
	{
		pressCount = 0;
		pressTime = 0;
		// MYLOG("BTN", "Fice Clicks");
		if (g_settings_ui)
		{
			switch (sel_menu)
			{
			case T_TOP_MENU:
				if (api.lorawan.nwm.get() == 1)
				{
					// LoRaWAN settings
					sel_menu = T_LORAWAN_MENU;
					display_show_menu(settings_lpw_menu, settings_lpw_menu_len, sel_menu, 255, g_last_settings.display_saver, g_last_settings.location_on);
				}
				else
				{
					// LoRaWAN settings
					sel_menu = T_LORAP2P_MENU;
					display_show_menu(settings_p2p_menu, settings_p2p_menu_len, sel_menu, 255, g_last_settings.display_saver, g_last_settings.location_on);
				}
				break;
			case T_LORAWAN_MENU:
				sel_menu = S_LPW_BAND;
				selected_item = api.lorawan.band.get();
				display_show_menu(back_menu, back_menu_len, sel_menu, selected_item, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			case T_LORAP2P_MENU:
				sel_menu = S_P2P_CR;
				selected_item = ui_p2p_cr;
				display_show_menu(back_menu, back_menu_len, sel_menu, selected_item, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			case T_MODE_MENU:
				g_last_settings.test_mode = MODE_FIELDTESTER;
				selected_item = g_last_settings.test_mode + 2;
				display_show_menu(mode_menu, mode_menu_len, sel_menu, selected_item, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			}
			MYLOG("BTN", "5x Menu Level %d", sel_menu);
		}
		else
		{
			oled_clear();
			oled_write_header((char *)"BOOTLOADER");
			udrv_enter_dfu();
		}
		break;
	}
	case QUAD_CLICK:
	{
		pressCount = 0;
		pressTime = 0;
		// MYLOG("BTN", "Four Clicks");
		if (g_settings_ui)
		{
			switch (sel_menu)
			{
			case T_TOP_MENU:
				sel_menu = T_MODE_MENU;
				selected_item = g_last_settings.test_mode + 2;
				display_show_menu(mode_menu, mode_menu_len, sel_menu, selected_item, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			case T_INFO_MENU: // NA
				sel_menu = T_TOP_MENU;
				display_show_menu(top_menu, top_menu_len, sel_menu, 255, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			case T_SETT_MENU:
				sel_menu = T_SETT_MENU;
				// Toggle screen saver, not saved
				g_last_settings.display_saver = !g_last_settings.display_saver;
				display_show_menu(settings_menu, settings_menu_len, sel_menu, 255, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			case T_MODE_MENU:
				g_last_settings.test_mode = MODE_P2P;
				selected_item = g_last_settings.test_mode + 2;
				display_show_menu(mode_menu, mode_menu_len, sel_menu, selected_item, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			case T_LORAWAN_MENU:
			{
				sel_menu = S_LPW_TX;
				uint8_t current_region = api.lorawan.band.get();
				switch (current_region)
				{
				case 0: // EU433
					ui_min_tx = 0;
					ui_max_tx = 5;
					break;
				case 1:	 // CN470
				case 2:	 // RU864
				case 4:	 // EU868
				case 7:	 // KR920
				case 8:	 // AS923-1
				case 9:	 // AS923-2
				case 10: // AS923-3
				case 11: // AS923-4
					ui_min_tx = 0;
					ui_max_tx = 7;
					break;
				case 3:	 // IN865
				case 5:	 // US915
				case 6:	 // AU915
				case 12: // LA915
					ui_min_tx = 0;
					ui_max_tx = 10;
					break;
				}
				selected_item = ui_last_dr + 10;
				display_show_menu(back_menu, back_menu_len, sel_menu, selected_item, g_last_settings.display_saver, g_last_settings.location_on);
			}
			break;
			case T_LORAP2P_MENU:
				sel_menu = S_P2P_BW;
				selected_item = ui_p2p_bw;
				display_show_menu(back_menu, back_menu_len, sel_menu, selected_item, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			case S_SEND_INT:
				sel_menu = S_SEND_INT;
				display_show_menu(back_menu, back_menu_len, sel_menu, selected_item, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			}
			MYLOG("BTN", "4x Menu Level %d", sel_menu);
		}
		else
		{
			oled_clear();
			oled_write_header((char *)"RESET");
			api.system.reboot();
		}
		break;
	}
	case TRIPPLE_CLICK:
	{
		pressCount = 0;
		pressTime = 0;
		// MYLOG("BTN", "Tripple Click");
		if (g_settings_ui)
		{
			switch (sel_menu)
			{
			case T_TOP_MENU:
				sel_menu = T_SETT_MENU;
				display_show_menu(settings_menu, settings_menu_len, sel_menu, 255, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			case T_INFO_MENU: // NA
				sel_menu = T_TOP_MENU;
				display_show_menu(top_menu, top_menu_len, sel_menu, 255, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			case T_SETT_MENU:
				sel_menu = T_SETT_MENU;
				// Toggle location, not saved
				g_last_settings.location_on = !g_last_settings.location_on;
				display_show_menu(settings_menu, settings_menu_len, sel_menu, 255, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			case T_MODE_MENU:
				g_last_settings.test_mode = MODE_CFM;
				selected_item = g_last_settings.test_mode + 2;
				display_show_menu(mode_menu, mode_menu_len, sel_menu, selected_item, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			case T_LORAWAN_MENU:
			{
				sel_menu = S_LPW_DR;
				uint8_t current_region = api.lorawan.band.get();
				switch (current_region)
				{
				case 0: // EU433
				case 1: // CN470
				case 2: // RU864
				case 3: // IN865
				case 4: // EU868
				case 7: // KR920
					ui_min_dr = 0;
					ui_max_dr = 5;
					break;
				case 5: // US915
					ui_min_dr = 0;
					ui_max_dr = 4;
					break;
				case 6:	 // AU915
				case 12: // LA915
					ui_min_dr = 0;
					ui_max_dr = 6;
					break;
				case 8:	 // AS923-1
				case 9:	 // AS923-2
				case 10: // AS923-3
				case 11: // AS923-4
					ui_min_dr = 2;
					ui_max_dr = 5;
					break;
				}
				selected_item = ui_last_dr + 10;
				display_show_menu(back_menu, back_menu_len, sel_menu, selected_item, g_last_settings.display_saver, g_last_settings.location_on);
			}
			break;
			case T_LORAP2P_MENU:
			{
				sel_menu = S_P2P_SF;
				display_show_menu(back_menu, back_menu_len, sel_menu, ui_p2p_sf, g_last_settings.display_saver, g_last_settings.location_on);
			}
			break;
			case S_SEND_INT:
				sel_menu = S_SEND_INT;
				if (g_last_settings.send_interval != 0)
				{
					if (g_last_settings.send_interval < 15000)
					{
						g_last_settings.send_interval = 0;
					}
					else
					{
						g_last_settings.send_interval -= 15000;
					}
				}
				display_show_menu(back_menu, back_menu_len, sel_menu, 255, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			case S_LPW_BAND:
				sel_menu = S_LPW_BAND;
				if (ui_last_band == 0)
				{
					ui_last_band = 12;
				}
				else
				{
					ui_last_band -= 1;
				}
				MYLOG("BTN", "Region %d", ui_last_band);
				display_show_menu(back_menu, back_menu_len, sel_menu, ui_last_band, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			case S_LPW_DR:
				sel_menu = S_LPW_DR;
				if (ui_last_dr == ui_min_dr)
				{
					ui_last_dr = ui_max_dr;
				}
				else
				{
					ui_last_dr -= 1;
				}
				display_show_menu(back_menu, back_menu_len, sel_menu, ui_last_dr, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			case S_LPW_TX:
				sel_menu = S_LPW_TX;
				if (ui_last_tx == ui_min_tx)
				{
					ui_last_tx = ui_max_tx;
				}
				else
				{
					ui_last_tx -= 1;
				}
				display_show_menu(back_menu, back_menu_len, sel_menu, ui_last_dr, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			case S_P2P_FREQ:
				sel_menu = S_P2P_FREQ;
				ui_p2p_freq = ui_p2p_freq - 100000;
				if (ui_p2p_freq < 430000000)
				{
					ui_p2p_freq = 430000000;
				}
				display_show_menu(back_menu, back_menu_len, sel_menu, 255, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			case S_P2P_SF:
				sel_menu = S_P2P_SF;
				if (ui_p2p_sf == 6)
				{
					ui_p2p_sf = 12;
				}
				else
				{
					ui_p2p_sf = ui_p2p_sf - 1;
				}
				display_show_menu(back_menu, back_menu_len, sel_menu, ui_p2p_sf, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			case S_P2P_BW:
				sel_menu = S_P2P_BW;
				if (ui_p2p_bw == 0)
				{
					ui_p2p_bw = 9;
				}
				else
				{
					ui_p2p_bw = ui_p2p_bw - 1;
				}
				display_show_menu(back_menu, back_menu_len, sel_menu, ui_p2p_bw, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			case S_P2P_CR:
				sel_menu = S_P2P_CR;
				if (ui_p2p_cr == 0)
				{
					ui_p2p_cr = 3;
				}
				else
				{
					ui_p2p_cr = ui_p2p_cr - 1;
				}
				display_show_menu(back_menu, back_menu_len, sel_menu, ui_p2p_cr, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			case S_P2P_TX:
				sel_menu = S_P2P_TX;
				 if (ui_p2p_tx == 5)
				{
					ui_p2p_tx = 22;
				}
				else
				{
					ui_p2p_tx = ui_p2p_tx - 1;
				}
				display_show_menu(back_menu, back_menu_len, sel_menu, ui_p2p_tx, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			}
			MYLOG("BTN", "3x Menu Level %d", sel_menu);
		}
		else
		{
			MYLOG("BTN", "Force sending");
			if (g_custom_parameters.test_mode == MODE_FIELDTESTER)
			{
				if (!gnss_active)
				{
					// Stop interval sending
					if (g_custom_parameters.send_interval != 0)
					{
						api.system.timer.stop(RAK_TIMER_0);
						api.system.timer.start(RAK_TIMER_0, g_custom_parameters.send_interval, NULL);
					}
					send_packet(NULL);
				}
			}
			else
			{
				// Stop interval sending
				if (g_custom_parameters.send_interval != 0)
				{
					api.system.timer.stop(RAK_TIMER_0);
					api.system.timer.start(RAK_TIMER_0, g_custom_parameters.send_interval, NULL);
				}
				send_packet(NULL);
			}
		}
		break;
	}
	case DOUBLE_CLICK:
	{
		pressCount = 0;
		pressTime = 0;
		// MYLOG("BTN", "Double Click");
		if (!g_settings_ui)
		{
			api.system.timer.stop(RAK_TIMER_0);
			api.system.timer.stop(RAK_TIMER_1);
			api.system.timer.stop(RAK_TIMER_2);

			if (!display_power)
			{
				oled_power(true);
			}

			g_settings_ui = true;
			sel_menu = T_TOP_MENU;
			g_last_settings.send_interval = g_custom_parameters.send_interval;
			g_last_settings.test_mode = g_custom_parameters.test_mode;
			g_last_settings.display_saver = g_custom_parameters.display_saver;
			g_last_settings.location_on = g_custom_parameters.location_on;
			if (api.lorawan.nwm.get())
			{
				MYLOG("BTN", "Get LoRaWAN settings");
				ui_last_dr = api.lorawan.dr.get();
				ui_last_band = api.lorawan.band.get();
				ui_last_adr = api.lorawan.adr.get();
				ui_last_tx = api.lorawan.txp.get();
			}
			else
			{
				MYLOG("BTN", "Get LoRaWAN settings");
				api.lora.precv(0);
				ui_p2p_freq = api.lora.pfreq.get();
				ui_p2p_sf = api.lora.psf.get();
				ui_p2p_bw = api.lora.pbw.get();
				ui_p2p_cr = api.lora.pcr.get();
				ui_p2p_tx = api.lora.ptp.get();
				MYLOG("BTN", "F %.3f F %.3f", (long)ui_p2p_freq / 100000000.0, (long)api.lora.pfreq.get() / 100000000.0);
			}
			display_show_menu(top_menu, top_menu_len, sel_menu, 0);
			MYLOG("BTN", "2x Menu Level %d", sel_menu);
		}
		else
		{
			switch (sel_menu)
			{
			case T_TOP_MENU:
				sel_menu = T_INFO_MENU;
				display_show_menu(back_menu, back_menu_len, sel_menu, 255, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			case T_INFO_MENU: // NA
				sel_menu = T_TOP_MENU;
				display_show_menu(top_menu, top_menu_len, sel_menu, 255, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			case T_SETT_MENU:
				sel_menu = S_SEND_INT;
				display_show_menu(&back_menu[0], back_menu_len, sel_menu, 255, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			case T_MODE_MENU:
				g_last_settings.test_mode = MODE_LINKCHECK;
				selected_item = g_last_settings.test_mode + 2;
				display_show_menu(mode_menu, mode_menu_len, sel_menu, selected_item, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			case T_LORAWAN_MENU:
				sel_menu = S_LPW_ADR;
				selected_item = ui_last_adr + 10;
				display_show_menu(back_menu, back_menu_len, sel_menu, selected_item, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			case T_LORAP2P_MENU:
				sel_menu = S_P2P_FREQ;
				display_show_menu(back_menu, back_menu_len, sel_menu, 255, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			case S_LPW_BAND:
				sel_menu = S_LPW_BAND;
				if (ui_last_band == 12)
				{
					ui_last_band = 0;
				}
				else
				{
					ui_last_band += 1;
				}
				MYLOG("BTN", "Region %d", ui_last_band);
				display_show_menu(back_menu, back_menu_len, sel_menu, ui_last_band, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			case S_LPW_ADR:
				sel_menu = S_LPW_ADR;
				if (ui_last_adr == 0)
				{
					ui_last_adr = 1;
				}
				else
				{
					ui_last_adr = 0;
				}
				selected_item = ui_last_adr + 10;
				display_show_menu(back_menu, back_menu_len, sel_menu, selected_item, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			case S_LPW_DR:
				sel_menu = S_LPW_DR;
				if (ui_last_dr == ui_max_dr)
				{
					ui_last_dr = ui_min_dr;
				}
				else
				{
					ui_last_dr += 1;
				}
				display_show_menu(back_menu, back_menu_len, sel_menu, ui_last_dr, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			case S_LPW_TX:
				sel_menu = S_LPW_TX;
				if (ui_last_tx == ui_max_tx)
				{
					ui_last_tx = ui_min_tx;
				}
				else
				{
					ui_last_tx += 1;
				}
				display_show_menu(back_menu, back_menu_len, sel_menu, ui_last_tx, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			case S_P2P_FREQ:
				sel_menu = S_P2P_FREQ;
				ui_p2p_freq = ui_p2p_freq + 100000;
				if (ui_p2p_freq > 960000000)
				{
					ui_p2p_freq = 960000000;
				}
				display_show_menu(back_menu, back_menu_len, sel_menu, 255, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			case S_P2P_SF:
				sel_menu = S_P2P_SF;
				if (ui_p2p_sf == 12)
				{
					ui_p2p_sf = 6;
				}
				else
				{
					ui_p2p_sf = ui_p2p_sf + 1;
				}
				display_show_menu(back_menu, back_menu_len, sel_menu, ui_p2p_sf, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			case S_P2P_BW:
				sel_menu = S_P2P_BW;
				if (ui_p2p_bw == 9)
				{
					ui_p2p_bw = 0;
				}
				else
				{
					ui_p2p_bw = ui_p2p_bw + 1;
				}
				display_show_menu(back_menu, back_menu_len, sel_menu, ui_p2p_bw, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			case S_P2P_CR:
				sel_menu = S_P2P_CR;
				if (ui_p2p_cr == 3)
				{
					ui_p2p_cr = 0;
				}
				else
				{
					ui_p2p_cr = ui_p2p_cr + 1;
				}
				display_show_menu(back_menu, back_menu_len, sel_menu, ui_p2p_cr, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			case S_P2P_TX:
				sel_menu = S_P2P_TX;
				if (ui_p2p_tx == 22)
				{
					ui_p2p_tx = 5;
				}
				else
				{
					ui_p2p_tx = ui_p2p_tx + 1;
				}
				display_show_menu(back_menu, back_menu_len, sel_menu, ui_p2p_tx, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			case S_SEND_INT:
				sel_menu = S_SEND_INT;
				g_last_settings.send_interval += 15000;
				display_show_menu(back_menu, back_menu_len, sel_menu, selected_item, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			}
			MYLOG("BTN", "2x Menu Level %d", sel_menu);
		}
		break;
	}
	case SINGLE_CLICK:
	{
		pressCount = 0;
		pressTime = 0;
		// MYLOG("BTN", "Single Click");
		if (g_settings_ui)
		{
			switch (sel_menu)
			{
			case T_TOP_MENU:
				// Exit menu
				g_settings_ui = false;
				save_n_reboot();
				break;
			case T_INFO_MENU:
			case T_SETT_MENU:
			case T_MODE_MENU:
			case T_LORAWAN_MENU:
			case T_LORAP2P_MENU:
				sel_menu = T_TOP_MENU;
				display_show_menu(top_menu, top_menu_len, sel_menu, 255, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			case S_LPW_BAND:
			case S_LPW_ADR:
			case S_LPW_DR:
			case S_LPW_TX:
				sel_menu = T_LORAWAN_MENU;
				display_show_menu(settings_lpw_menu, settings_lpw_menu_len, sel_menu, 255, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			case S_P2P_FREQ:
			case S_P2P_SF:
			case S_P2P_BW:
			case S_P2P_CR:
			case S_P2P_TX:
				sel_menu = T_LORAP2P_MENU;
				display_show_menu(settings_p2p_menu, settings_p2p_menu_len, sel_menu, 255, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			case S_SEND_INT:
				sel_menu = T_SETT_MENU;
				display_show_menu(settings_menu, settings_menu_len, sel_menu, 255, g_last_settings.display_saver, g_last_settings.location_on);
				break;
			}
			MYLOG("BTN", "1x Menu Level %d", sel_menu);
		}
		break;
	}
	case BUTTONSTATE_NONE:
	default:
		break;
	}
}