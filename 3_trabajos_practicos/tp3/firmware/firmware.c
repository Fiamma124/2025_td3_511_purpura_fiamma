#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/irq.h"

#include "lcd.h"
#include "helper.h"
#include "FreeRTOS.h"
#include "task.h"

#include "semphr.h"

#define PWM_PIN 16
#define INPUT_PIN 15
#define PIN_SDA 0
#define PIN_SCL 1


SemaphoreHandle_t xFlancoSemaphore ;

void fq_ISR(uint gpio, uint32_t event_mask){
    static BaseType_t xTask = pdFALSE ;
    xSemaphoreGiveFromISR(xFlancoSemaphore , &xTask);
    portYIELD_FROM_ISR(xTask);
}



void task_print(void *params){
    char buffer[16];

    while(1) {
        //int cuenta = uxSemaphoreGetCount(xFlancoSemaphore);
        //sprintf(buffer, "Frec: %d Hz", cuenta);
        //printf("Valor del semáforo: %d\n", cuenta);
        int x = uxSemaphoreGetCount(xFlancoSemaphore);
        sprintf(buffer, "%d", x);
        //sprintf(buffer, "Frec: %d Hz", uxSemaphoreGetCount(xFlancoSemaphore));
        lcd_clear();
        //printf("%s\n", buffer);
        lcd_set_cursor(1, 0);
   		lcd_string(buffer);
        xQueueReset(xFlancoSemaphore);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


int main()
{
    stdio_init_all();

    //Inicializo los pines
    gpio_init(PWM_PIN);
    gpio_set_dir(PWM_PIN, true);
    gpio_init(INPUT_PIN);
    gpio_set_dir(INPUT_PIN, false);

    // Inicializo el I2C a 100 KHz
    i2c_init(i2c0, 100000);

    // Habilito el I2C en los GPIOs
    gpio_set_function(PIN_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_SDA);
    gpio_pull_up(PIN_SCL);

    //Inicializo la interrupcion
    gpio_set_irq_enabled_with_callback(INPUT_PIN, GPIO_IRQ_EDGE_RISE, true, fq_ISR);

    //Inicializacion del semaforo
    xFlancoSemaphore = xSemaphoreCreateCounting(10000, 0);
    
    //Inicializacion del PWM
    pwm_user_init(16, 4747);

    //Inicializacion del display
    lcd_init(i2c0, 0x27);
    lcd_clear();
    // Escribe un mensaje
    //lcd_set_cursor(0, 0);
    //lcd_string("Hello world!");

    xTaskCreate(task_print, "Printer", 4*configMINIMAL_STACK_SIZE, NULL, 2,  NULL);

    vTaskStartScheduler();
    while (true) {
    }
}

