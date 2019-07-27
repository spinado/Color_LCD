/*
 * Bafang LCD SW102 Bluetooth firmware
 *
 * Released under the GPL License, Version 3
 */
#include "ble_uart.h"
#include "app_timer.h"
#include "main.h"
#include "button.h"
#include "lcd.h"
#include "ugui.h"
#include "fonts.h"
#include "uart.h"
#include "utils.h"
#include "eeprom.h"
#include "screen.h"

/* Variable definition */

/* �GUI */
UG_GUI gui;

/* Buttons */
Button buttonM, buttonDWN, buttonUP, buttonPWR;

/* App Timer */
APP_TIMER_DEF(button_poll_timer_id); /* Button timer. */
#define BUTTON_POLL_INTERVAL APP_TIMER_TICKS(10/*ms*/, APP_TIMER_PRESCALER)
APP_TIMER_DEF(seconds_timer_id); /* Second counting timer. */
#define SECONDS_INTERVAL APP_TIMER_TICKS(1000/*ms*/, APP_TIMER_PRESCALER)
volatile uint32_t seconds_since_startup, seconds_since_reset;

#define FONT12_Y 14 // we want a little bit of extra space

Field faultHeading = { .variant = FieldDrawText, .drawText = { .font = &MY_FONT_8X12, .msg = "FAULT" }};
Field faultCode = { .variant = FieldDrawText, .drawText = { .font = &FONT_5X12 } };
Field addrHeading = { .variant = FieldDrawText, .drawText = { .font = &MY_FONT_8X12, .msg = "PC" } };
Field addrCode = { .variant = FieldDrawText, .drawText = { .font = &FONT_5X12 } };
Field infoHeading = { .variant = FieldDrawText, .drawText = { .font = &MY_FONT_8X12, .msg = "Info" }};
Field infoCode = { .variant = FieldDrawText, .drawText = { .font = &FONT_5X12 } };

Screen faultScreen = {
    {
        .x = 0, .y = 0,
        .width = -1, .height = -1,
        .color = ColorInvert,
        .field = &faultHeading
    },
    {
        .x = 0, .y = FONT12_Y,
        .width = -1, .height = -1,
        .color = ColorNormal,
        .field = &faultCode
    },
    {
        .x = 0, .y = 2 * FONT12_Y,
        .width = -1, .height = -1,
        .color = ColorNormal,
        .field = &addrHeading
    },
    {
        .x = 0, .y = 3 * FONT12_Y,
        .width = -1, .height = -1,
        .color = ColorNormal,
        .field = &addrCode
    },
    {
        .x = 0, .y = 4 * FONT12_Y,
        .width = -1, .height = -1,
        .color = ColorNormal,
        .field = &infoHeading
    },
    {
        .x = 0, .y = 5 * FONT12_Y,
        .width = -1, .height = -1,
        .color = ColorNormal,
        .field = &infoCode
    },
    {
        .field = NULL
    }
};

/* Function prototype */
static void system_power(bool state);
static void gpio_init(void);
static void init_app_timers(void);
static void button_poll_timer_timeout(void * p_context);
static void seconds_timer_timeout(void * p_context);
/* UART RX/TX */



/**
 * @brief Application main entry.
 */
int main(void)
{
  gpio_init();
  lcd_init();
  uart_init();

  // kevinh FIXME - turn off ble for now because somtimes it calls app_error_fault_handler(1...) from nrf51822_sw102_ble_advdata
  // ble_init();

  /* eeprom_init AFTER ble_init! */
  eeprom_init();
  // FIXME
  // eeprom_read_configuration(get_configuration_variables());
  init_app_timers();
  system_power(true);

  UG_ConsoleSetArea(0, 0, 63, 127);
  UG_ConsoleSetForecolor(C_WHITE);

  /*
  UG_FontSelect(&MY_FONT_BATTERY);
  UG_ConsolePutString("5\n");
  UG_ConsolePutString("4\n");
  UG_ConsolePutString("3\n");
  UG_ConsolePutString("2\n");
  UG_ConsolePutString("1\n");
  UG_ConsolePutString("0\n");
  */

  /*
  UG_FontSelect(&MY_FONT_8X12);
  static const char degC[] = { 31, 'C', 0 };
  UG_ConsolePutString(degC);
  */

  UG_FontSelect(&MY_FONT_8X12);
  UG_ConsolePutString("boot\n");
  lcd_refresh();


  //screenShow(&mainScreen);
  //screenUpdate(); // FIXME - move into main loop

  // APP_ERROR_HANDLER(5);

  // Enter main loop.
  while (1)
  {
    /* New RX packet to decode? */
    const uint8_t* p_rx_buffer = uart_get_rx_buffer_rdy();
    if (p_rx_buffer != NULL)
    {
      /* RX */
      // bool ok = decode_rx_stream(p_rx_buffer, get_motor_controller_data(), get_configuration_variables());
      /* TX */
#if 0
      if (ok)
      {
        uint8_t* tx_buffer = uart_get_tx_buffer();
        prepare_tx_stream(tx_buffer, get_motor_controller_data(), get_configuration_variables());
        uart_send_tx_buffer(tx_buffer);
      }
#endif
    }
  }

}

/**
 * @brief Hold system power (true) or not (false)
 */
static void system_power(bool state)
{
  if (state)
    nrf_gpio_pin_set(SYSTEM_POWER_HOLD__PIN);
  else
    nrf_gpio_pin_clear(SYSTEM_POWER_HOLD__PIN);
}



#if 0
/**
 * @brief Serialize structs & write to tx_buffer
 */
void prepare_tx_stream(uint8_t* tx_buffer, struct_motor_controller_data* p_motor_controller_data, struct_configuration_variables* p_configuration_variables)
{
  // start up byte
  tx_buffer[0] = 0x59;
  tx_buffer[1] = p_motor_controller_data->master_comm_package_id;
  tx_buffer[2] = p_motor_controller_data->slave_comm_package_id;

  // assist level
  if (p_configuration_variables->assist_level) // send assist level factor for normal operation
  {
    tx_buffer[3] = p_configuration_variables->assist_level_factor[((p_configuration_variables->assist_level) - 1)];
  }
  else if (p_motor_controller_data->walk_assist_level) // if walk assist function is enabled, send walk assist level factor
  {
    tx_buffer[3] = p_configuration_variables->walk_assist_level_factor[(p_configuration_variables->assist_level)];
  }
  else // send nothing
  {
    tx_buffer[3] = 0;
  }

  // set lights state
  // walk assist level state
  // set offroad state
  tx_buffer[4] = ((p_motor_controller_data->lights & 1)
      | ((p_motor_controller_data->walk_assist_level & 1) << 1)
      | ((p_motor_controller_data->offroad_mode & 1) << 2));

  // battery max current in amps
  tx_buffer[5] = p_configuration_variables->battery_max_current;

  // battery power
  tx_buffer[6] = p_configuration_variables->target_max_battery_power_div25;

  switch (p_motor_controller_data->master_comm_package_id)
  {
  case 0:
    // battery low voltage cut-off
    tx_buffer[7] = (uint8_t) (p_configuration_variables->battery_low_voltage_cut_off_x10 & 0xff);
    tx_buffer[8] = (uint8_t) (p_configuration_variables->battery_low_voltage_cut_off_x10 >> 8);
    break;

  case 1:
    // wheel perimeter
    tx_buffer[7] = (uint8_t) (p_configuration_variables->wheel_perimeter & 0xff);
    tx_buffer[8] = (uint8_t) (p_configuration_variables->wheel_perimeter >> 8);
    break;

  case 2:
    // wheel max speed
    tx_buffer[7] = p_configuration_variables->wheel_max_speed;
    break;

  case 3:
    // set motor type
    // enable/disable motor assistance without pedal rotation
    // enable/disable motor temperature limit function
    tx_buffer[7] = ((p_configuration_variables->motor_type & 3)
            | ((p_configuration_variables->motor_assistance_startup_without_pedal_rotation & 1) << 2)
            | ((p_configuration_variables->temperature_limit_feature_enabled & 3) << 3));

    // motor power boost startup state
    tx_buffer[8] = p_configuration_variables->startup_motor_power_boost_state;
    break;

  case 4:
    // startup motor power boost
    tx_buffer[7] = p_configuration_variables->startup_motor_power_boost_factor[((p_configuration_variables->assist_level) - 1)];
    // startup motor power boost time
    tx_buffer[8] = p_configuration_variables->startup_motor_power_boost_time;
    break;

  case 5:
    // startup motor power boost fade time
    tx_buffer[7] = p_configuration_variables->startup_motor_power_boost_fade_time;
    // boost feature enabled
    tx_buffer[8] = (p_configuration_variables->startup_motor_power_boost_feature_enabled & 1) ? 1 : 0;
    break;

  case 6:
    // motor over temperature min and max values to limit
    tx_buffer[7] = p_configuration_variables->motor_temperature_min_value_to_limit;
    tx_buffer[8] = p_configuration_variables->motor_temperature_max_value_to_limit;
    break;

  case 7:
    // offroad mode configuration
    tx_buffer[7] = ((p_configuration_variables->offroad_feature_enabled & 1)
            | ((p_configuration_variables->offroad_enabled_on_startup & 1) << 1));
    tx_buffer[8] = p_configuration_variables->offroad_speed_limit;
    break;

  case 8:
    // offroad mode power limit configuration
    tx_buffer[7] = p_configuration_variables->offroad_power_limit_enabled & 1;
    tx_buffer[8] = p_configuration_variables->offroad_power_limit_div25;
    break;

  case 9:
    // ramp up, amps per second
    tx_buffer[7] = p_configuration_variables->ramp_up_amps_per_second_x10;
    // cruise target speed
    if (p_configuration_variables->cruise_function_set_target_speed_enabled)
    {
      tx_buffer[8] = p_configuration_variables->cruise_function_target_speed_kph;
    }
    else
    {
      tx_buffer[8] = 0;
    }
    break;
  }

  // prepare crc of the package
  U16 crc_tx;
  crc_tx.u16 = 0xffff;
  for (uint8_t i = 0; i < 9; i++)
  {
    crc16(tx_buffer[i], &crc_tx.u16);
  }
  tx_buffer[9] = (crc_tx.u16 & 0xff);
  tx_buffer[10] = (crc_tx.u16 >> 8) & 0xff;
}
#endif


/* Hardware Initialization */

static void gpio_init(void)
{
  /* POWER_HOLD */
  nrf_gpio_cfg_output(SYSTEM_POWER_HOLD__PIN);

  /* LCD (none SPI) */
  nrf_gpio_cfg_output(LCD_COMMAND_DATA__PIN);
  nrf_gpio_pin_set(LCD_COMMAND_DATA__PIN);
  nrf_gpio_cfg_output(LCD_RES__PIN);
  nrf_gpio_pin_clear(LCD_RES__PIN); // Hold LCD in reset until initialization

  /* Buttons */
  InitButton(&buttonPWR, BUTTON_PWR__PIN, NRF_GPIO_PIN_NOPULL, BUTTON_ACTIVE_HIGH);
  InitButton(&buttonM, BUTTON_M__PIN, NRF_GPIO_PIN_PULLUP, BUTTON_ACTIVE_LOW);
  InitButton(&buttonUP, BUTTON_UP__PIN, NRF_GPIO_PIN_PULLUP, BUTTON_ACTIVE_LOW);
  InitButton(&buttonDWN, BUTTON_DOWN__PIN, NRF_GPIO_PIN_PULLUP, BUTTON_ACTIVE_LOW);
}

static void init_app_timers(void)
{
  // Start APP_TIMER to generate timeouts.
  APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, NULL);

  // Create&Start button_poll_timer
  app_timer_create(&button_poll_timer_id, APP_TIMER_MODE_REPEATED, button_poll_timer_timeout);
  app_timer_start(button_poll_timer_id, BUTTON_POLL_INTERVAL, NULL);

  // Create&Start timers.
  app_timer_create(&seconds_timer_id, APP_TIMER_MODE_REPEATED, seconds_timer_timeout);
  app_timer_start(seconds_timer_id, SECONDS_INTERVAL, NULL);
}



/* Event handler */

static void button_poll_timer_timeout(void *p_context)
{
    UNUSED_PARAMETER(p_context);

    PollButton(&buttonPWR);
    PollButton(&buttonM);
    PollButton(&buttonUP);
    PollButton(&buttonDWN);
}

static void seconds_timer_timeout(void *p_context)
{
    UNUSED_PARAMETER(p_context);

    seconds_since_startup++;
    seconds_since_reset++;
}

/**@brief       Callback function for errors, asserts, and faults.
 *
 * @details     This function is called every time an error is raised in app_error, nrf_assert, or
 *              in the SoftDevice.
 *
 *              DEBUG_NRF flag must be set to trigger asserts!
 *
 */
void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info)
{
  /*
  UG_FontSelect(&MY_FONT_8X12);
  char buf[32];
  sprintf(buf, "ERR 0x%lx\n", id);
  UG_ConsolePutString(buf);

  UG_ConsolePutString("PC\n");
  UG_FontSelect(&FONT_5X12);
  sprintf(buf, "0x%lx\n", pc);
  UG_ConsolePutString(buf);
  UG_FontSelect(&MY_FONT_8X12);
  UG_ConsolePutString("INFO\n");
  UG_FontSelect(&FONT_5X12);
  sprintf(buf, "0x%lx\n", info);
  UG_ConsolePutString(buf);
  lcd_refresh();
  */

  fieldPrintf(&faultCode, "0x%lx", id);
  fieldPrintf(&addrCode, "0x%06lx", pc);
  fieldPrintf(&infoCode, "%08lx", info);

  screenShow(&faultScreen);

  // FIXME - instead we should wait a few seconds and then reboot
  while (1);
}
