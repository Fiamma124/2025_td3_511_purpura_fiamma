#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "hardware/irq.h"
#include "semphr.h"

#define IN_PIN 27
SemaphoreHandle_t xFlancoSemaphore ;

void fq_ISR(uint gpio, uint32_t event_mask) {
    static uint32_t last_time = 0;
    uint32_t now = to_ms_since_boot(get_absolute_time());

    if (now - last_time > 75) {  // Ignoramos flancos que ocurran a menos de 50ms entre sí
        BaseType_t xTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(xFlancoSemaphore, &xTaskWoken);
        portYIELD_FROM_ISR(xTaskWoken);
        last_time = now;
    }
}

void Tacometro(void *params) {
    int counter = 0;
    float vueltas = 0;
    float vel_pwm = 0;

    while(1) {
        counter = 0;

        // Dentro de la ventana de 1 segundo, contamos flancos
        TickType_t start = xTaskGetTickCount();
        while (xTaskGetTickCount() - start < pdMS_TO_TICKS(1000)) {
            if (xSemaphoreTake(xFlancoSemaphore, pdMS_TO_TICKS(50)) == pdTRUE) {
                counter++;
            }
        }

        vueltas = counter / 21.0f;
        //vel_pwm = counter * PENDIENTE + ORDENADA;

        printf("Activaciones por segundo: %d\n", counter);
        printf("Vueltas por segundo: %.2f\n", vueltas);
        //printf("Velocidad PWM: %.2f\n", vel_pwm);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
/*
void Tacometro(void *params) {
    int counter = 0;
    float vueltas = 0;
    int estado_anterior = gpio_get(IN_PIN);

    TickType_t start_tick = xTaskGetTickCount();  // tiempo de referencia
    const TickType_t periodo = pdMS_TO_TICKS(1000);  // 1 segundo

    while (true) {
        int estado_actual = gpio_get(IN_PIN);

        // Detectar cambio de estado (flanco de subida)
        if (estado_actual != estado_anterior) {
            estado_anterior = estado_actual;

            if (estado_actual == 1) {
                counter++;
            }
        }

        // ¿Pasó 1 segundo?
        if ((xTaskGetTickCount() - start_tick) >= periodo) {
            vueltas = counter / 21.0f;
            printf("Activaciones por segundo: %d\n", counter);
            printf("Vueltas por segundo: %.2f\n", vueltas);

            counter = 0;
            vueltas = 0;
            start_tick = xTaskGetTickCount();  // reinicio del conteo
        }

        vTaskDelay(pdMS_TO_TICKS(1));  // respiro para evitar uso 100% CPU
    }
}*/

int main()
{
    stdio_init_all();

    gpio_init(IN_PIN);
    gpio_set_dir(IN_PIN, GPIO_IN);
    //gpio_pull_down(IN_PIN);
    //Inicializo la interrupcion
    gpio_set_irq_enabled_with_callback(IN_PIN, GPIO_IRQ_EDGE_RISE, true, fq_ISR);

    //Inicializacion del semaforo
    xFlancoSemaphore = xSemaphoreCreateCounting(10000, 0);
    // Crear la tarea
    xTaskCreate(Tacometro, "Tacometro", configMINIMAL_STACK_SIZE + 100, NULL, 1, NULL);

    // Iniciar el scheduler
    vTaskStartScheduler();

    // Si llegás acá, hubo un error
    while (true) { }
}
