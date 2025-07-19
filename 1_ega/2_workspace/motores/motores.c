#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "hardware/pwm.h"

// Pines para control de dirección y PWM
#define MOTOR_IN1 14
#define MOTOR_IN2 15
#define MOTOR_PWM 16  // GPIO con soporte PWM

// Configuración de la frecuencia PWM (Hz)
#define PWM_FREQ 1000

void motor_init() {
    // Dirección
    gpio_init(MOTOR_IN1);
    gpio_set_dir(MOTOR_IN1, GPIO_OUT);
    gpio_put(MOTOR_IN1, 0);

    gpio_init(MOTOR_IN2);
    gpio_set_dir(MOTOR_IN2, GPIO_OUT);
    gpio_put(MOTOR_IN2, 0);

    // PWM en MOTOR_PWM (GPIO 16)
    gpio_set_function(MOTOR_PWM, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(MOTOR_PWM);

    pwm_set_wrap(slice, 255);  // resolución de 8 bits (0-255)
    pwm_set_chan_level(slice, pwm_gpio_to_channel(MOTOR_PWM), 0);  // inicialmente apagado
    pwm_set_enabled(slice, true);
}

// Cambiar dirección del motor
void motor_set_direction(bool forward) {
    gpio_put(MOTOR_IN1, forward ? 1 : 0);
    gpio_put(MOTOR_IN2, forward ? 0 : 1);
}

// Cambiar velocidad (0 a 255)
void motor_set_speed(uint8_t speed) {
    uint slice = pwm_gpio_to_slice_num(MOTOR_PWM);
    pwm_set_chan_level(slice, pwm_gpio_to_channel(MOTOR_PWM), speed);
}

// Tarea FreeRTOS para probar el motor
void task_motor(void *params) {
    motor_init();
    motor_set_direction(true);  // Sentido "hacia adelante"

    const uint8_t speed_min = 0;
    const uint8_t speed_max = 255;
    const uint8_t step = 5;      // Paso de incremento de velocidad
    const TickType_t delay = pdMS_TO_TICKS(50);  // Delay entre pasos

    uint8_t speed = speed_min;

    while (1) {
        // Incremento gradual de velocidad
        for (speed = speed_min; speed <= speed_max; speed += step) {
            printf("Velocidad: %d\n", speed);
            motor_set_speed(speed);
            vTaskDelay(delay);
        }

        // Luego bajamos la velocidad de nuevo (opcional)
        for (speed = speed_max; speed >= speed_min; speed -= step) {
            printf("Velocidad: %d\n", speed);
            motor_set_speed(speed);
            vTaskDelay(delay);
            if (speed < step) break; // evitar underflow en uint8_t
        }
    }
}


int main() {
    stdio_init_all();

    xTaskCreate(task_motor, "Motor", configMINIMAL_STACK_SIZE + 100, NULL, 1, NULL);
    vTaskStartScheduler();

    while (1) { }  // Nunca llega acá
}
