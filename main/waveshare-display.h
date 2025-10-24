// this is written in C++, so we have to add this stuff to make it callable from C:
#ifdef __cplusplus
extern "C" {
#endif

    void doLvglInit(void);
    void processDisplay(void);

#ifdef __cplusplus
}
#endif

// backlight control
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          (GPIO_NUM_4) // Define the output GPIO
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY_0               (0) // Set duty to 0%. (2 ** 13) * 0% = 0
#define LEDC_DUTY_10               (1023) // Set duty to 0%. (2 ** 13) * 10% = 1024
#define LEDC_DUTY_25               (2047) // Set duty to 0%. (2 ** 13) * 25% = 4069
#define LEDC_DUTY_50               (4095) // Set duty to 50%. (2 ** 13) * 50% = 4096
#define LEDC_DUTY_100               (8191) // Set duty to 100%. (2 ** 13 - 1) = 8191, app note says dont use 8192
#define LEDC_FREQUENCY          (8000) // Frequency in Hertz. Set frequency at 4 kHz
