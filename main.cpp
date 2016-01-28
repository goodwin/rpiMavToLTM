#include <wiringPi.h>
#include <wiringSerial.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdint.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "main.h"

#include <mavlink.h>
#include <mavlink_types.h>
//#include <AP_Common.h>
//#include <GCS_MAVLink.h>
#include "MavLink.cpp"
#include "LightTelemetry.cpp"

#ifdef INCLUDE_LED
#include "ws2811.h"
#endif

#define ARRAY_SIZE(stuff)                        (sizeof(stuff) / sizeof(stuff[0]))

#define TARGET_FREQ                              WS2811_TARGET_FREQ
#define GPIO_PIN                                 18
#define DMA                                      5

#define WIDTH                                    32
#define HEIGHT                                   1
#define LED_COUNT                                (WIDTH * HEIGHT)

char debug_file[] = "/tmp/mav_to_ltm.log";
int input_baud = 115200;
int ltm_delay = 50;
int led_delay = 200;

ws2811_t ledstring =
{
    nullptr,
    nullptr,
    TARGET_FREQ, //freq
    DMA, //dmanum
    { //channel
        {
            GPIO_PIN, //gpionum
            LED_COUNT, //count
            0, //invert
            255, //brightness
        },
        {
            0,
            0,
            0,
            0,
        }
    }
};

ws2811_led_t matrix[WIDTH][HEIGHT];

void setup() {
    if((fd = serialOpen ("/dev/ttyAMA0", input_baud)) < 0 )
         perror("device not opened \n");

    if ( wiringPiSetup () < 0 )
        perror("WiringPiSetup problem \n ");

    if (!PASSIVEMODE) {
      enable_frame_request = 1;
      messageCounter = 50;
    }
    set_port(fd);
    if(debug)
    {
        fp_out=fopen(debug_file, "w+");
        fputs("Setup done!\n", fp_out);
        fflush(fp_out);
    }
}

extern "C" void* mav(void* argA)
{
    while(1)
    {
//        printf("read mavlink\n");
        read_mavlink();
    }
}

extern "C" void* ltm(void* argB)
{
    while(1)
    {
//        printf("send ltm\n");
        if(debug)
        {
            fprintf(fp_out, "%i, %i, %i, %i, %i\n", uav_roll, uav_pitch, uav_bat, uav_flightmode, uav_arm);
            fprintf(fp_out, "%li, %li, %li, %li\n", ltm_Aframe_chg, ltm_Sframe_chg, ltm_Aframe_snd, ltm_Sframe_snd);
            fflush(fp_out);
        }
        send_LTM();
        if(ltm_mode != 2)
            delay(ltm_delay);
    }
}

#ifdef INCLUDE_LED
extern "C" void* led(void* argC)
{
    while(1)
    {
//        printf("send ltm\n");
//printf("%i, %i, %i, %i, %i\n", uav_roll, uav_pitch, uav_bat, uav_flightmode, uav_arm);
//        set_led();
        delay(led_delay);
    }
}
#endif

#ifdef INCLUDE_LED
void matrix_render(void)
{
    int x, y;

    for (x = 0; x < WIDTH; x++)
    {
        for (y = 0; y < HEIGHT; y++)
        {
            ledstring.channel[0].leds[(y * WIDTH) + x] = matrix[x][y];
        }
    }
}
#endif

int main(int argc, char* argv[])
{
    int ret = 0;

#ifdef INCLUDE_LED
    if (ws2811_init(&ledstring))
    {
        return -1;
    }
#endif

    pthread_t thread1, thread2, thread3;
    setup();
    pthread_create(&thread1, NULL, mav, NULL );
    pthread_create(&thread2, NULL, ltm, NULL );
#ifdef INCLUDE_LED
    pthread_create(&thread3, NULL, led, NULL );
#endif
     if(debug)
    {
        fputs("Threads created\n", fp_out);
        fflush(fp_out);
    }
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
#ifdef INCLUDE_LED
    pthread_join(thread3, NULL);
#endif
    for (;;) {
      delay(1);
    }

    serialClose(fd);

    if(debug)
        fclose(fp_out);

    return ret;
}
