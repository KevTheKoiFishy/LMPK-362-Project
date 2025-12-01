#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "display.h"


uint32_t adc_fifo_out = 0;
void display_init_pins();
void display_init_timer();
void display_char_print(const char* buffer);


void init_adc_freerun() {
    adc_init();
    adc_gpio_init(45);
    adc_select_input(5);
    adc_read();

    // adc_run();
    // start freerunning conversions on the ADC after enabling it.
    hw_set_bits(&adc_hw->cs, ADC_CS_START_MANY_BITS);
}

void init_dma() {
    // Configure DMA Channel 0 to read from the ADC FIFO and write to the variable adc_fifo_out. 
    dma_channel_hw_addr(0)->read_addr = (uint32_t) &adc_hw->fifo;
    dma_channel_hw_addr(0)->write_addr = (uint32_t) &adc_fifo_out;
    
    // Set the transfer count, or TRANS_COUNT, to the arithmetic OR of two values.
    dma_channel_hw_addr(0)->transfer_count = (1u << 28) | 1u;

    // Initialize the control trigger register of the DMA to zero.
    dma_hw->ch[0].ctrl_trig = 0;

    // Create a uint32_t variable called temp
    u_int32_t temp = 0;

    // OR in the following:
    // left-shift the value 1 by the correct number of bits 
    // that would correspond to the DATA_SIZE field of the DMA channel 0 configuration, 
    // DMA_CH0_CTRL_TRIG_DATA_SIZE_VALUE_SIZE_HALFWORD _u(0x1)
    // but you assign it to temp first.
    temp |= (DMA_CH0_CTRL_TRIG_DATA_SIZE_VALUE_SIZE_HALFWORD << DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB);

    // Specify the ADC DREQ signal as the TREQ trigger.
    temp |= (DREQ_ADC << 17); 
    
    // set the EN bit
    temp |= 1u;

    // Write the temp variable to the dma_hw->ch[0].ctrl_trig register.
    dma_hw->ch[0].ctrl_trig = temp;
}

void init_adc_dma() {
    // call init_dma
    init_dma();

    // Call init_adc_freerun  
    init_adc_freerun();

    // Enable the ADC FIFO to store the generated samples.
    // Configure the ADC to send out a DREQ signal whenever a new sample is ready in the FIFO. 
    adc_fifo_setup(1, 1, 0, 0, 0);
}

//////////////////////////////////////////////////////////////////////////////

void control_update (void)
{
    // Configures our microcontroller to 
    // communicate over UART through the TX/RX pins
    stdio_init_all();

    display_init_pins();
    display_init_timer();

    init_adc_dma();
    char buffer[10];
    for(;;) {
        float f = (adc_fifo_out * 3.3) / 4095.0;
        snprintf(buffer, sizeof(buffer), "%1.7f", f);
        display_char_print(buffer);

        printf("ADC Result: %s     \r", buffer);
        fflush(stdout);
        
        sleep_ms(250);
    }

    for(;;);
}
