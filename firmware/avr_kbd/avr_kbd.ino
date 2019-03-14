/*
 * AVR keyboard firmware for ps2_cpld_kbd project
 * 
 * Designed to build on Arduino IDE.
 * 
 * @author Andy Karpov <andy.karpov@gmail.com>
 * Ukraine, 2019
 */

#include "ps2.h"
#include "matrix.h"
#include "ps2_codes.h"
#include <EEPROM.h>
#include <SPI.h>

#define DEBUG_MODE 0

// ---- Pins for Atmega328
#define PIN_KBD_CLK 3 // PD2
#define PIN_KBD_DAT 2 // PD2

// hardware SPI
#define PIN_SS 10 // SPI slave select

#define LED_PWR A0
#define LED_KBD A1
#define LED_TURBO A2
#define LED_SPECIAL A3

#define EEPROM_TURBO_ADDRESS 0x00
#define EEPROM_SPECIAL_ADDRESS 0x01

#define EEPROM_VALUE_TRUE 10
#define EEPROM_VALUE_FALSE 20

PS2KeyRaw kbd;

bool matrix[ZX_MATRIX_FULL_SIZE]; // matrix of pressed keys + special keys to be transmitted on CPLD side by SPI protocol

bool blink_state = false;

bool is_turbo = false;
bool is_special = false;

unsigned long t = 0;  // current time
unsigned long tl = 0; // blink poll time
unsigned long te = 0; // eeprom store time

SPISettings settingsA(8000000, MSBFIRST, SPI_MODE0); // SPI transmission settings

// transform PS/2 scancodes into internal matrix of pressed keys
void fill_kbd_matrix(int sc)
{

  static bool is_up=false, is_e=false, is_e1=false;
  static bool is_ctrl=false, is_alt=false, is_del=false, is_bksp = false, is_shift = false, is_esc = false;

  // is extended scancode prefix
  if (sc == 0xE0) {
    is_e = 1;
    return;
  }

 if (sc == 0xE1) {
    is_e = 1;
    is_e1 = 1;
    return;
  }

  // is key released prefix
  if (sc == 0xF0 && !is_up) {
    is_up = 1;
    return;
  }

  int scancode = sc + ((is_e || is_e1) ? 0x100 : 0);

  switch (scancode) {
  
    // Shift -> CS for ZX
    case PS2_L_SHIFT: 
    case PS2_R_SHIFT:
      matrix[ZX_K_CS] = !is_up;
      is_shift = !is_up;
      break;

    // Ctrl -> SS for ZX
    case PS2_L_CTRL:
    case PS2_R_CTRL:
      matrix[ZX_K_SS] = !is_up;
      is_ctrl = !is_up;
      break;

    // Alt (L) -> SS+CS for ZX
    case PS2_L_ALT:
      matrix[ZX_K_SS] = !is_up;
      matrix[ZX_K_CS] = !is_up;
      is_alt = !is_up;
      break;

    // Alt (R) -> SS+CS for ZX
    case PS2_R_ALT:
      matrix[ZX_K_SS] = !is_up;
      matrix[ZX_K_CS] = !is_up;
      is_alt = !is_up;
      break;

    // Del -> SS+C for ZX
    case PS2_DELETE:
       matrix[ZX_K_SS] = !is_up;
       matrix[ZX_K_C] =  !is_up;
      is_del = !is_up;
    break;

    // Ins -> SS+A for ZX
    case PS2_INSERT:
       matrix[ZX_K_SS] = !is_up;
       matrix[ZX_K_A] =  !is_up;
    break;

    // Cursor -> CS + 5,6,7,8
    case PS2_UP:
      matrix[ZX_K_CS] = !is_up;
      matrix[ZX_K_7] = !is_up;
      break;
    case PS2_DOWN:
      matrix[ZX_K_CS] = !is_up;
      matrix[ZX_K_6] = !is_up;
      break;
    case PS2_LEFT:
      matrix[ZX_K_CS] = !is_up;
      matrix[ZX_K_5] = !is_up;
      break;
    case PS2_RIGHT:
      matrix[ZX_K_CS] = !is_up;
      matrix[ZX_K_8] = !is_up;
      break;

    // ESC -> CS+SPACE for ZX
    case PS2_ESC:
      matrix[ZX_K_CS] = !is_up;
      matrix[ZX_K_SP] = !is_up;
      is_esc = !is_up;
      break;

    // Backspace -> CS+0
    case PS2_BACKSPACE:
      matrix[ZX_K_CS] = !is_up;
      matrix[ZX_K_0] = !is_up;
      is_bksp = !is_up;
      break;

    // Enter
    case PS2_ENTER:
    case PS2_KP_ENTER:
      matrix[ZX_K_ENT] = !is_up;
      break;

    // Space
    case PS2_SPACE:
      matrix[ZX_K_SP] = !is_up;
      break;

    // Letters & numbers
    case PS2_A: matrix[ZX_K_A] = !is_up; break;
    case PS2_B: matrix[ZX_K_B] = !is_up; break;
    case PS2_C: matrix[ZX_K_C] = !is_up; break;
    case PS2_D: matrix[ZX_K_D] = !is_up; break;
    case PS2_E: matrix[ZX_K_E] = !is_up; break;
    case PS2_F: matrix[ZX_K_F] = !is_up; break;
    case PS2_G: matrix[ZX_K_G] = !is_up; break;
    case PS2_H: matrix[ZX_K_H] = !is_up; break;
    case PS2_I: matrix[ZX_K_I] = !is_up; break;
    case PS2_J: matrix[ZX_K_J] = !is_up; break;
    case PS2_K: matrix[ZX_K_K] = !is_up; break;
    case PS2_L: matrix[ZX_K_L] = !is_up; break;
    case PS2_M: matrix[ZX_K_M] = !is_up; break;
    case PS2_N: matrix[ZX_K_N] = !is_up; break;
    case PS2_O: matrix[ZX_K_O] = !is_up; break;
    case PS2_P: matrix[ZX_K_P] = !is_up; break;
    case PS2_Q: matrix[ZX_K_Q] = !is_up; break;
    case PS2_R: matrix[ZX_K_R] = !is_up; break;
    case PS2_S: matrix[ZX_K_S] = !is_up; break;
    case PS2_T: matrix[ZX_K_T] = !is_up; break;
    case PS2_U: matrix[ZX_K_U] = !is_up; break;
    case PS2_V: matrix[ZX_K_V] = !is_up; break;
    case PS2_W: matrix[ZX_K_W] = !is_up; break;
    case PS2_X: matrix[ZX_K_X] = !is_up; break;
    case PS2_Y: matrix[ZX_K_Y] = !is_up; break;
    case PS2_Z: matrix[ZX_K_Z] = !is_up; break;

    // digits
    case PS2_0: matrix[ZX_K_0] = !is_up; break;
    case PS2_1: matrix[ZX_K_1] = !is_up; break;
    case PS2_2: matrix[ZX_K_2] = !is_up; break;
    case PS2_3: matrix[ZX_K_3] = !is_up; break;
    case PS2_4: matrix[ZX_K_4] = !is_up; break;
    case PS2_5: matrix[ZX_K_5] = !is_up; break;
    case PS2_6: matrix[ZX_K_6] = !is_up; break;
    case PS2_7: matrix[ZX_K_7] = !is_up; break;
    case PS2_8: matrix[ZX_K_8] = !is_up; break;
    case PS2_9: matrix[ZX_K_9] = !is_up; break;

    // Keypad digits
    case PS2_KP_0: matrix[ZX_K_0] = !is_up; break;
    case PS2_KP_1: matrix[ZX_K_1] = !is_up; break;
    case PS2_KP_2: matrix[ZX_K_2] = !is_up; break;
    case PS2_KP_3: matrix[ZX_K_3] = !is_up; break;
    case PS2_KP_4: matrix[ZX_K_4] = !is_up; break;
    case PS2_KP_5: matrix[ZX_K_5] = !is_up; break;
    case PS2_KP_6: matrix[ZX_K_6] = !is_up; break;
    case PS2_KP_7: matrix[ZX_K_7] = !is_up; break;
    case PS2_KP_8: matrix[ZX_K_8] = !is_up; break;
    case PS2_KP_9: matrix[ZX_K_9] = !is_up; break;

    // '/" -> SS+P / SS+7
    case PS2_QUOTE:
      matrix[ZX_K_SS] = !is_up;
      matrix[is_shift ? ZX_K_P : ZX_K_7] = !is_up;
      break;

    // ,/< -> SS+N / SS+R
    case PS2_COMMA:
      matrix[ZX_K_SS] = !is_up;
      matrix[is_shift ? ZX_K_R : ZX_K_N] = !is_up;
      break;

    // ./> -> SS+M / SS+T
    case PS2_PERIOD:
    case PS2_KP_PERIOD:
      matrix[ZX_K_SS] = !is_up;
      matrix[is_shift ? ZX_K_T : ZX_K_M] = !is_up;
      break;

    // ;/: -> SS+O / SS+Z
    case PS2_SEMICOLON:
      matrix[ZX_K_SS] = !is_up;
      matrix[is_shift ? ZX_K_Z : ZX_K_O] = !is_up;
      break;

    // [,{ -> SS+Y / SS+F
    case PS2_L_BRACKET:
      matrix[ZX_K_SS] = !is_up;
      matrix[is_shift ? ZX_K_F : ZX_K_Y] = !is_up;
      break;

    // ],} -> SS+U / SS+G
    case PS2_R_BRACKET:
      matrix[ZX_K_SS] = !is_up;
      matrix[is_shift ? ZX_K_G : ZX_K_U] = !is_up;
      break;

    // /,? -> SS+V / SS+C
    case PS2_SLASH:
    case PS2_KP_SLASH:
      matrix[ZX_K_SS] = !is_up;
      matrix[is_shift ? ZX_K_C : ZX_K_V] = !is_up;
      break;

    // \,| -> SS+D / SS+S
    case PS2_BACK_SLASH:
      matrix[ZX_K_SS] = !is_up;
      matrix[is_shift ? ZX_K_S : ZX_K_D] = !is_up;
      break;

    // =,+ -> SS+L / SS+K
    case PS2_EQUALS:
      matrix[ZX_K_SS] = !is_up;
      matrix[is_shift ? ZX_K_K : ZX_K_L] = !is_up;
      break;

    // -,_ -> SS+J / SS+0
    case PS2_MINUS:
      matrix[ZX_K_SS] = !is_up;
      matrix[is_shift ? ZX_K_0 : ZX_K_J] = !is_up;
      break;

    // `,~ -> SS+X / SS+A
    case PS2_ACCENT:
      matrix[ZX_K_SS] = !is_up;
      matrix[is_shift ? ZX_K_A : ZX_K_X] = !is_up;
      break;

    // Keypad * -> SS+B
    case PS2_KP_STAR:
      matrix[ZX_K_SS] = !is_up;
      matrix[ZX_K_B] = !is_up;
      break;

    // Keypad - -> SS+J
    case PS2_KP_MINUS:
      matrix[ZX_K_SS] = !is_up;
      matrix[ZX_K_J] = !is_up;
      break;

    // Keypad + -> SS+K
    case PS2_KP_PLUS:
      matrix[ZX_K_SS] = !is_up;
      matrix[ZX_K_K] = !is_up;
      break;

    // Tab
    case PS2_TAB:
      matrix[ZX_K_CS] = !is_up;
      matrix[ZX_K_I] = !is_up;
      break;

    // CapsLock
    case PS2_CAPS:
      matrix[ZX_K_SS] = !is_up;
      matrix[ZX_K_CS] = !is_up;
      break;

    // PgUp -> CS+3 for ZX
    case PS2_PGUP:
      matrix[ZX_K_CS] = !is_up;
      matrix[ZX_K_3] = !is_up;
      break;

    // PgDn -> CS+4 for ZX
    case PS2_PGDN:
      matrix[ZX_K_CS] = !is_up;
      matrix[ZX_K_4] = !is_up;
      break;

    // Scroll Lock -> Turbo
    case PS2_SCROLL: 
      if (is_up) {
        is_turbo = !is_turbo;        
        eeprom_store_value(EEPROM_TURBO_ADDRESS, is_turbo);
        matrix[ZX_K_TURBO] = is_turbo;
      }
    break;

    // PrintScreen -> Special
    case PS2_PSCR1: 
      if (is_up) {
        is_special = !is_special;        
        eeprom_store_value(EEPROM_SPECIAL_ADDRESS, is_special);
        matrix[ZX_K_SPECIAL] = is_special;
      }
    break;

    // F2 -> Magick button
    case PS2_F2:
      if (is_up) {
        do_magick();
      }
    break;
  
  }

  // Ctrl+Alt+Del -> RESET
  if (is_ctrl && is_alt && is_del) {
    is_ctrl = false;
    is_alt = false;
    is_del = false;
    is_shift = false;
    do_reset();
  }

  // Ctrl+Alt+Bksp -> REINIT controller
  if (is_ctrl && is_alt && is_bksp) {
      is_ctrl = false;
      is_alt = false;
      is_bksp = false;
      is_shift = false;
      clear_matrix(ZX_MATRIX_SIZE);
      matrix[ZX_K_RESET] = true;
      transmit_keyboard_matrix();
      matrix[ZX_K_S] = true;
      transmit_keyboard_matrix();
      delay(500);
      matrix[ZX_K_RESET] = false;
      transmit_keyboard_matrix();
      delay(500);
      matrix[ZX_K_S] = false;
  }

   // clear flags
   is_up = 0;
   if (is_e1) {
    is_e1 = 0;
   } else {
     is_e = 0;
   }
}

uint8_t get_matrix_byte(uint8_t pos)
{
  uint8_t result = 0;
  for (uint8_t i=0; i<8; i++) {
    uint8_t k = pos*8 + i;
    if (k < ZX_MATRIX_FULL_SIZE) {
      bitWrite(result, i, matrix[k]);
    }
  }
  return result;
}

void spi_send(uint8_t addr, uint8_t data) 
{
      SPI.beginTransaction(settingsA);
      digitalWrite(PIN_SS, LOW);
      uint8_t cmd = SPI.transfer(addr); // command (1...6)
      uint8_t res = SPI.transfer(data); // data byte
      digitalWrite(PIN_SS, HIGH);
      SPI.endTransaction();
}

// transmit keyboard matrix from AVR to CPLD side via SPI
void transmit_keyboard_matrix()
{
    uint8_t bytes = 6;
    for (uint8_t i=0; i<bytes; i++) {
      uint8_t data = get_matrix_byte(i);
      spi_send(i+1, data);
    }
}

void do_reset()
{
  clear_matrix(ZX_MATRIX_SIZE);
  matrix[ZX_K_RESET] = true;
  transmit_keyboard_matrix();
  delay(500);
  matrix[ZX_K_RESET] = false;
  transmit_keyboard_matrix();  
}

void do_magick()
{
  matrix[ZX_K_MAGICK] = true;
  transmit_keyboard_matrix();
  delay(500);
  matrix[ZX_K_MAGICK] = false;
  transmit_keyboard_matrix();
}

void clear_matrix(int clear_size)
{
    // all keys up
  for (int i=0; i<clear_size; i++) {
      matrix[i] = false;
  }
}

bool eeprom_restore_value(int addr, bool default_value)
{
  byte val;  
  val = EEPROM.read(addr);
  if ((val == EEPROM_VALUE_TRUE) || (val == EEPROM_VALUE_FALSE)) {
    return (val == EEPROM_VALUE_TRUE) ? true : false;
  } else {
    EEPROM.update(addr, (default_value ? EEPROM_VALUE_TRUE : EEPROM_VALUE_FALSE));
    return default_value;
  }
}

void eeprom_store_value(int addr, bool value)
{
  byte val = (value ? EEPROM_VALUE_TRUE : EEPROM_VALUE_FALSE);
  EEPROM.update(addr, val);
}

void eeprom_restore_values()
{
  is_turbo = eeprom_restore_value(EEPROM_TURBO_ADDRESS, is_turbo);
  is_special = eeprom_restore_value(EEPROM_SPECIAL_ADDRESS, is_special);
  matrix[ZX_K_TURBO] = is_turbo;
  matrix[ZX_K_SPECIAL] = is_special;
}

void eeprom_store_values()
{
  eeprom_store_value(EEPROM_TURBO_ADDRESS, is_turbo);
  eeprom_store_value(EEPROM_SPECIAL_ADDRESS, is_special);
}

// initial setup
void setup()
{
  Serial.begin(115200);
  Serial.flush();
  SPI.begin();

  pinMode(PIN_SS, OUTPUT);
  digitalWrite(PIN_SS, HIGH);

  pinMode(LED_PWR, OUTPUT);
  pinMode(LED_KBD, OUTPUT);
  pinMode(LED_TURBO, OUTPUT);
  pinMode(LED_SPECIAL, OUTPUT);
  
  digitalWrite(LED_PWR, HIGH);
  digitalWrite(LED_KBD, HIGH);
  digitalWrite(LED_TURBO, LOW);
  digitalWrite(LED_SPECIAL, LOW);

  // ps/2

  pinMode(PIN_KBD_CLK, INPUT_PULLUP);
  pinMode(PIN_KBD_DAT, INPUT_PULLUP);

  // zx signals (output)

  // clear full matrix
  clear_matrix(ZX_MATRIX_FULL_SIZE);

  // restore saved modes from EEPROM
  eeprom_restore_values();

  digitalWrite(LED_TURBO, is_turbo ? HIGH : LOW);
  digitalWrite(LED_SPECIAL, is_special ? HIGH: LOW);

Serial.println(F("ZX PS/2 Keyboard controller v1.0"));

#if DEBUG_MODE
  Serial.println(F("Reset on boot..."));
#endif

  do_reset();

#if DEBUG_MODE
  Serial.println(F("done"));
  Serial.println(F("Keyboard init..."));
#endif

  kbd.begin(PIN_KBD_DAT, PIN_KBD_CLK);

#if DEBUG_MODE
  Serial.println(F("done"));
#endif

  digitalWrite(LED_KBD, LOW);
  
}


// main loop
void loop()
{
  unsigned long n = millis();
  
  if (kbd.available()) {
    int c = kbd.read();
    blink_state = true;
    tl = n;
    digitalWrite(LED_KBD, HIGH);
#if DEBUG_MODE    
    Serial.print(F("Scancode: "));
    Serial.println(c, HEX);
#endif
    fill_kbd_matrix(c);
  }

  // transmit kbd always
  transmit_keyboard_matrix();


  // update leds
  if (n - tl >= 200) {
    digitalWrite(LED_KBD, LOW);
    blink_state = false;

    digitalWrite(LED_TURBO, is_turbo ? HIGH : LOW);
    digitalWrite(LED_SPECIAL, is_special ? HIGH: LOW);
    
  }
}
