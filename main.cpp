//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  A copy of the GNU General Public License is available at <http://www.gnu.org/licenses/>.
//

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
#include "Led.cpp"
#endif

char debug_file[] = "/tmp/mav_to_ltm.log";
int input_baud = 57600;
int ltm_delay = 50;

#ifdef INCLUDE_LED
int led_delay = 10;
LedController* led_strip_ptr;
#endif

void setup() {
    if(debug)
    {
        fp_out=fopen(debug_file, "w+");
        fputs("Setup start!\n", fp_out);
        fflush(fp_out);
    }
    if((fd = serialOpen ("/dev/ttyAMA0", input_baud)) < 0 )
         perror("device not opened \n");

    if ( wiringPiSetup () < 0 )
        perror("WiringPiSetup problem \n ");

    if (!PASSIVEMODE) {
      enable_frame_request = 1;
      messageCounter = 50;
    }

#ifdef INCLUDE_LED
    ws2811_init(&ledstring);

    for (int x = 0; x < LED_MAX_BULBS; x++)
        {
            for (int y = 0; y < LED_MAX_STRIPS; y++)
            {
                ledstring.channel[0].leds[(y * 8) + x] = 0x00000;
            }
        }
    ws2811_render(&ledstring);

    led_strip_ptr = new LedController();
#endif

    if(debug)
    {
        fputs("Setup done!\n", fp_out);
        fflush(fp_out);
    }

}

#ifdef INCLUDE_LED
void leds_render(bool show)
{
    for (int x = 0; x < LED_MAX_BULBS; x++)
    {
        for (int y = 0; y < LED_MAX_STRIPS; y++)
        {
            ledstring.channel[0].leds[(y * LED_MAX_BULBS) + x] = leds[x][y];
        }
    }
   if(show)
   {
     if (ws2811_render(&ledstring))
     {

     }
   }
}
#endif

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
        led_strip_ptr->rc8 = uav_rc10_raw;
        led_strip_ptr->climb_rate = uav_climb_rate;
        led_strip_ptr->custom_mode = uav_flightmode;
        led_strip_ptr->process_10_millisecond();
        delay(led_delay);
    }
}
#endif

int main(int argc, char* argv[])
{
    int ret = 0;

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

    ws2811_fini(&ledstring);

    serialClose(fd);

    if(debug)
        fclose(fp_out);

    return ret;
}
