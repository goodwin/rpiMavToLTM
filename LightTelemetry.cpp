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

/* #################################################################################################################
 * LightTelemetry protocol (LTM)
 *
 * Ghettostation one way telemetry protocol for really low bitrates (1200/2400 bauds). 
 *			   
 * Protocol details: 3 different frames, little endian.
 *   G Frame (GPS position) (2hz @ 1200 bauds , 5hz >= 2400 bauds): 18BYTES
 *    0x24 0x54 0x47 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF  0xFF   0xC0   
 *     $     T    G  --------LAT-------- -------LON---------  SPD --------ALT-------- SAT/FIX  CRC
 *   A Frame (Attitude) (5hz @ 1200bauds , 10hz >= 2400bauds): 10BYTES
 *     0x24 0x54 0x41 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0xC0   
 *      $     T   A   --PITCH-- --ROLL--- -HEADING-  CRC
 *   S Frame (Sensors) (2hz @ 1200bauds, 5hz >= 2400bauds): 11BYTES
 *     0x24 0x54 0x53 0xFF 0xFF  0xFF 0xFF    0xFF    0xFF      0xFF       0xC0     
 *      $     T   S   VBAT(mv)  Current(ma)   RSSI  AIRSPEED  ARM/FS/FMOD   CRC
 * ################################################################################################################# */
#include "LightTelemetry.h"

static bool send_LTM_Packet(uint8_t *LTPacket, uint8_t LTPacket_size)
{
    //calculate Checksum
    uint8_t LTCrc = 0x00;
    int i;
    for (i = 3; i < LTPacket_size-1; i++) {
        LTCrc ^= LTPacket[i];
    }
    LTPacket[LTPacket_size-1]=LTCrc;
    bool byte_dropped = false;
    bool packet_dropped = false;
    uint32_t frame_timer = millis();

    for (i = 0; i<LTPacket_size; i++) {
        if(debug)
        {
            fprintf(fp_out, "%c\n", LTPacket[i]);
            fflush(fp_out);
            }
        if ( fprintf(stdout,"%c", LTPacket[i]) < 0 ) {
         //buffer is full, flush & retry.
            //SerialPort2.flush(); 
            fflush(stdout);
            byte_dropped = true;
            if (millis() - frame_timer >= 100) {
            // drop the whole frame, it's too old. Will resend a fresh one.
               packet_dropped = true;

               break;
            }

        }
        if (byte_dropped) {
            i--; //resend dropped byte  
            byte_dropped = false;
        }
        if (packet_dropped) {
            break;
        }
//        int32_t currentmicros = micros();
//        while ( (micros() - currentmicros) < softserial_delay ){
//            ;// wait at least 1 byte is sent
//        }
    }
    if (packet_dropped)
        return false;
    else
    {
        fflush(stdout);
        return true;
    }
}


// GPS frame
void send_LTM_Gframe()
{

    uint8_t LTBuff[LTM_GFRAME_SIZE];
    //protocol: START(2 bytes)FRAMEID(1byte)LAT(cm,4 bytes)LON(cm,4bytes)SPEED(m/s,1bytes)ALT(cm,4bytes)SATS(6bits)FIX(2bits)CRC(xor,1byte)
    //START
    LTBuff[0]=0x24; //$
    LTBuff[1]=0x54; //T
    //FRAMEID
    LTBuff[2]=0x47; // G ( gps frame at 5hz )
    //PAYLOAD
    LTBuff[3]=(uav_lat >> 8*0) & 0xFF;
    LTBuff[4]=(uav_lat >> 8*1) & 0xFF;
    LTBuff[5]=(uav_lat >> 8*2) & 0xFF;
    LTBuff[6]=(uav_lat >> 8*3) & 0xFF;
    LTBuff[7]=(uav_lon >> 8*0) & 0xFF;
    LTBuff[8]=(uav_lon >> 8*1) & 0xFF;
    LTBuff[9]=(uav_lon >> 8*2) & 0xFF;
    LTBuff[10]=(uav_lon >> 8*3) & 0xFF;
    LTBuff[11]=( (uint8_t) round( (uav_groundspeed * 1000) / 3600) >> 8*0) & 0xFF;
    LTBuff[12]=(uav_alt >> 8*0) & 0xFF;
    LTBuff[13]=(uav_alt >> 8*1) & 0xFF;
    LTBuff[14]=(uav_alt >> 8*2) & 0xFF;
    LTBuff[15]=(uav_alt >> 8*3) & 0xFF;
    LTBuff[16]= ((uav_satellites_visible << 2 )& 0xFF) | (uav_fix_type & 0b00000011) ; // last 6 bits: sats number, first 2:fix type (0,1,2,3)
    if (send_LTM_Packet(LTBuff,LTM_GFRAME_SIZE) == true) {
    ltm_scheduler++;
    }
}

//Sensors frame
static void send_LTM_Sframe() 
{
    uint8_t LTBuff[LTM_SFRAME_SIZE];
    
    //START
    LTBuff[0]=0x24; //$
    LTBuff[1]=0x54; //T
    //FRAMEID
    LTBuff[2]=0x53; //S 
    //PAYLOAD
    LTBuff[3]=(uav_bat >> 8*0) & 0xFF;                                                    //vbat converted in mv
    LTBuff[4]=(uav_bat >> 8*1) & 0xFF;
    LTBuff[5]=(uav_amp >> 8*0) & 0xFF;                                                    //consumed current in ma.
    LTBuff[6]=(uav_amp >> 8*1) & 0xFF;
    LTBuff[7]=(uav_rssi >> 8*0) & 0xFF;                                                   
    LTBuff[8]=(uav_airspeed >> 8*0) & 0xFF;                                               // no airspeed in multiwii/baseflight
    LTBuff[9]= ((uav_flightmode << 2)& 0xFF ) | ((uav_failsafe << 1)& 0b00000010 ) | (uav_arm & 0b00000001) ; // last 6 bits: flight mode, 2nd bit: failsafe, 1st bit: Arm status.
    // Flight mode(0-19): 0: Manual, 1: Rate, 2: Attitude/Angle, 3: Horizon, 4: Acro, 5: Stabilized1, 6: Stabilized2, 7: Stabilized3,
    // 8: Altitude Hold, 9: Loiter/GPS Hold, 10: Auto/Waypoints, 11: Heading Hold / headFree, 
    // 12: Circle, 13: RTH, 14: FollowMe, 15: LAND, 16:FlybyWireA, 17: FlybywireB, 18: Cruise, 19: Unknown
    if (send_LTM_Packet(LTBuff,LTM_SFRAME_SIZE) == true) {
    ltm_scheduler++;
    }
}

// Attitude frame
static void send_LTM_Aframe() 
{
    uint8_t LTBuff[LTM_AFRAME_SIZE];
    
    //A Frame: $T(2 bytes)A(1byte)PITCH(2 bytes)ROLL(2bytes)HEADING(2bytes)CRC(xor,1byte)
    //START
    LTBuff[0]=0x24; //$
    LTBuff[1]=0x54; //T
    //FRAMEID
    LTBuff[2]=0x41; //A 
    //PAYLOAD
    LTBuff[3]=(uav_pitch >> 8*0) & 0xFF;
    LTBuff[4]=(uav_pitch >> 8*1) & 0xFF;
    LTBuff[5]=(uav_roll >> 8*0) & 0xFF;
    LTBuff[6]=(uav_roll >> 8*1) & 0xFF;
    LTBuff[7]=(uav_heading >> 8*0) & 0xFF;
    LTBuff[8]=(uav_heading >> 8*1) & 0xFF;
    if (send_LTM_Packet(LTBuff,LTM_AFRAME_SIZE) == true) {
    ltm_scheduler++;
    }
}

static void send_LTM_Cframe() 
{

    uint8_t LTBuff[LTM_CFRAME_SIZE];
    
    //A Frame: $T(2 bytes)C(1byte)uav_satellites_visible(1 byte)uav_fix_type(1byte)uav_hdop(2bytes)uav_rc9_raw(2bytes)uav_rc10_raw(2bytes)uav_rc11_raw(2bytes)uav_rc12_raw(2bytes)
    //START
    LTBuff[0]=0x24; //$
    LTBuff[1]=0x54; //T
    //FRAMEID
    LTBuff[2]=0x43; //C
    //PAYLOAD

    LTBuff[3]=(uav_satellites_visible >> 8*0) & 0xFF;
    LTBuff[4]=(uav_fix_type >> 8*0) & 0xFF;
    LTBuff[5]=(uav_hdop >> 8*0) & 0xFF;
    LTBuff[6]=(uav_hdop >> 8*1) & 0xFF;
    LTBuff[7]=(uav_rc9_raw >> 8*0) & 0xFF;
    LTBuff[8]=(uav_rc9_raw >> 8*1) & 0xFF;
    LTBuff[9]=(uav_rc10_raw >> 8*0) & 0xFF;
    LTBuff[10]=(uav_rc10_raw >> 8*1) & 0xFF;
    LTBuff[11]=(uav_rc11_raw >> 8*0) & 0xFF;
    LTBuff[12]=(uav_rc11_raw >> 8*1) & 0xFF;
    LTBuff[13]=(uav_rc12_raw >> 8*0) & 0xFF;
    LTBuff[14]=(uav_rc12_raw >> 8*1) & 0xFF;
    if (send_LTM_Packet(LTBuff,LTM_CFRAME_SIZE) == true) {
    ltm_scheduler++;
    }

}

static void send_LTM() {
        if (ltm_mode == 1)
        {
            if(debug)
            {
                fputs("Sending LTM mode 1\n", fp_out);
                fflush(fp_out);
            }
            if (ltm_scheduler & 1) {    // is odd
                send_LTM_Aframe();
            }
            else                        // is even
            {
                if (ltm_scheduler % 4 == 0) send_LTM_Sframe();
                else send_LTM_Gframe();
            }
        }
        else if (ltm_mode == 2)
        {
            if(debug)
            {
                fputs("Sending LTM mode 2\n", fp_out);
                fflush(fp_out);
            }
            if (ltm_Aframe_chg > ltm_Aframe_snd)
            {
                send_LTM_Aframe();
                ltm_Aframe_snd = ltm_Aframe_chg;
            }
            if (ltm_Sframe_chg > ltm_Sframe_snd)
            {
                send_LTM_Sframe();
                ltm_Sframe_snd = ltm_Sframe_chg;
            }
            if (ltm_Gframe_chg > ltm_Gframe_snd)
            {
                send_LTM_Gframe();
                ltm_Gframe_snd = ltm_Gframe_chg;
            }
            if (ltm_Cframe_chg > ltm_Cframe_snd)
            {
                send_LTM_Cframe();
                ltm_Cframe_snd = ltm_Cframe_chg;
            }
        }
        //ltm_scheduler++; 
        if (ltm_scheduler > 9 )
        ltm_scheduler = 1;
}
