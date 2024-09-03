/**
 * @file app.h
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Includes and defines
 * @version 0.1
 * @date 2023-12-29
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef _APP_H_
#define _APP_H_
#include <Arduino.h>

// ATC+PCKG=02685B0367011E05650001237D02CB017400D6306601

/** Set _RAK19026_ to have correct display orientation */
#define _RAK19026_

// Debug
// Debug output set to 0 to disable app debug output
#ifndef MY_DEBUG
#define MY_DEBUG 0
#endif

#if MY_DEBUG > 0
#if defined(_VARIANT_RAK3172_) || defined(_VARIANT_RAK3172_SIP_)
#define MYLOG(tag, ...)                  \
	do                                   \
	{                                    \
		if (tag)                         \
			Serial.printf("[%s] ", tag); \
		Serial.printf(__VA_ARGS__);      \
		Serial.printf("\n");             \
	} while (0);                         \
	delay(100)
#else // RAK4630 || RAK11720
#define MYLOG(tag, ...)                  \
	do                                   \
	{                                    \
		if (tag)                         \
			Serial.printf("[%s] ", tag); \
		Serial.printf(__VA_ARGS__);      \
		Serial.printf("\r\n");           \
		Serial6.printf(__VA_ARGS__);     \
		Serial6.printf("\r\n");          \
	} while (0);                         \
	delay(100)
#endif
#else
#define MYLOG(...)
#endif

// Set firmware version
#define SW_VERSION_0 1
#define SW_VERSION_1 0
#define SW_VERSION_2 1

/** Custom flash parameters structure */
struct custom_param_s
{
	uint8_t valid_flag = 0xAA;
	uint32_t send_interval = 0;
	uint8_t test_mode = 0;
	bool display_saver = true;
	bool location_on = false;
	uint8_t custom_packet[129] = {0x01, 0x02, 0x03, 0x04};
	uint16_t custom_packet_len = 4;
};

typedef enum test_mode_num
{
	MODE_LINKCHECK = 0,
	MODE_CFM = 1,
	MODE_P2P = 2,
	MODE_FIELDTESTER = 3
} test_mode_num_t;

/** Custom flash parameters */
extern custom_param_s g_custom_parameters;

// Forward declarations
bool init_status_at(void);
bool init_interval_at(void);
bool init_test_mode_at(void);
bool init_custom_pckg_at(void);
bool get_at_setting(void);
bool save_at_setting(void);
void set_cfm(void);
void set_linkcheck(void);
void set_p2p(void);
void set_field_tester(void);
void send_packet(void *data);
uint8_t get_min_dr(uint16_t region, uint16_t payload_size);
void get_min_max_dr(uint16_t region, uint8_t *min_dr, uint8_t *max_dr);
extern uint32_t g_send_repeat_time;
extern bool lorawan_mode;
extern bool use_link_check;
extern volatile bool tx_active;
extern volatile bool forced_tx;

// LoRaWAN stuff
#include "wisblock_cayenne.h"
extern WisCayenne g_solution_data;

// OLED
bool init_oled(void);
void oled_add_line(char *line);
void oled_show(void);
void oled_write_header(char *header_line);
void oled_clear(void);
void oled_write_line(int16_t line, int16_t y_pos, String text);
void oled_display(void);
void oled_power(bool on_off);
void display_show_menu(char *menu[], uint8_t menu_len, uint8_t sel_menu, uint8_t sel_item, bool display_saver = false, bool location_on = false);
void oled_saver(void *);
extern custom_param_s g_last_settings;
extern char line_str[];
extern char *g_regions_list[];
extern char *p_bw_menu[];
extern bool has_oled;

// UI
typedef enum disp_mode_num
{
	T_TOP_MENU = 0,
	T_INFO_MENU,
	T_SETT_MENU,
	T_MODE_MENU,
	T_LORAWAN_MENU,
	T_LORAP2P_MENU,
	S_SEND_INT,
	S_LPW_BAND,
	S_LPW_ADR,
	S_LPW_DR,
	S_LPW_TX,
	S_P2P_FREQ,
	S_P2P_SF,
	S_P2P_BW,
	S_P2P_CR,
	S_P2P_PPL,
	S_P2P_TX,
	S_SUB_NONE = 255
};
extern bool g_settings_ui;

// Button
#include "MillisTaskManager.h"

#define BUTTON_INT_PIN WB_IO5

/*
 * @brief button state.
 */
typedef enum
{
	SINGLE_CLICK = 0,
	DOUBLE_CLICK,
	LONG_PRESS,
	TRIPPLE_CLICK,
	QUAD_CLICK,
	FIVE_CLICK,
	SIX_CLICK,
	BUTTONSTATE_NONE,
} buttonState_t;

bool buttonInit(void);
uint8_t getButtonStatus(void);
void handle_button(void);
void buttonIntHandle(void);
extern MillisTaskManager mtmMain;
void mtm_handler(void *);
extern volatile uint8_t pressCount;
extern volatile bool display_power;

// ACC
#include <SparkFunLIS3DH.h>
#define ACC_INT_PIN WB_IO1
bool init_acc(bool active = false);
void clear_acc_int(void);
void read_acc(void);

// GNSS
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
bool init_gnss(bool active = false);
bool poll_gnss(void);
void gnss_handler(void *);
extern bool gnss_active;
extern uint16_t check_gnss_counter;
extern uint16_t check_gnss_max_try;
extern uint8_t max_sat;
extern uint8_t max_sat_unchanged;
extern float g_last_lat;
extern float g_last_long;
extern float g_last_accuracy;
extern uint32_t g_last_altitude;
extern uint8_t g_last_satellites;

#endif // _APP_H_
