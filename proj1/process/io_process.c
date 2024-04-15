/*
 * The only logic that the io process has, aside from device io logic,
 * is to check and request to MERGE process if MERGE is needed.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "io_process.h"
#include "../ipc/payload/shm_input.h"
#include "../ipc/payload/shm_output.h"
#include "../common/logging.h"
#include "../common/mode.h"

static struct device_controller* dc;
static const char ERROR_STR[10] = "Error";
static const enum mode MODES[MODE_CNT] = {PUT, GET, MERGE};
static const enum led_action INITIAL_LED_AC[MODE_CNT] = {LED1_ON, LED5_ON, LED_OFF};
static const char MODES_STR[MODE_CNT][TEXT_LCD_MAX_LINE] = {"PUT Mode", "GET Mode", "MERGE Mode"};
static const char SW_MAPPING_TABLE[SWITCH_CNT + 1][5] = {
	"", 		// 0
	"", 		// 1
	"ABC", 		// 2
	"DEF", 		// 3
	"GHI", 		// 4
	"JKL", 		// 5
	"MNO", 		// 6
	"PQRS", 	// 7
	"TUV", 		// 8
	"WXYZ" 		// 9
};

/* modifiable global variable */
static int cur_mode_idx;
static enum led_action cur_led_ac;
static int is_english;
static int is_key;

/* key-value buffer and functions */
static char key[KEY_DIGIT + 1];
static int key_idx;
static char val[VAL_MAX_LEN + 1];
static int val_idx;
static inline int key_exist();
static inline int val_exist();
static inline void init_key();
static inline void init_val();
static void append_key(char sw);
static void append_val(char sw);

/* utility functions */
static void reset_settings_on_mode();
static void apply_switch_input(char sw);
static void set_shm_input(struct shm_input* shmi);
static void exchange_shm_io_sync_with_main_process(struct sem_ids* sem, struct shm_io* ipc_io, struct shm_input* shmi, struct shm_output* shmo);

static inline int key_exist(){
	return key[0] != '0';
}

static inline int val_exist(){
	return val[0] != '\0';
}

static inline void init_key(){
	memset(key, '0', sizeof(key) - 1);
	key_idx = 0;
}

static inline void init_val(){
	memset(val, 0, sizeof(val));
	val_idx = 0;
}

static void append_key(char sw){
	if(sw - '0' < 1 || sw - '0' > 9){
		LOG(LOG_LEVEL_ERROR, "apply_switch_input: invalid sw");
		killpg(getpgrp(), SIGABRT);
	}
	
	key[key_idx] = sw;
	key_idx = (key_idx + 1) % KEY_DIGIT;
}

static void append_val(char sw){
	int n = sw - '0';
	if(n < 2 || n > 9){
		LOG(LOG_LEVEL_ERROR, "apply_switch_input: invalid sw");
		killpg(getpgrp(), SIGABRT);
	}
	const char* chars = SW_MAPPING_TABLE[n];
	char prev_char;
	char next_char = chars[0];
	int i;
	if(!is_english){
		next_char = sw;
	}
	else{
		if(val_exist()){
			// search SW_MAPPING_TABLE
			prev_char = val[(val_idx + VAL_MAX_LEN - 1) % VAL_MAX_LEN];
			// search prev_char
			for(i = 0; chars[i] != '\0'; ++i){
				if(chars[i] == prev_char){
					next_char = chars[i + 1] ? chars[i + 1] : chars[0];
					// change prev_char
					val_idx = (val_idx + VAL_MAX_LEN - 1) % VAL_MAX_LEN;
				}
			}
		}
	}
	val[val_idx] = next_char;
	val_idx = (val_idx + 1) % VAL_MAX_LEN;
}

/* performs the action corresponding to the given switch input */
static void apply_switch_input(char sw){
	if(sw - '0' < 1 || sw - '0' > 9){
		LOG(LOG_LEVEL_ERROR, "apply_switch_input: invalid sw %d", sw);
		killpg(getpgrp(), SIGABRT);
	}
	if(MODES[cur_mode_idx] == PUT){
		if(is_key){
			cur_led_ac = LED3_AND_LED4_TOGGLE;
			append_key(sw);
			device_controller_fnd_print(dc, key);
			return;
		}
		cur_led_ac = LED7_AND_8_TOGGLE;
		append_val(sw);
		device_controller_lcd_print(dc, MODES_STR[cur_mode_idx], val);
		return;
	}
	if(MODES[cur_mode_idx] == GET){
		cur_led_ac = LED3_AND_LED4_TOGGLE;
		append_key(sw);
		device_controller_fnd_print(dc, key);
		return;
	}
}

static void reset_settings_on_mode(){
	cur_led_ac = INITIAL_LED_AC[cur_mode_idx];
	is_english = 0;
	is_key = 1;
	init_key();
	init_val();
}

static void set_shm_input(struct shm_input* shmi){
	memset(shmi, 0, sizeof(struct shm_input));

	shmi->terminate = 0;
	shmi->m = MODES[cur_mode_idx];
	memcpy(shmi->r.key, key, sizeof(key));
	memcpy(shmi->r.val, val, sizeof(val));
}

static void exchange_shm_io_sync_with_main_process(struct sem_ids* sem, struct shm_io* ipc_io, struct shm_input* shmi, struct shm_output* shmo){
	// initialize shared memory input value
	set_shm_input(shmi);
	// write the value to shared memory
	shm_io_write_input(ipc_io, shmi);
	// notify the main process that a job has arrived
	sem_shm_input_up(sem);
	// wait for main process to finish its job
	sem_shm_output_down(sem);
	// read shared memory output value initialized by main process
	shm_io_read_output(ipc_io, shmo);
}

static void merge(struct database* db, struct bidir_message_queue* msgq, struct merge_res* res){
	device_controller_motor_on(dc);
	// background merge
	bidir_message_queue_send_merge_req(msgq, res);
	device_controller_motor_off(dc);
}

void io_process(struct sem_ids* sem, struct database* db, struct shm_io* ipc_io, struct bidir_message_queue* msgq){
	// set default
	dc = device_controller_create();
	cur_mode_idx = 0;
	reset_settings_on_mode();
	device_controller_fnd_off(dc);
	device_controller_lcd_print(dc, MODES_STR[cur_mode_idx], "\0");
	
	enum input_type input;
	struct shm_input shmi;
	struct shm_output shmo;
	char result_buf[TEXT_LCD_MAX_LINE];
	while(1){
		input = device_controller_get_input(dc, cur_led_ac);

		switch(input){
			
			case BACK:
				shmi.terminate = 1;
				shm_io_write_input(ipc_io, &shmi);
				sem_shm_input_up(sem);
				sem_shm_output_down(sem);
				shm_io_read_output(ipc_io, &shmo);
				LOG(LOG_LEVEL_INFO, "terminating now...");
				// do remaining tasks
				if(storage_table_is_full(db)) {
					struct merge_res dummy;
					merge(db, msgq, &dummy);
				} 
				device_controller_fnd_off(dc);
				device_controller_lcd_off(dc);
				device_controller_led_off(dc);
				return; // BACK

			case VOL_UP:
				cur_mode_idx = (cur_mode_idx + 1) % MODE_CNT;
				reset_settings_on_mode();
				device_controller_fnd_off(dc);
				device_controller_lcd_print(dc, MODES_STR[cur_mode_idx], "\0");
				break; // VOL_UP

			case VOL_DOWN:
				cur_mode_idx = (cur_mode_idx + MODE_CNT - 1) % MODE_CNT;
				reset_settings_on_mode();	
				device_controller_fnd_off(dc);
				device_controller_lcd_print(dc, MODES_STR[cur_mode_idx], "\0");
				break; // VOL_DOWN

			case S4_AND_S6:
				if(MODES[cur_mode_idx] == PUT){
					int led_all = 0; // false
					if(key_exist() && val_exist()){
						exchange_shm_io_sync_with_main_process(sem, ipc_io, &shmi, &shmo);

						if(storage_table_is_full(db)){
							struct merge_res dummy;
							merge(db, msgq, &dummy);
						}
						
						sprintf(result_buf, "(%lld, %s,%s)", shmo.r.pk, shmo.r.key, shmo.r.val);
						led_all = 1;
					}
					else{
						LOG(LOG_LEVEL_INFO, "key and value do not exist");
						sprintf(result_buf, ERROR_STR);
					}
					reset_settings_on_mode();	
					device_controller_fnd_off(dc);
					device_controller_lcd_print(dc, MODES_STR[cur_mode_idx], result_buf);
					if(led_all) device_controller_led_all_on(dc);
				}
				break; // S4_AND_S6

			case RESET:
				if(MODES[cur_mode_idx] == PUT){
					if(cur_led_ac == LED3_AND_LED4_TOGGLE){
						cur_led_ac = LED7_AND_8_TOGGLE;
					}
					else if(cur_led_ac == LED7_AND_8_TOGGLE){
						cur_led_ac = LED3_AND_LED4_TOGGLE;
					}
					is_key = !is_key;
				}
				if(MODES[cur_mode_idx] == GET){
					int led_all = 0; // false
					if(key_exist()){
						exchange_shm_io_sync_with_main_process(sem, ipc_io, &shmi, &shmo);
						
						if(!shmo.error){
							sprintf(result_buf, "(%lld, %s,%s)", shmo.r.pk, shmo.r.key, shmo.r.val);
							led_all = 1;
						}
						else{
							sprintf(result_buf, ERROR_STR);
						}
					}
					else{
						LOG(LOG_LEVEL_INFO, "key does not exist");
						sprintf(result_buf, ERROR_STR);
					}
					reset_settings_on_mode();
					device_controller_fnd_off(dc);
					device_controller_lcd_print(dc, MODES_STR[cur_mode_idx], result_buf);
					if(led_all) device_controller_led_all_on(dc);
				}
				if(MODES[cur_mode_idx] == MERGE){
					if(!storage_table_can_merge(db)){
						LOG(LOG_LEVEL_INFO, "storage table size is not enough to merge");
						sprintf(result_buf, ERROR_STR);
					}
					else{
						struct merge_res res;
						merge(db, msgq, &res);
						sprintf(result_buf, "(%d, %zu)", res.st_name, res.cnt);
					}

					reset_settings_on_mode();
					device_controller_fnd_off(dc);
					device_controller_lcd_print(dc, MODES_STR[cur_mode_idx], result_buf);
				}
				break; // RESET

			case S1_HOLD:
				if(MODES[cur_mode_idx] == PUT){
					if(is_key){
						init_key();
						device_controller_fnd_off(dc);
					}
					else{
						init_val();
						device_controller_lcd_print(dc, MODES_STR[cur_mode_idx], "\0");
					}
				}
				break; // S1_HOLD
			
			case S1:
				if(MODES[cur_mode_idx] == PUT && !is_key){
					is_english = !is_english;
				}
				else{
					apply_switch_input('1');
				}
				break; // S1

			case S2:
				apply_switch_input('2');
				break;

			case S3:
				apply_switch_input('3');
				break;

			case S4:
				apply_switch_input('4');
				break;

			case S5:
				apply_switch_input('5');
				break;

			case S6:
				apply_switch_input('6');
				break;

			case S7:
				apply_switch_input('7');
				break;

			case S8:
				apply_switch_input('8');
				break;

			case S9:
				apply_switch_input('9');
				break;

			case DEFAULT:
				break;
		}
		
	}
}
