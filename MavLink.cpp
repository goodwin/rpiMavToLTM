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

//#include "../mavlink/include/mavlink.h"
//#include "./GCS_MAVLink/include/mavlink/v1.0/mavlink_types.h"
//#include "./GCS_MAVLink/include/mavlink/v1.0/ardupilotmega/mavlink.h"

#define START_MAVLINK_PACKETS       1

// true when we have received at least 1 MAVLink packet
//int messageCounter;
static bool mavlink_active;
static uint8_t crlf_count = 0;
static bool mavbeat = 0;
static uint8_t apm_mav_type;

static int packet_drops = 0;
static int parse_error = 0;
mavlink_message_t msg; 
mavlink_status_t status;

void start_mavlink_packet_type(mavlink_message_t* msg_ptr, uint8_t stream_id, uint16_t rate) {
  uint16_t byte_length;
  uint8_t buf[MAVLINK_MAX_PACKET_LEN];

  mavlink_msg_request_data_stream_pack(0xFF, 0xBE, msg_ptr, 1, 1, stream_id, rate, START_MAVLINK_PACKETS);
  byte_length = mavlink_msg_to_send_buffer(buf, msg_ptr);
  for(int i = 0; i < byte_length; i++)
  {
    serialPutchar(fd, buf[i]);
  }
  delay(10);
}

void request_mavlink_rates(mavlink_message_t* msg_ptr)
{
   #ifdef DEBUG
//     Serial.println("MavRatesRequest");
   #endif
  start_mavlink_packet_type(msg_ptr, MAV_DATA_STREAM_RAW_SENSORS, 3);
  start_mavlink_packet_type(msg_ptr, MAV_DATA_STREAM_EXTENDED_STATUS, 3);
  start_mavlink_packet_type(msg_ptr, MAV_DATA_STREAM_RAW_CONTROLLER, 0);
  start_mavlink_packet_type(msg_ptr, MAV_DATA_STREAM_POSITION, 5);
  start_mavlink_packet_type(msg_ptr, MAV_DATA_STREAM_EXTRA1, 5);
  start_mavlink_packet_type(msg_ptr, MAV_DATA_STREAM_EXTRA2, 2);
  start_mavlink_packet_type(msg_ptr, MAV_DATA_STREAM_RAW_SENSORS, 3);
}

void read_mavlink(){
    mavlink_message_t msg; 
    mavlink_status_t status;
    messageCounter++;

    if(messageCounter > 500) messageCounter = 51;

    if (!PASSIVEMODE) {
      if( (messageCounter >= 20 && mavlink_active) || enable_frame_request ) {
        request_mavlink_rates(&msg);
        enable_frame_request = 0;
      }
    }

    //grabing data 
    while(serialDataAvail(fd) > 0) { 
        uint8_t c = serialGetchar(fd);

        //trying to grab msg  
        //if(mavlink_parse_char(MAVLINK_COMM_0, c, &msg, &status)) {
        if(mavlink_parse_char(MAVLINK_COMM_0, c, &msg, &status)) {
            messageCounter = 0;
            mavlink_active = 1;
            lastpacketreceived = millis();
            //handle msg
            switch(msg.msgid) {
            case MAVLINK_MSG_ID_HEARTBEAT:
                {
                    mavbeat = 1;
                    //apm_mav_system    = msg.sysid;
                    //apm_mav_component = msg.compid;
   // TODO there's different flightmodes value depending of arducopter or arduplane. 
   // Need to check first vehicle type, then we will apply correct flightmode map.
   // for now only arducopter is supported.
                    apm_mav_type      = mavlink_msg_heartbeat_get_type(&msg);
                    uav_flightmode = (uint8_t)mavlink_msg_heartbeat_get_custom_mode(&msg);
                    if (apm_mav_type == 2)    //ArduCopter MultiRotor or ArduCopter Heli
                    {                         
                        switch (uav_flightmode) {
                            case 0: uav_flightmode = 2;   break;      //Stabilize 
                            case 1: uav_flightmode = 1;   break;      //Acro 
                            case 2: uav_flightmode = 8;   break;      //Alt Hold
                            case 3: uav_flightmode = 10;  break;      //Auto
                            case 4: uav_flightmode = 10;  break;      //Guided -> Auto
                            case 5: uav_flightmode = 9;   break;      //Loiter
                            case 6: uav_flightmode = 13;  break;      //RTL
                            case 7: uav_flightmode = 12;  break;      //Circle
                            case 8: uav_flightmode = 9 ;  break;      //Position -> Loiter
                            case 9: uav_flightmode = 15;  break;      //Land
                            case 10: uav_flightmode = 9;  break;      //OF LOiter
                            case 11: uav_flightmode = 5;  break;      //Drift -> Stabilize1
                            case 12: uav_flightmode = 6;  break;      //Sport -> Stabilize2
                            default: uav_flightmode = 19; break;      //Unknown                      
                        }
                    }
                    else if (apm_mav_type == 1)    //ArduPlane
                    {
                        switch (uav_flightmode) {
                            case 0:                       break;       //Manual
                            case 1:  uav_flightmode = 12; break;       //Circle
                            case 2:                       break;       //Stabilize
                            case 5:  uav_flightmode = 16; break;       //FlyByWire A
                            case 6:  uav_flightmode = 17; break;       //FlyByWire B
                            case 10:                      break;       //Auto
                            case 11: uav_flightmode = 13; break;       //RTH
                            case 12: uav_flightmode = 9;  break;       //Loiter
                            default: uav_flightmode = 19; break;       //Unknown
                        }
                    }
                    //Mode (arducoper armed/disarmed)
                    uav_arm = mavlink_msg_heartbeat_get_base_mode(&msg);
                    if (getBit(uav_arm,7)) 
                        uav_arm = 1;
                    else 
                        uav_arm = 0;
                    if(ltm_mode == 2)
                        ltm_Sframe_chg = millis();
                }
                break;
            case MAVLINK_MSG_ID_SYS_STATUS:
                {
                    uav_bat = mavlink_msg_sys_status_get_voltage_battery(&msg);
                    uav_current = mavlink_msg_sys_status_get_current_battery(&  msg)/10;  
                    uav_amp = (int16_t)mavlink_msg_sys_status_get_battery_remaining(  &msg); //Mavlink send battery remaining % , will use this instead
                    if(ltm_mode == 2)
                        ltm_Sframe_chg = millis();
                }
                break;

            case MAVLINK_MSG_ID_GPS_RAW_INT:
                {
                    uav_lat =  mavlink_msg_gps_raw_int_get_lat(&msg) ;
                    uav_lon =  mavlink_msg_gps_raw_int_get_lon(&msg);
                    uav_alt = (int32_t)round(mavlink_msg_gps_raw_int_get_alt(&msg)/10.0f); // from mm to cm
                    uav_fix_type = (uint8_t) mavlink_msg_gps_raw_int_get_fix_type(&msg);
                    uav_satellites_visible = (uint8_t) mavlink_msg_gps_raw_int_get_satellites_visible(&msg);
                    uav_hdop = (uint16_t)mavlink_msg_gps_raw_int_get_eph(&msg);
                    uav_gpsheading = (int16_t) mavlink_msg_gps_raw_int_get_cog(&msg);
                    if(ltm_mode == 2)
                        ltm_Gframe_chg = millis();
                    if(ltm_mode == 2)
                        ltm_Cframe_chg = millis();
                }
                break; 
            case MAVLINK_MSG_ID_VFR_HUD:
                {
                    uav_groundspeed = (int)round(mavlink_msg_vfr_hud_get_groundspeed(&msg));
                    uav_airspeed = (uint8_t)round(mavlink_msg_vfr_hud_get_airspeed(&msg));
                    uav_alt = (int32_t)round(mavlink_msg_vfr_hud_get_alt(&msg) * 100.0f);  // from m to cm
                    uav_climb_rate = mavlink_msg_vfr_hud_get_climb(&msg); 
                    uav_throttle = mavlink_msg_vfr_hud_get_throttle(&msg); 
                    if(ltm_mode == 2)
                    {
                        ltm_Sframe_chg = millis();
                        ltm_Gframe_chg = millis();
                    }
                }
                break;
            case MAVLINK_MSG_ID_ATTITUDE:
                {
                    uav_roll = (int16_t)round(toDeg(mavlink_msg_attitude_get_roll(&msg)));
                    uav_pitch = (int16_t)round(toDeg(mavlink_msg_attitude_get_pitch(&msg)));
                    uav_heading = (int16_t)round(toDeg(mavlink_msg_attitude_get_yaw(&msg)));
                    if(ltm_mode == 2)
                        ltm_Aframe_chg = millis();
                    //if (uav_heading >= 180 ) uav_heading = -360+uav_heading; //convert from 0-360 to -180/180°
                }
                break;

            case MAVLINK_MSG_ID_RC_CHANNELS_RAW:
                {

                   uav_rssi      = mavlink_msg_rc_channels_raw_get_rssi(&msg);
                   uav_rc1_raw = mavlink_msg_rc_channels_raw_get_chan1_raw(&msg);
                   uav_rc2_raw = mavlink_msg_rc_channels_raw_get_chan2_raw(&msg);
                   uav_rc3_raw = mavlink_msg_rc_channels_raw_get_chan3_raw(&msg);
                   uav_rc4_raw = mavlink_msg_rc_channels_raw_get_chan4_raw(&msg);
                   uav_rc5_raw = mavlink_msg_rc_channels_raw_get_chan5_raw(&msg);
                   uav_rc6_raw = mavlink_msg_rc_channels_raw_get_chan6_raw(&msg);
                   uav_rc7_raw = mavlink_msg_rc_channels_raw_get_chan7_raw(&msg);
                   uav_rc8_raw = mavlink_msg_rc_channels_raw_get_chan8_raw(&msg);
                    if(ltm_mode == 2)
                        ltm_Sframe_chg = millis();
                }
                break;
 
            case MAVLINK_MSG_ID_RC_CHANNELS:
                {
                   uav_rc9_raw = mavlink_msg_rc_channels_get_chan9_raw(&msg);
                   uav_rc10_raw = mavlink_msg_rc_channels_get_chan10_raw(&msg);
                   uav_rc11_raw = mavlink_msg_rc_channels_get_chan11_raw(&msg);
                   uav_rc12_raw = mavlink_msg_rc_channels_get_chan12_raw(&msg);
//                   if (uav_rc9_raw >= UINT16_MAX)  {uav_rc9_raw = -1;}
//                   if (uav_rc10_raw >= UINT16_MAX) {uav_rc10_raw = -1;}
//                   if (uav_rc11_raw >= UINT16_MAX) {uav_rc11_raw = -1;}
//                   if (uav_rc12_raw >= UINT16_MAX) {uav_rc12_raw = -1;}
                    if(ltm_mode == 2)
                        ltm_Cframe_chg = millis();
                }
                break;
            }
        }
        //delayMicroseconds(138);
    }
    // Update global packet drops counter
    packet_drops += status.packet_rx_drop_count;
    parse_error += status.parse_error;

}
