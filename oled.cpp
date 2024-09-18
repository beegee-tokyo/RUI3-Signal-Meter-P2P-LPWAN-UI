/**
 * @file RAK1921_oled.cpp
 * @author Bernd Giesecke (bernd.giesecke@rakwireless.com)
 * @brief Initialization and usage of RAK1921 OLED
 * @version 0.1
 * @date 2022-02-18
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "app.h"
#include <nRF_SSD1306Wire.h>

void oled_show(void);

/** Width of the display in pixel */
#define OLED_WIDTH 128
/** Height of the display in pixel */
#define OLED_HEIGHT 64
/** Height of the status bar in pixel */
#define STATUS_BAR_HEIGHT 11
/** Height of a single line */
#define LINE_HEIGHT 10

/** Number of message lines */
#define NUM_OF_LINES (OLED_HEIGHT - STATUS_BAR_HEIGHT) / LINE_HEIGHT

/** Line buffer for messages */
char disp_buffer[NUM_OF_LINES + 1][32] = {0};

/** Current line used */
uint8_t current_line = 0;

/** Display class using Wire */
SSD1306Wire display(0x3c, PIN_WIRE_SDA, PIN_WIRE_SCL, GEOMETRY_128_64, &Wire);

/** Flag if display is on or off */
volatile bool display_power = true;

// Forward declarations for UI
extern uint8_t ui_last_dr;
extern uint8_t ui_max_dr;
extern uint8_t ui_min_dr;
extern uint8_t ui_last_band;
extern uint8_t ui_last_adr;
extern uint8_t ui_last_tx;
extern uint8_t ui_max_tx;
extern uint8_t ui_min_tx;
extern uint32_t ui_p2p_freq;
extern uint8_t ui_p2p_sf;
extern uint8_t ui_p2p_bw;
extern uint8_t ui_p2p_cr;
extern uint8_t ui_p2p_tx;

/**
 * @brief Initialize the display
 *
 * @return true always
 * @return false never
 */
bool init_oled(void)
{
	Wire.begin();

	delay(500); // Give display reset some time
	display.setI2cAutoInit(true);
	display.init();
	display.displayOff();
	display.clear();
	display.displayOn();
	display.setBrightness(200);
#ifdef _RAK19026_
	display.flipScreenVertically();
#endif
	display.setContrast(100, 241, 64);
	display.setFont(ArialMT_Plain_10);
	display.display();

	return true;
}

/**
 * @brief Write the top line of the display
 */
void oled_write_header(char *header_line)
{
	display.setFont(ArialMT_Plain_10);

	// clear the status bar
	display.setColor(BLACK);
	display.fillRect(0, 0, OLED_WIDTH, STATUS_BAR_HEIGHT);

	display.setColor(WHITE);
	display.setTextAlignment(TEXT_ALIGN_LEFT);

	display.drawString(0, 0, header_line);

	uint16_t len = 0;
	char oled_line[64];
#ifdef _VARIANT_RAK4630_
	uint8_t usbStatus = NRF_POWER->USBREGSTATUS;
	switch (usbStatus)
	{
	case 0:
	{
		float bat = api.system.bat.get();
		bat = 0.0;
		for (int idx = 0; idx < 10; idx++)
		{
			bat += api.system.bat.get();
		}
		bat = bat / 10.0;

		len = sprintf(oled_line, "%.2fV", bat);
		display.drawString(127 - (display.getStringWidth(oled_line, len)), 0, oled_line);
		break;
	}
	case 3: // VBUS voltage above valid threshold and USBREG output settling time elapsed (same information as USBPWRRDY event)
	{
		len = sprintf(oled_line, "%s", "USB");
		display.drawString(127 - (display.getStringWidth(oled_line, len)), 0, oled_line);
		break;
	}
	default:
		break;
	}
#else
	float bat = api.system.bat.get();
	bat = 0.0;
	for (int idx = 0; idx < 10; idx++)
	{
		bat += api.system.bat.get();
	}
	bat = bat / 10.0;

	len = sprintf(oled_line, "%.2fV", bat);
	display.drawString(127 - (display.getStringWidth(oled_line, len)), 0, oled_line);
#endif

	// draw divider line
	display.drawLine(0, 11, 127, 11);
	display.display();
}

/**
 * @brief Add a line to the display buffer
 *
 * @param line Pointer to char array with the new line
 */
void oled_add_line(char *line)
{
	if (current_line == NUM_OF_LINES)
	{
		// Display is full, shift text one line up
		for (int idx = 0; idx < NUM_OF_LINES; idx++)
		{
			memcpy(disp_buffer[idx], disp_buffer[idx + 1], 32);
		}
		current_line--;
	}
	snprintf(disp_buffer[current_line], 32, "%s", line);

	if (current_line != NUM_OF_LINES)
	{
		current_line++;
	}

	oled_show();
}

/**
 * @brief Update display messages
 *
 */
void oled_show(void)
{
	display.setColor(BLACK);
	display.fillRect(0, STATUS_BAR_HEIGHT + 1, OLED_WIDTH, OLED_HEIGHT);

	display.setFont(ArialMT_Plain_10);
	display.setColor(WHITE);
	display.setTextAlignment(TEXT_ALIGN_LEFT);
	for (int line = 0; line < current_line; line++)
	{
		display.drawString(0, (line * LINE_HEIGHT) + STATUS_BAR_HEIGHT + 1, disp_buffer[line]);
	}
	display.display();
}

/**
 * @brief Clear the display
 *
 */
void oled_clear(void)
{
	display.setColor(BLACK);
	display.fillRect(0, STATUS_BAR_HEIGHT + 1, OLED_WIDTH, OLED_HEIGHT);
	display.setColor(WHITE);
	current_line = 0;
}

/**
 * @brief Write a line at given position
 *
 * @param line line number
 * @param y_pos x position to start
 * @param text String text
 */
void oled_write_line(int16_t line, int16_t y_pos, String text)
{
	display.drawString(y_pos, (line * LINE_HEIGHT) + STATUS_BAR_HEIGHT + 1, text);
}

/**
 * @brief Display the buffer
 *
 */
void oled_display(void)
{
	display.display();
}

/**
 * @brief Timer callback for display saver
 * 
 */
void oled_saver(void *)
{
	oled_power(false);
}

/**
 * @brief Switch display on/off
 *
 * @param on_off true == Display on, false == Display off
 */
void oled_power(bool on_off)
{
	if (on_off)
	{
		display.displayOn();
		display_power = true;
		// Restart display saver timer if enabled
		if (g_custom_parameters.display_saver)
		{
			api.system.timer.start(RAK_TIMER_2, 60000, NULL);
		}
	}
	else
	{
		display.displayOff();
		display_power = false;
	}
}

/**
 * @brief Settings display handler
 * 
 * @param menu which menu content to show
 * @param menu_len number of menu items
 * @param sel_menu selected menu
 * @param sel_item selected item
 * @param display_saver status display saver
 * @param location_on status location module power saver
 */
void display_show_menu(char *menu[], uint8_t menu_len, uint8_t sel_menu, uint8_t sel_item, bool display_saver, bool location_on)
{
	// char line[64];
	oled_clear();
	uint16_t y_pos = 0;
	uint16_t line_pos = 0;

	// Handle info menu
	if (sel_menu == T_INFO_MENU)
	{
		oled_write_line(0, 0, (char *)"(1) Back");
		switch (g_last_settings.test_mode)
		{
		case MODE_LINKCHECK:
			oled_write_line(1, 0, (char *)"LinkCheck Mode");
			break;
		case MODE_P2P:
			oled_write_line(1, 0, (char *)"P2P Mode");
			break;
		case MODE_FIELDTESTER:
			oled_write_line(1, 0, (char *)"Field Tester mode");
			break;
		}
		sprintf(line_str, "Sent interval %ds", g_last_settings.send_interval / 1000);
		oled_write_line(2, 0, line_str);
		sprintf(line_str, "Location %s", location_on ? "on" : "off");
		oled_write_line(3, 0, line_str);
		sprintf(line_str, "Display saver %s", display_saver ? "on" : "off");
		oled_write_line(4, 0, line_str);
	}
	// Handle send interval menu
	else if (sel_menu == S_SEND_INT)
	{
		oled_write_line(0, 0, (char *)"(1) Back");
		oled_write_line(1, 0, (char *)"(2) 10 seconds more");
		oled_write_line(2, 0, (char *)"(3) 10 seconds less");
		sprintf(line_str, "              ==>  %ld s", g_last_settings.send_interval / 1000);
		oled_write_line(4, 0, line_str);
	}
	// Handle LoRaWAN band selection
	else if (sel_menu == S_LPW_BAND)
	{
		oled_write_line(0, 0, (char *)"(1) Back");
		oled_write_line(1, 0, (char *)"(2) Next");
		oled_write_line(2, 0, (char *)"(3) Prev");
		uint8_t prev_item = sel_item == 0 ? 12 : sel_item - 1;
		uint8_t next_item = sel_item == 12 ? 0 : sel_item + 1;
		sprintf(line_str, "%s", g_regions_list[prev_item]);
		oled_write_line(2, 64, line_str);
		sprintf(line_str, "==>  %s", g_regions_list[sel_item]);
		oled_write_line(3, 64, line_str);
		sprintf(line_str, "%s", g_regions_list[next_item]);
		oled_write_line(4, 64, line_str);
	}
	// Handle LoRaWAN ADR selection
	else if (sel_menu == S_LPW_ADR)
	{
		oled_write_line(0, 0, (char *)"(1) Back");
		if (sel_item == 10)
		{
			sprintf(line_str, "(2) ADR OFF");
		}
		else
		{
			sprintf(line_str, "(2) ADR ON");
		}

		oled_write_line(1, 0, line_str);
	}
	// Handle LoRaWAN DR selection
	else if (sel_menu == S_LPW_DR)
	{
		oled_write_line(0, 0, (char *)"(1) Back");
		oled_write_line(1, 0, (char *)"(2) Next");
		oled_write_line(2, 0, (char *)"(3) Prev");
		uint8_t prev_item = (ui_last_dr == ui_min_dr ? ui_max_dr : ui_last_dr - 1);
		uint8_t next_item = (ui_last_dr == ui_max_dr ? ui_min_dr : ui_last_dr + 1);
		sprintf(line_str, "DR%d", prev_item);
		oled_write_line(2, 64, line_str);
		sprintf(line_str, "==>  DR%d", ui_last_dr);
		oled_write_line(3, 64, line_str);
		sprintf(line_str, "DR%d", next_item);
		oled_write_line(4, 64, line_str);
	}
	// Handle LoRaWAN TX selection
	else if (sel_menu == S_LPW_TX)
	{
		oled_write_line(0, 0, (char *)"(1) Back");
		oled_write_line(1, 0, (char *)"(2) Next");
		oled_write_line(2, 0, (char *)"(3) Prev");
		uint8_t prev_item = (ui_last_tx == ui_min_tx ? ui_max_tx : ui_last_tx - 1);
		uint8_t next_item = (ui_last_tx == ui_max_tx ? ui_min_tx : ui_last_tx + 1);
		sprintf(line_str, "TXP %d", prev_item);
		oled_write_line(2, 64, line_str);
		sprintf(line_str, "==>  TXP %d", ui_last_tx);
		oled_write_line(3, 64, line_str);
		sprintf(line_str, "TXP %d", next_item);
		oled_write_line(4, 64, line_str);
	}
	// Handle P2P frequency menu
	else if (sel_menu == S_P2P_FREQ)
	{
		oled_write_line(0, 0, (char *)"(1) Back");
		oled_write_line(1, 0, (char *)"(2) 0.1MHz up");
		oled_write_line(2, 0, (char *)"(3) 0.1MHz down");
		sprintf(line_str, "              ==>  %.3f MHz", (long)ui_p2p_freq / 1000000.0);
		oled_write_line(4, 0, line_str);
	}
	// Handle P2P SF menu
	else if (sel_menu == S_P2P_SF)
	{
		oled_write_line(0, 0, (char *)"(1) Back");
		oled_write_line(1, 0, (char *)"(2) Next");
		oled_write_line(2, 0, (char *)"(3) Prev");
		uint8_t prev_item = (sel_item == 6 ? 12 : sel_item - 1);
		uint8_t next_item = (sel_item == 12 ? 6 : sel_item + 1);
		sprintf(line_str, "SF %d", prev_item);
		oled_write_line(2, 64, line_str);
		sprintf(line_str, "==>  SF %d", sel_item);
		oled_write_line(3, 64, line_str);
		sprintf(line_str, "SF %d", next_item);
		oled_write_line(4, 64, line_str);
	}
	// Handle P2P BW menu
	else if (sel_menu == S_P2P_BW)
	{
		oled_write_line(0, 0, (char *)"(1) Back");
		oled_write_line(1, 0, (char *)"(2) Next");
		oled_write_line(2, 0, (char *)"(3) Prev");
		uint8_t prev_item = (sel_item == 0 ? 9 : sel_item - 1);
		uint8_t next_item = (sel_item == 9 ? 0 : sel_item + 1);
		sprintf(line_str, "BW %skHz", p_bw_menu[prev_item]);
		oled_write_line(2, 44, line_str);
		sprintf(line_str, "==>  BW %skHz", p_bw_menu[sel_item]);
		oled_write_line(3, 44, line_str);
		sprintf(line_str, "BW %skHz", p_bw_menu[next_item]);
		oled_write_line(4, 44, line_str);
	}
	// Handle P2P SF menu
	else if (sel_menu == S_P2P_CR)
	{
		oled_write_line(0, 0, (char *)"(1) Back");
		oled_write_line(1, 0, (char *)"(2) Next");
		oled_write_line(2, 0, (char *)"(3) Prev");
		uint8_t prev_item = (sel_item == 0 ? 3 : sel_item - 1);
		uint8_t next_item = (sel_item == 3 ? 0 : sel_item + 1);
		sprintf(line_str, "CR 4/%d", prev_item + 5);
		oled_write_line(2, 64, line_str);
		sprintf(line_str, "==>  CR 4/%d", sel_item + 5);
		oled_write_line(3, 64, line_str);
		sprintf(line_str, "CR 4/%d", next_item + 5);
		oled_write_line(4, 64, line_str);
	}
	// Handle P2P TX selection
	else if (sel_menu == S_P2P_TX)
	{
		oled_write_line(0,0,(char *)"(1) Back");
		oled_write_line(1, 0, (char *)"(2) Next");
		oled_write_line(2, 0, (char *)"(3) Prev");
		uint8_t prev_item = (sel_item == 5 ? 22 : sel_item - 1);
		uint8_t next_item = (sel_item == 22 ? 5 : sel_item + 1);
		sprintf(line_str, "TXP %d", prev_item);
		oled_write_line(2, 64, line_str);
		sprintf(line_str, "==>  TXP %d", sel_item);
		oled_write_line(3, 64, line_str);
		sprintf(line_str, "TXP %d", next_item);
		oled_write_line(4, 64, line_str);
	}
	// Handle "standard" menu
	else
	{
		for (int idx = 0; idx < menu_len; idx++)
		{
			if (idx + 1 == sel_item)
			{
				sprintf(line_str, "(X) %s", menu[idx]);
			}
			else
			{
				// Handle settings menu special items
				if ((sel_menu == T_SETT_MENU) && (idx == 2))
				{
					if (location_on)
					{
						sprintf(line_str, "(%d) %s on", idx + 1, menu[idx]);
					}
					else
					{
						sprintf(line_str, "(%d) %s off", idx + 1, menu[idx]);
					}
				}
				else if ((sel_menu == T_SETT_MENU) && (idx == 3))
				{
					if (display_saver)
					{
						sprintf(line_str, "(%d) %s on", idx + 1, menu[idx]);
					}
					else
					{
						sprintf(line_str, "(%d) %s off", idx + 1, menu[idx]);
					}
				}
				else
				{
					sprintf(line_str, "(%d) %s", idx + 1, menu[idx]);
				}
			}

			if (idx > 4)
			{
				y_pos = 64;
				line_pos = idx - 5;
			}
			else
			{
				y_pos = 0;
				line_pos = idx;
			}
			oled_write_line(line_pos, y_pos, line_str);
		}
	}
	oled_display();
}
