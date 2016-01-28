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

#define INCLUDE_LED
#define PASSIVEMODE 0
typedef uint8_t byte;

//int          softserial_delay = (int)round(10000000.0f/(OUTPUT_BAUD)); // time to wait between each byte sent.
//boolean      ltm_counter = false;
uint8_t      ltm_scheduler = 0;
int32_t      uav_lat = 0;                    // latitude
int32_t      uav_lon = 0;                    // longitude
uint8_t      uav_satellites_visible = 0;     // number of satelites
uint8_t      uav_fix_type = 0;               // GPS lock 0-1=no fix, 2=2D, 3=3D
int32_t      uav_alt = 0;                    // altitude (cm)
int          uav_groundspeed = 0;            // ground speed
int16_t      uav_pitch = 0;                  // attitude pitch
int16_t      uav_roll = 0;                   // attitude roll
int16_t      uav_heading = 0;                // attitude heading
float        uav_climb_rate = 0;                // attitude heading
uint16_t     uav_throttle = 0;                // attitude heading
int16_t      uav_gpsheading=0;               // gps heading
uint16_t     uav_bat = 0;                    // battery voltage (mv)
uint16_t     uav_amp = 0;                    // consumed mah.
uint16_t     uav_current = 0;                // actual current
uint8_t      uav_rssi = 0;                   // radio RSSI (%)
uint8_t      uav_linkquality = 0;            // radio link quality
uint8_t      uav_airspeed = 0;               // Airspeed sensor (m/s)
uint8_t      uav_arm = 0;                    // 0: disarmed, 1: armed
uint8_t      uav_failsafe = 0;               // 0: normal,   1: failsafe 
uint8_t      uav_flightmode = 16;            // Flight mode(0-19): 0: Manual, 1: Rate, 2: Attitude/Angle, 3: Horizon, 4: Acro, 5: Stabilized1, 6: Stabilized2, 7: Stabilized3,
                                             // 8: Altitude Hold, 9: Loiter/GPS Hold, 10: Auto/Waypoints, 11: Heading Hold / headFree, 12: Circle, 13: RTH, 14: FollowMe, 15: LAND, 
                                             // 16:FlybyWireA, 17: FlybywireB, 18: Cruise, 19: Unknown
time_t ltm_Aframe_chg = 0;
time_t ltm_Gframe_chg = 0;
time_t ltm_Sframe_chg = 0;
time_t ltm_Cframe_chg = 0;
time_t ltm_Aframe_snd = 0;
time_t ltm_Gframe_snd = 0;
time_t ltm_Sframe_snd = 0;
time_t ltm_Cframe_snd = 0;
uint8_t ltm_mode = 2;

int fd;
FILE * fp_out;
bool debug = true;
/*
uint16_t uav_rc1_raw = 0;
uint16_t uav_rc2_raw = 0;
uint16_t uav_rc3_raw = 0;
uint16_t uav_rc4_raw = 0;
uint16_t uav_rc5_raw = 0;
uint16_t uav_rc6_raw = 0;
uint16_t uav_rc7_raw = 0;
uint16_t uav_rc8_raw = 0;
*/
uint16_t uav_rc9_raw = 0;
uint16_t uav_rc10_raw = 0;
uint16_t uav_rc11_raw = 0;
uint16_t uav_rc12_raw = 0;

long lastpacketreceived;
static bool      enable_frame_request = 0;

bool getBit(byte Reg, byte whichBit) {
    bool State;
    State = Reg & (1 << whichBit);
    return State;
}

byte setBit(byte &Reg, byte whichBit, bool stat) {
    if (stat) {
        Reg = Reg | (1 << whichBit);
    } 
    else {
        Reg = Reg & ~(1 << whichBit);
    }
    return Reg;
}

float toRad(float angle) {
// convert degrees to radians
	angle = angle*0.01745329; // (angle/180)*pi
	return angle;
}

float toDeg(float angle) {
// convert radians to degrees.
	angle = angle*57.29577951;   // (angle*180)/pi
        return angle;
}

uint16_t pwm_range = 0;
int brightness = 100;
int real_brightness = 250;
int messageCounter = 0;
