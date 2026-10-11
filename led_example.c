#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>


// LEDS

// Red = led0
static const struct gpio_dt_spec red =
    GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

// Green = led1
static const struct gpio_dt_spec green =
    GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

// Blue = led2
static const struct gpio_dt_spec blue =
    GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);


// UART

#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)

static const struct device *const uart_dev =
    DEVICE_DT_GET(UART_DEVICE_NODE);


// FIFO

// UART task writes received colors to this FIFO.
// Dispatcher task reads them from the FIFO.
K_FIFO_DEFINE(dispatcher_fifo);


// Data type stored in the FIFO.
struct data_t {
    void *fifo_reserved;
    char color;
};


// Red
K_MUTEX_DEFINE(red_mutex);
K_CONDVAR_DEFINE(red_signal);

// Yellow 
K_MUTEX_DEFINE(yellow_mutex);
K_CONDVAR_DEFINE(yellow_signal);

// Green
K_MUTEX_DEFINE(green_mutex);
K_CONDVAR_DEFINE(green_signal);

bool red_requested = false;
bool yellow_requested = false;
bool green_requested = false;


// Dispatcher waits for the light task to finish.
K_SEM_DEFINE(release_sem, 0, 1);


int init_led(void);
int init_uart(void);

void uart_task(void *, void *, void *);
void dispatcher_task(void *, void *, void *);

void red_led_task(void *, void *, void *);
void yellow_led_task(void *, void *, void *);
void green_led_task(void *, void *, void *);


#define STACKSIZE 500
#define PRIORITY 5


K_THREAD_DEFINE(
    uart_thread,
    STACKSIZE,
    uart_task,
    NULL,
    NULL,
    NULL,
    PRIORITY,
    0,
    0
);


K_THREAD_DEFINE(
    dispatcher_thread,
    STACKSIZE,
    dispatcher_task,
    NULL,
    NULL,
    NULL,
    PRIORITY,
    0,
    0
);


K_THREAD_DEFINE(
    red_thread,
    STACKSIZE,
    red_led_task,
    NULL,
    NULL,
    NULL,
    PRIORITY,
    0,
    0
);


K_THREAD_DEFINE(
    yellow_thread,
    STACKSIZE,
    yellow_led_task,
    NULL,
    NULL,
    NULL,
    PRIORITY,
    0,
    0
);


K_THREAD_DEFINE(
    green_thread,
    STACKSIZE,
    green_led_task,
    NULL,
    NULL,
    NULL,
    PRIORITY,
    0,
    0
);


// MAIN

int main(void)
{
    int ret;

    // Initialize LEDs
    ret = init_led();

    if (ret < 0) {
        printk("LED initialization failed\n");
        return 0;
    }


    // Initialize UART
    ret = init_uart();

    if (ret < 0) {
        printk("UART initialization failed\n");
        return 0;
    }


    printk("Main started\n");
    printk("Send R, Y or G through serial port\n");


    while (true) {
        k_msleep(1000);
    }


    return 0;
}


// LED INITIALIZATION

int init_led(void)
{
    int ret;


    // Configure RED
    ret = gpio_pin_configure_dt(&red, GPIO_OUTPUT_ACTIVE);

    if (ret < 0) {
        printk("Error: Red LED configure failed\n");
        return ret;
    }


    // Configure GREEN
    ret = gpio_pin_configure_dt(&green, GPIO_OUTPUT_ACTIVE);

    if (ret < 0) {
        printk("Error: Green LED configure failed\n");
        return ret;
    }

    ret = gpio_pin_configure_dt(&blue, GPIO_OUTPUT_ACTIVE);

    if (ret < 0) {
        printk("Error: Blue LED configure failed\n");
        return ret;
    }


    // Turn all LEDs off
    gpio_pin_set_dt(&red, 0);
    gpio_pin_set_dt(&green, 0);
    gpio_pin_set_dt(&blue, 0);


    printk("LEDs initialized OK\n");

    return 0;
}


int init_uart(void)
{
    // Check that UART is ready
    if (!device_is_ready(uart_dev)) {

        printk("UART device not ready\n");

        return -1;
    }


    printk("UART initialized OK\n");

    return 0;
}


// UART TASK

void uart_task(void *, void *, void *)
{
    char rc;


    while (true) {

        if (uart_poll_in(uart_dev, &rc) == 0) {


            // Accept only R, Y and G
            if (rc == 'R' || rc == 'Y' || rc == 'G') {

                printk("UART received: %c\n", rc);

                struct data_t *buf =
                    k_malloc(sizeof(struct data_t));


                if (buf == NULL) {

                    printk("Memory allocation failed\n");

                } else {

                    // Save received color
                    buf->color = rc;


                    // Put color into FIFO
                    k_fifo_put(
                        &dispatcher_fifo,
                        buf
                    );


                    printk("Added %c to FIFO\n", rc);
                }
            }
        }


        k_msleep(10);
    }
}


// DISPATCHER TASK

void dispatcher_task(void *, void *, void *)
{
    while (true) {

        // Wait for data from UART task
        struct data_t *item =
            k_fifo_get(
                &dispatcher_fifo,
                K_FOREVER
            );


        // Read the color
        char color = item->color;


        // Free allocated memory
        k_free(item);


        printk("Dispatcher received: %c\n", color);


        // RED
        if (color == 'R') {

            printk("Dispatcher -> RED\n");

            k_mutex_lock(
                &red_mutex,
                K_FOREVER
            );

            red_requested = true;

            k_condvar_signal(
                &red_signal
            );

            k_mutex_unlock(
                &red_mutex
            );
        }


        // YELLOW
        else if (color == 'Y') {

            printk("Dispatcher -> YELLOW\n");

            k_mutex_lock(
                &yellow_mutex,
                K_FOREVER
            );

            yellow_requested = true;

            k_condvar_signal(
                &yellow_signal
            );

            k_mutex_unlock(
                &yellow_mutex
            );
        }


        // GREEN
        else if (color == 'G') {

            printk("Dispatcher -> GREEN\n");

            k_mutex_lock(
                &green_mutex,
                K_FOREVER
            );

            green_requested = true;

            k_condvar_signal(
                &green_signal
            );

            k_mutex_unlock(
                &green_mutex
            );
        }


        // Wait until the light task is finished
        k_sem_take(
            &release_sem,
            K_FOREVER
        );
    }
}


// =====================================================
// RED TASK
// =====================================================

void red_led_task(void *, void *, void *)
{
    printk("Red LED thread started\n");


    while (true) {

        // Lock mutex
        k_mutex_lock(
            &red_mutex,
            K_FOREVER
        );


        // Wait for dispatcher signal
        while (!red_requested) {

            k_condvar_wait(
                &red_signal,
                &red_mutex,
                K_FOREVER
            );
        }


        red_requested = false;


        k_mutex_unlock(
            &red_mutex
        );


        // Red ON
        gpio_pin_set_dt(&red, 1);

        printk("RED ON\n");


        k_sleep(K_SECONDS(1));


        // Red OFF
        gpio_pin_set_dt(&red, 0);

        printk("RED OFF\n");


        // Tell dispatcher that task is finished
        k_sem_give(
            &release_sem
        );
    }
}


void yellow_led_task(void *, void *, void *)
{
    printk("Yellow LED thread started\n");


    while (true) {

        k_mutex_lock(
            &yellow_mutex,
            K_FOREVER
        );


        // Wait for dispatcher signal
        while (!yellow_requested) {

            k_condvar_wait(
                &yellow_signal,
                &yellow_mutex,
                K_FOREVER
            );
        }


        yellow_requested = false;


        k_mutex_unlock(
            &yellow_mutex
        );


        // Yellow = red + green
        gpio_pin_set_dt(&red, 1);
        gpio_pin_set_dt(&green, 1);

        printk("YELLOW ON\n");


        k_sleep(K_SECONDS(1));


        // Yellow OFF
        gpio_pin_set_dt(&red, 0);
        gpio_pin_set_dt(&green, 0);

        printk("YELLOW OFF\n");


        // Tell dispatcher that task is finished
        k_sem_give(
            &release_sem
        );
    }
}


void green_led_task(void *, void *, void *)
{
    printk("Green LED thread started\n");


    while (true) {

        k_mutex_lock(
            &green_mutex,
            K_FOREVER
        );


        // Wait for dispatcher signal
        while (!green_requested) {

            k_condvar_wait(
                &green_signal,
                &green_mutex,
                K_FOREVER
            );
        }


        green_requested = false;


        k_mutex_unlock(
            &green_mutex
        );


        // Green ON
        gpio_pin_set_dt(&green, 1);

        printk("GREEN ON\n");


        k_sleep(K_SECONDS(1));


        // Green OFF
        gpio_pin_set_dt(&green, 0);

        printk("GREEN OFF\n");


        // Tell dispatcher that task is finished
        k_sem_give(
            &release_sem
        );
    }
}