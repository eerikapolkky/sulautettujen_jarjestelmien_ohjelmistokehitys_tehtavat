// 3 pistettä: RYG-sekvenssin taskikohtaiset suoritusajat ja kokonaisaika mikrosekunteina, mittaukset debug päällä/pois. 
// Erillinen matalan prioriteetin debug-task ja FIFO debug-viesteille. UART-komento D vaihtaa debug-tulostukset päälle/pois.


#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>


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


// DISPATCHER FIFO

// UART task puts R/Y/G commands here.
// Dispatcher reads them.
K_FIFO_DEFINE(dispatcher_fifo);

struct data_t {
    void *fifo_reserved;
    char color;
};


// DEBUG FIFO

// Other tasks put debug messages here.
// Only debug_task prints them to console.
K_FIFO_DEFINE(debug_fifo);

struct debug_data_t {
    void *fifo_reserved;
    char msg[64];
};


bool debug_enabled = true;

uint64_t red_time_us = 0;
uint64_t yellow_time_us = 0;
uint64_t green_time_us = 0;


// RED
K_MUTEX_DEFINE(red_mutex);
K_CONDVAR_DEFINE(red_signal);

// YELLOW
K_MUTEX_DEFINE(yellow_mutex);
K_CONDVAR_DEFINE(yellow_signal);

// GREEN
K_MUTEX_DEFINE(green_mutex);
K_CONDVAR_DEFINE(green_signal);


// These tell the light tasks that dispatcher requested them.
bool red_requested = false;
bool yellow_requested = false;
bool green_requested = false;


// Dispatcher waits for the current light task to finish
// before processing the next color.
K_SEM_DEFINE(release_sem, 0, 1);


// FUNCTION PROTOTYPES
int init_led(void);
int init_uart(void);

void send_debug(const char *msg);

void uart_task(void *, void *, void *);
void dispatcher_task(void *, void *, void *);

void red_led_task(void *, void *, void *);
void yellow_led_task(void *, void *, void *);
void green_led_task(void *, void *, void *);

void debug_task(void *, void *, void *);



// THREADS

#define STACKSIZE 1024
#define PRIORITY 5
#define DEBUG_PRIORITY 7


// UART thread
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


// Dispatcher thread
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


// Red light thread
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


// Yellow light thread
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


// Green light thread
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


// Debug thread
// Priority 7 = lower priority than priority 5 threads.
K_THREAD_DEFINE(
    debug_thread,
    STACKSIZE,
    debug_task,
    NULL,
    NULL,
    NULL,
    DEBUG_PRIORITY,
    0,
    0
);


// MAIN


int main(void)
{
    int ret;

    ret = init_led();

    if (ret < 0) {
        printk("LED initialization failed\n");
        return 0;
    }


    ret = init_uart();

    if (ret < 0) {
        printk("UART initialization failed\n");
        return 0;
    }


    printk("Main started\n");
    printk("Commands: R = red, Y = yellow, G = green\n");
    printk("D = debug ON/OFF\n");


    while (true) {
        k_msleep(1000);
    }


    return 0;
}


int init_led(void)
{
    int ret;


    ret = gpio_pin_configure_dt(
        &red,
        GPIO_OUTPUT_ACTIVE
    );

    if (ret < 0) {
        printk("Error: Red LED configure failed\n");
        return ret;
    }


    ret = gpio_pin_configure_dt(
        &green,
        GPIO_OUTPUT_ACTIVE
    );

    if (ret < 0) {
        printk("Error: Green LED configure failed\n");
        return ret;
    }


    ret = gpio_pin_configure_dt(
        &blue,
        GPIO_OUTPUT_ACTIVE
    );

    if (ret < 0) {
        printk("Error: Blue LED configure failed\n");
        return ret;
    }


    gpio_pin_set_dt(&red, 0);
    gpio_pin_set_dt(&green, 0);
    gpio_pin_set_dt(&blue, 0);


    printk("LEDs initialized OK\n");

    return 0;
}


int init_uart(void)
{
    if (!device_is_ready(uart_dev)) {

        printk("UART device not ready\n");

        return -1;
    }


    printk("UART initialized OK\n");

    return 0;
}


// SEND DEBUG MESSAGE

void send_debug(const char *msg)
{
    // If debugging is disabled, do nothing.
    if (!debug_enabled) {
        return;
    }


    // Allocate memory for one debug message.
    struct debug_data_t *buf =
        k_malloc(sizeof(struct debug_data_t));


    if (buf == NULL) {
        return;
    }


    // Copy the message to the FIFO item.
    snprintf(
        buf->msg,
        sizeof(buf->msg),
        "%s",
        msg
    );


    // Put the message into debug FIFO.
    k_fifo_put(
        &debug_fifo,
        buf
    );
}


// DEBUG TASK

void debug_task(void *, void *, void *)
{
    while (true) {

        // Wait until a debug message arrives.
        struct debug_data_t *item =
            k_fifo_get(
                &debug_fifo,
                K_FOREVER
            );


        // ONLY this task prints normal debug messages.
        printk("%s\n", item->msg);


        // Free the memory allocated in send_debug().
        k_free(item);
    }
}


// UART TASK

void uart_task(void *, void *, void *)
{
    char rc;


    while (true) {

        // Check whether a character has arrived.
        if (uart_poll_in(uart_dev, &rc) == 0) {

            // D = toggle debug ON/OFF

            if (rc == 'D') {

                debug_enabled = !debug_enabled;


                // This is printed directly so that
                // user always sees whether debug is
                // now ON or OFF.
                if (debug_enabled) {

                    printk("DEBUG ON\n");

                } else {

                    printk("DEBUG OFF\n");
                }
            }

            // R / Y / G = light command

            else if (
                rc == 'R' ||
                rc == 'Y' ||
                rc == 'G'
            ) {

                // Create FIFO item.
                struct data_t *buf =
                    k_malloc(sizeof(struct data_t));


                if (buf == NULL) {

                    printk(
                        "Memory allocation failed\n"
                    );

                } else {

                    // Save received color.
                    buf->color = rc;


                    // Put color into dispatcher FIFO.
                    k_fifo_put(
                        &dispatcher_fifo,
                        buf
                    );
                }
            }
        }


        k_msleep(10);
    }
}


// DISPATCHER TASK

void dispatcher_task(void *, void *, void *)
{
    char msg[64];


    while (true) {

        // Wait until UART task puts a color into FIFO.
        struct data_t *item =
            k_fifo_get(
                &dispatcher_fifo,
                K_FOREVER
            );


        // Read the color
        char color = item->color;


        // FIFO item is no longer needed
        k_free(item);


        // Send debug information through debug FIFO.
        snprintf(
            msg,
            sizeof(msg),
            "Dispatcher received: %c",
            color
        );

        send_debug(msg);

        // RED

        if (color == 'R') {

            send_debug("Dispatcher -> RED");


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

            send_debug("Dispatcher -> YELLOW");


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

            send_debug("Dispatcher -> GREEN");


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


        // Wait until the selected light task finishes.
        k_sem_take(
            &release_sem,
            K_FOREVER
        );
    }
}

// RED TASK

void red_led_task(void *, void *, void *)
{
    while (true) {

        // Lock mutex before checking the condition.
        k_mutex_lock(
            &red_mutex,
            K_FOREVER
        );


        // Wait until dispatcher requests RED.
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

        // START TIMING

        uint32_t start_time =
            k_cycle_get_32();


        // Turn red light ON.
        gpio_pin_set_dt(
            &red,
            1
        );


        send_debug("RED ON");


        // Keep light on for one second.
        k_sleep(
            K_SECONDS(1)
        );


        // Turn red light OFF.
        gpio_pin_set_dt(
            &red,
            0
        );


        send_debug("RED OFF");


        // STOP TIMING

        uint32_t end_time =
            k_cycle_get_32();


        red_time_us =
            k_cyc_to_us_floor64(
                end_time - start_time
            );


        // We want to see this even when debug_enabled == false.
        printk(
            "RED task time: %llu us\n",
            red_time_us
        );


        // Tell dispatcher that RED is finished.
        k_sem_give(
            &release_sem
        );
    }
}

// YELLOW TASK
void yellow_led_task(void *, void *, void *)
{
    while (true) {

        k_mutex_lock(
            &yellow_mutex,
            K_FOREVER
        );


        // Wait until dispatcher requests YELLOW.
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

        // START TIMING

        uint32_t start_time =
            k_cycle_get_32();


        // Yellow = red + green.
        gpio_pin_set_dt(
            &red,
            1
        );

        gpio_pin_set_dt(
            &green,
            1
        );


        send_debug("YELLOW ON");


        k_sleep(
            K_SECONDS(1)
        );


        // Turn yellow OFF.
        gpio_pin_set_dt(
            &red,
            0
        );

        gpio_pin_set_dt(
            &green,
            0
        );


        send_debug("YELLOW OFF");


        // STOP TIMING

        uint32_t end_time =
            k_cycle_get_32();


        yellow_time_us =
            k_cyc_to_us_floor64(
                end_time - start_time
            );


        printk(
            "YELLOW task time: %llu us\n",
            yellow_time_us
        );


        // Tell dispatcher that YELLOW is finished.
        k_sem_give(
            &release_sem
        );
    }
}

// GREEN TASK

void green_led_task(void *, void *, void *)
{
    while (true) {

        k_mutex_lock(
            &green_mutex,
            K_FOREVER
        );


        // Wait until dispatcher requests GREEN.
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

        // START TIMING

        uint32_t start_time =
            k_cycle_get_32();


        // Turn green light ON.
        gpio_pin_set_dt(
            &green,
            1
        );


        send_debug("GREEN ON");


        k_sleep(
            K_SECONDS(1)
        );


        // Turn green light OFF.
        gpio_pin_set_dt(
            &green,
            0
        );


        send_debug("GREEN OFF");


        // STOP TIMING
        uint32_t end_time =
            k_cycle_get_32();


        green_time_us =
            k_cyc_to_us_floor64(
                end_time - start_time
            );


        printk(
            "GREEN task time: %llu us\n",
            green_time_us
        );


        // RYG TOTAL TIME
        uint64_t total_time =
            red_time_us +
            yellow_time_us +
            green_time_us;


        printk(
            "RYG total time: %llu us\n",
            total_time
        );


        // Tell dispatcher that GREEN is finished.
        k_sem_give(
            &release_sem
        );
    }
}
