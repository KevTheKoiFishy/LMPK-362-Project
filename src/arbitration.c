#include "const.h"

#include "arbitration.h"
#include "ambience_control.h"
#include "time.h"

void timer0_alarm_irq_dispatcher() {
    if (timer0_hw -> intr & (1 << VOL_RAMP_TIM_ALARM)) {
        volume_ramp_isr();
    } else if (timer0_hw -> intr & (1 << TIME_STEP_ALARM)) {
        time_update_isr();
    }
}