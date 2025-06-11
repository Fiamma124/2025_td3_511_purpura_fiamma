#include <stdio.h>
#include "pico/stdlib.h"

#include "lcd.h"
#include "helper.h"
#include "FreeRTOS.h"
#include "task.h"
#include "pico/cyw43_arch.h"
#include "semphr.h"

#define IN_PIN 15
#define PWM_PIN 16


TaskHandle_t Freq_Counter;
TaskHandle_t Printer;
SemaphoreHandle_t xFlancoSemaphore ;

void task_fq_counter(void *params){
    bool actual = false;
    bool anterior = false;
    // Aseguro que sea consistente el bloqueo
    TickType_t tick = xTaskGetTickCount();

    while(1) {
        anterior = gpio_get(IN_PIN);
        for (int i = 0; i < 1000; i++){
           actual = gpio_get(IN_PIN);
            if((actual != anterior) && (actual == true)){
                xSemaphoreGive(xFlancoSemaphore);
            }
            anterior = actual;
        vTaskDelayUntil(&tick, pdMS_TO_TICKS(0.1));
        
        }
    }
}
void task_print(void *params){
    while(1){
        uint32_t count = uxSemaphoreGetCount(xFlancoSemaphore);
        xQueueReset(xFlancoSemaphore);
        printf ("La frecuancia es de %d \n", count);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


int main()
{
    stdio_init_all();
    //cyw43_arch_init();
    gpio_init (IN_PIN);
    gpio_init(PWM_PIN);
    gpio_set_dir(IN_PIN, false);
    gpio_set_dir(PWM_PIN, true);
    pwm_user_init(16, 7777);

    //vSemaphoreCreateBinary(xFlancoSemaphore);
    xFlancoSemaphore = xSemaphoreCreateCounting(10000, 0);

    xTaskCreate(task_fq_counter, "Freq_Counter", configMINIMAL_STACK_SIZE, NULL, 1,  NULL);
    xTaskCreate(task_print, "Printer", 4*configMINIMAL_STACK_SIZE, NULL, 2,  NULL);

    vTaskStartScheduler();
    while (true) {
    }
}

/*
void task_in(){
    while(1){
        if (gpio_get(IN_PIN)){
            xSemaphoreGive(counting);
            while (gpio_get(IN_PIN));
        }
    }
}
void task_freq (){
     //DE MAYOR PRIORIDAD QUE TASK_IN
     //ASIGNAR MAYOR STACK SIZE
    while(1){
        uint32_t count = uxSemaphoreGetCount(counting);
        xQueueReset(counting);
        printf ("La frecuancia es de %d \n", count*10);
        xTaskDelay(pdMS_TO_TICKS(100));
    }
}

main {
    gpio_init (IN_PIN);
    gpio_set_dir (IN_PIN , false);

    counting //inicializo semafotor
}
    */