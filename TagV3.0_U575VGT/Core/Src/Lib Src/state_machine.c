/*
 * state_machine.c
 *
 *  Created on: Jul 24, 2023
 *      Author: Kaveet
 */


#include "Lib Inc/state_machine.h"
#include "Sensor Inc/audio.h"
#include "Sensor Inc/BNO08x.h"
#include "Sensor Inc/ECG.h"
#include "Lib Inc/threads.h"

//Event flags for signaling changes in state
TX_EVENT_FLAGS_GROUP state_machine_event_flags_group;

//External event flags for the other sensors. Use these to signal them to whenever they get the chance
extern TX_EVENT_FLAGS_GROUP audio_event_flags_group;
extern TX_EVENT_FLAGS_GROUP imu_event_flags_group;
extern TX_EVENT_FLAGS_GROUP ecg_event_flags_group;

//Threads array
extern Thread_HandleTypeDef threads[NUM_THREADS];

void state_machine_thread_entry(ULONG thread_input){

	//Event flags for triggering state changes
	tx_event_flags_create(&state_machine_event_flags_group, "State Machine Event Flags");

	//If simulating, set the simulation state defined in the header file, else, enter data capture as a default
	State state = (IS_SIMULATING) ? SIMULATING_STATE : STATE_DATA_CAPTURE;

	//Check the initial state and start in the appropriate state
	if (state == STATE_DATA_CAPTURE){
		enter_data_capture();
	} else if (state == STATE_RECOVERY){
		enter_recovery();
	} else if (state == STATE_DATA_OFFLOAD){
		enter_data_offload();
	}

	//Enter main thread execution loop
	while (1){

		ULONG actual_flags;
		tx_event_flags_get(&state_machine_event_flags_group, ALL_STATE_FLAGS, TX_OR_CLEAR, &actual_flags, TX_WAIT_FOREVER);

		//Timeout event
		if (actual_flags & STATE_TIMEOUT_FLAG){

		}

		//GPS geofencing flag
		if (actual_flags & STATE_GPS_FLAG){

		}

		//BMS low battery flag
		if (actual_flags & STATE_LOW_BATT_FLAG){

		}

		//USB detected flag
		if (actual_flags & STATE_V_BUS_FLAG){

		}
	}
}


void enter_data_capture(){

	//Start data capture threads
	tx_thread_resume(&threads[AUDIO_THREAD].thread);
	tx_thread_resume(&threads[IMU_THREAD].thread);
	tx_thread_resume(&threads[ECG_THREAD].thread);
}


void exit_data_capture(){

	//Signal data collection threads to stop running
	tx_event_flags_set(&audio_event_flags_group, AUDIO_STOP_THREAD_FLAG, TX_OR);
	tx_event_flags_set(&imu_event_flags_group, IMU_STOP_THREAD_FLAG, TX_OR);
	tx_event_flags_set(&ecg_event_flags_group, ECG_STOP_THREAD_FLAG, TX_OR);
}


void enter_recovery(){
	//Start APRS thread
	tx_thread_resume(&threads[APRS_THREAD].thread);
}


void exit_recovery(){
	//Stop APRS thread
	tx_thread_terminate(&threads[APRS_THREAD].thread);
}


void enter_data_offload(){

}

void exit_data_offload(){

}
