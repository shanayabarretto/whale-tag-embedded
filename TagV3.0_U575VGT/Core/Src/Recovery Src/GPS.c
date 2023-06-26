/*
 * GPS.c
 *
 *  Created on: Jun 22, 2023
 *      Author: Kaveet
 *
 * The C file for parsing and storing the GPS outputs for board recovery. See matching header file for more info.
 */


#include "Recovery Inc/GPS.h"
#include "Lib Inc/minmea.h"
#include <math.h>
#include <string.h>

//For parsing GPS outputs
static void parse_gps_output(GPS_HandleTypeDef* gps, uint8_t* buffer, uint8_t buffer_length);

HAL_StatusTypeDef initialize_gps(UART_HandleTypeDef* huart, GPS_HandleTypeDef* gps){

	gps->huart = huart;

	//TODO: Other initialization like configuring GPS output types or other parameter setting.

	return HAL_OK;
}


bool read_gps_data(GPS_HandleTypeDef* gps){

	uint8_t receiveBuffer[256] = {0};

	if (GPS_SIMULATION){

		gps->data[GPS_SIM].is_valid_data = true;
		gps->data[GPS_SIM].latitude = GPS_SIM_LAT;
		gps->data[GPS_SIM].longitude = GPS_SIM_LON;
		gps->data[GPS_SIM].timestamp[0] = 0;
		gps->data[GPS_SIM].timestamp[1] = 0;
		gps->data[GPS_SIM].timestamp[2] = 0;

		gps->is_pos_locked = true;

		return true;
	}
	HAL_UART_Receive(gps->huart, receiveBuffer, 1, GPS_UART_TIMEOUT);

	if (receiveBuffer[0] == GPS_PACKET_START_CHAR){

		uint8_t read_index = 0;

		while (receiveBuffer[read_index] != GPS_PACKET_END_CHAR){
			read_index++;
			HAL_UART_Receive(gps->huart, &receiveBuffer[read_index], 1, GPS_UART_TIMEOUT);
		}
		parse_gps_output(gps, receiveBuffer, read_index + 1);

		return true;
	}

	return false;
}

static void parse_gps_output(GPS_HandleTypeDef* gps, uint8_t* buffer, uint8_t buffer_length){

	enum minmea_sentence_id sentence_id = minmea_sentence_id(buffer, false);
	struct minmea_sentence_rmc frame;
	float lat;
	float lon;

	switch (sentence_id) {

	case MINMEA_SENTENCE_RMC:

		if (minmea_parse_rmc(&frame, buffer)){

			lat = minmea_tocoord(&frame.latitude);
			lon = minmea_tocoord(&frame.longitude);

			//Ensure the data is valid or not.
			if (isnan(lat) || isnan(lon)){

				//data invalid, set the default values and indicate invalid data
				gps->data[GPS_RMC].latitude = DEFAULT_LAT;
				gps->data[GPS_RMC].longitude = DEFAULT_LON;
				gps->data[GPS_RMC].is_valid_data = false;

			} else {

				//data is valid, save the latitude and longitude + valid data flags
				gps->data[GPS_RMC].latitude = lat;
				gps->data[GPS_RMC].longitude = lon;
				gps->data[GPS_RMC].is_valid_data = true;
				gps->is_pos_locked = true;

				//save the time data into our struct.
				uint16_t time_temp[3] = {frame.time.hours, frame.time.minutes, frame.time.seconds};
				memcpy(gps->data[GPS_RMC].timestamp, time_temp, 3);
			}
		}

		break;

	case MINMEA_SENTENCE_GLL:


		break;

	case MINMEA_SENTENCE_GGA:


		break;

	default:

		break;
	}
}

bool get_gps_lock(GPS_HandleTypeDef* gps, GPS_Data* gps_data){

	gps->is_pos_locked = false;

	//time trackers for any possible timeouts
	uint32_t startTime = HAL_GetTick();
	uint32_t currentTime = startTime;

	//Keep trying to read the GPS data until we get a lock, or we timeout
	while (!gps->is_pos_locked && ((currentTime - startTime) < GPS_TRY_LOCK_TIMEOUT)){

		read_gps_data(gps);
		currentTime = HAL_GetTick();

	}

	//Populate the GPS data struct that we are officially returning to the caller. Prioritize message types in the order they appear in the enum definiton
	for (GPS_MsgTypes msg_type = GPS_SIM; msg_type < GPS_NUM_MSG_TYPES; msg_type++){

		if (gps->data[msg_type].is_valid_data){

			//Set the message type
			gps->data[msg_type].msg_type = msg_type;

			//Copy into the struct that returns back to the user
			memcpy(gps_data, &gps->data[msg_type], sizeof(GPS_Data));

			return true;
		}
	}

	return false;
}
