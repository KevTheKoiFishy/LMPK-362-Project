#include "const.h"
#include "time.h"
#include "user_interface.h"

uint8_t  alarm_set_digits[4] = {2, 0, 0, 1}; // HHMM
uint8_t  alarm_set_digit_pos = 0;    // 0-3

void ui_process_keypad(uint16_t key) {
    if (key & 0x100) {
        // Key Pressed
        printf("\nKey Pressed: %c", key & 0xFF);
        key &= 0xFF;
        
        switch (key) {
            case '0':                                                                                      alarm_set_digits[alarm_set_digit_pos] = 0; break;
            case '1':                                                                                      alarm_set_digits[alarm_set_digit_pos] = 1; break;
            case '2':                                                                                      alarm_set_digits[alarm_set_digit_pos] = 2; break;
            case '3': if (alarm_set_digit_pos == 0) { break; }                                             alarm_set_digits[alarm_set_digit_pos] = 3; break;
            case '4': if (alarm_set_digit_pos == 0) { break; }                                             alarm_set_digits[alarm_set_digit_pos] = 4; break;
            case '5': if (alarm_set_digit_pos == 0) { break; }                                             alarm_set_digits[alarm_set_digit_pos] = 5; break;
            case '6': if (alarm_set_digit_pos == 0) { break; } if (!(alarm_set_digit_pos & 1)) { break; }  alarm_set_digits[alarm_set_digit_pos] = 6; break;
            case '7': if (alarm_set_digit_pos == 0) { break; } if (!(alarm_set_digit_pos & 1)) { break; }  alarm_set_digits[alarm_set_digit_pos] = 7; break;
            case '8': if (alarm_set_digit_pos == 0) { break; } if (!(alarm_set_digit_pos & 1)) { break; }  alarm_set_digits[alarm_set_digit_pos] = 8; break;
            case '9': if (alarm_set_digit_pos == 0) { break; } if (!(alarm_set_digit_pos & 1)) { break; }  alarm_set_digits[alarm_set_digit_pos] = 9; break;

            case '#': get_alarm_cfg().enabled ? alarm_disable() : alarm_enable(); break; // Toggle enable
            case '*': alarm_set_digit_pos = 0; break; // Reset position
            default: return; // Ignore other keys
        }

        set_alarm_time( (my_alarm_t) {
            .hours   = alarm_set_digits[0] * 10 + alarm_set_digits[1],
            .minutes = alarm_set_digits[2] * 10 + alarm_set_digits[3],
            .enabled = get_alarm_cfg().enabled,
            .fire    = get_alarm_cfg().fire
        } );
        alarm_set_digit_pos = (alarm_set_digit_pos + 1) & 0x03; // Wrap around 0-3
    }
}