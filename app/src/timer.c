#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/rcc.h>
#include "timer.h"

#define PRESCALER (72)
#define ARR_VALUE (1000)

void timer_setup(void){
    rcc_periph_clock_enable(RCC_TIM2);
    timer_set_mode(TIM2,TIM_CR1_CKD_CK_INT,TIM_CR1_CMS_EDGE,TIM_CR1_DIR_UP);
    timer_set_oc_mode(TIM2,TIM_OC1,TIM_OCM_PWM1);
    timer_enable_counter(TIM2);
    timer_enable_oc_output(TIM2,TIM_OC1);
    timer_set_prescaler(TIM2,PRESCALER-1);
    timer_set_period(TIM2,ARR_VALUE-1);
}

void timer_pwm_set_duty_cycle(int duty_cycle){
    const int raw_val = (ARR_VALUE/100) * duty_cycle;
    timer_set_oc_value(TIM2,TIM_OC1,raw_val);
}