/****************************************************************************
 * Copyright (C) 2025 by Paweł Smarkucki                                    *
 *                                                                          *
 *   This file is part of NIXIE B16.                                        *
 *                                                                          *
 *   NIXIE B16 is free software: you can redistribute it, modify it,        *
 *   sell it and do whatever you want under no terms or conditions.         *
 *                                                                          *
 *   NIXIE B16 is distributed in the hope that it will be useful,           *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                   *
 ****************************************************************************/

#ifndef __OBJECT_MANAGER_H__
#define __OBJECT_MANAGER_H__

/* Headers */
#include "esp_err.h"
#include "esp_gatts_api.h"
#include "pp_alarm.h"

/* Macros */
#define MOUNT_POINT                 "/sdcard"
#define ALARMS_PATH MOUNT_POINT     "/alarms"
#define RIGNTONES_PATH MOUNT_POINT  "/ringtones"

#define MAX_ALARMS 100
#define MAX_RINGTONES 5
#define MAX_OBJECTS ((MAX_ALARMS) + (MAX_RINGTONES))

#define DATA_LEN_MAX 2000
#define NAME_LEN_MAX 32
#define FLASH_PAGE 256

#define PROPERTY_DELETE             (1<<0)
#define PROPERTY_EXECUTE            (1<<1)
#define PROPERTY_READ               (1<<2)
#define PROPERTY_WRITE              (1<<3)
#define PROPERTY_APPEND             (1<<4)
#define PROPERTY_TRUNCATE           (1<<5)
#define PROPERTY_PATCH              (1<<6)
#define PROPERTY_MARK               (1<<7)
#define PROPERTY_ALL_WITHOUT_MARK   0x7F
#define PROPERTY_ALL                0xFF

#define TYPE_UNSPECIFIED_    0xCA2A    //inversed of little endian

/* STRUCTURES */
typedef enum {
    WAIT_FOR_ACTION = 0,
    WAIT_FOR_FILE_TYPE,
    WAIT_FOR_FILE_SIZE,
    ONGOING,
} action_status_t;

typedef struct file_transfer{
    uint32_t bytes_done;
    action_status_t status;
} file_transfer_t;

typedef struct object{
    int32_t index;
    size_t size;
    size_t alloc_size;
    char name[NAME_LEN_MAX+1];
    uint8_t name_len;
    esp_bt_uuid_t type;
    uint64_t id;
    uint32_t properties;
    bool set_custom_object;
} object_t;

typedef enum{
    ALARM_TYPE,
    RINGTONE_TYPE
}object_type_t;

typedef struct{
    object_type_t type,
    uint64_t id
}object_id_array_t;

typedef struct ListFilter
{
    uint8_t type;
    uint8_t parameter[NAME_LEN_MAX+1];
    uint8_t par_length;
}ListFilter_t;

/***************************************************************************************************************/
/* Functions - Object Manager files */

/** @brief pp_object_manager_init: Object Manager initialization function
 * 
 * This functions initializes sd card and mounts file system. Reads all the objects in alarm and ringtone
 * folders and add them to the object list. If folders do not exist then the function creates the hierarchy.
 * Created list is not filtered and order of objects is not provided.
 * 
 * @return
 */
void pp_object_manager_init(void);

/** @brief pp_object_manager_get_object: Get pointer to current object
 * 
 * This functions provides pointer to current object
 * 
 * @return  (object_t*) Pointer to current object
 */
object_t* pp_object_manager_get_object(void);

/** @brief pp_object_manager_is_object_empty: Check if current object is empty
 * 
 * This functions checks if current object is empty
 * 
 * @return  (bool) True if current object is empty
 *                 False otherwise
 */
bool pp_object_manager_is_object_empty(void);

/** @brief pp_object_manager_create_object: Create object
 * 
 * This functions creates object with specified size and type. Newly created object
 * is set as a current object. List of objects is not filtered after succes of this operation.
 * 
 * @param[in]   size  (uint32_t) Size of file
 * @param[in]   type  (esp_bt_uuid_t) Object type
 * 
 * @return  (oacp_op_code_result_t) Operation result code
 */
oacp_op_code_result_t pp_object_manager_create_object(uint32_t size, esp_bt_uuid_t type);

/** @brief pp_object_manager_delete_object: Delete object
 * 
 * This functions deletes object current_object. Current object is empty
 * after succes of this operation.
 * 
 * @return  (oacp_op_code_result_t) Operation result code
 */
oacp_op_code_result_t pp_object_manager_delete_object(void);

/** @brief pp_object_manager_next_object: Sets first object in object list as current object.
 * 
 * This functions sets first object in object list as current object.
 * 
 * @return  (olcp_op_code_result_t) Operation result code
 */
olcp_op_code_result_t pp_object_manager_first_object(void);

 /** @brief pp_object_manager_next_object: Sets last object in object list as current object.
 * 
 * This functions sets last object in object list as current object.
 * 
 * @return  (olcp_op_code_result_t) Operation result code
 */
olcp_op_code_result_t pp_object_manager_last_object(void);

/** @brief pp_object_manager_next_object: Sets next object in object list as current object.
 * 
 * This functions sets next object in object list as current object.
 * 
 * @return  (olcp_op_code_result_t) Operation result code
 */
olcp_op_code_result_t pp_object_manager_next_object(void);

/** @brief pp_object_manager_previous_object: Sets previous object in object list as current object.
 * 
 * This functions sets previous object in object list as current object.
 * 
 * @return  (olcp_op_code_result_t) Operation result code
 */
olcp_op_code_result_t pp_object_manager_previous_object(void);

/** @brief pp_object_manager_goto_object: Search object with specified ID and set it as current object.
 * 
 * This functions search object with specified ID and set it as current object. The current object
 * remains the same if an object with desired ID does not exist.
 * 
 * @param[in]   id  (uint64_t) ID of an object
 * 
 * @return  (olcp_op_code_result_t) Operation result code
 */
olcp_op_code_result_t pp_object_manager_goto_object(uint64_t id);

/** @brief pp_object_manager_order_object: Change an order of the object list and reaarange object list.
 * 
 * This functions change an order of the object list and reaarange object list.
 * 
 * @param[in]   type  (uint8_t) type of an object
 * 
 * @return  (olcp_op_code_result_t) Operation result code
 */
olcp_op_code_result_t pp_object_manager_order_object(uint8_t type);

/** @brief pp_object_manager_request_number: Provides number of objects in list.
 * 
 * This functions provides number of objects in list. If the filter is already applied on the
 * object list then this function counts only object in filtered list.
 * 
 * @param[in]   type  (uint8_t) type of an object
 * 
 * @return  (olcp_op_code_result_t) Operation result code
 */
olcp_op_code_result_t pp_object_manager_request_number(uint32_t *number);

/** @brief pp_object_manager_clear_marking: Clear marking in objects of object list
 * 
 * This functions clears marking in objects of object list. If the filter is already applied on the
 * object list then this function clears marking only in objects in filtered list.
 * 
 * @return  (olcp_op_code_result_t) Operation result code
 */
olcp_op_code_result_t pp_object_manager_clear_marking(void);

/** @brief pp_object_manager_change_name_in_file: Change the current object name in the file
 * 
 * This functions changes the current object name in the file.
 * 
 * @return
 */
void pp_object_manager_change_name_in_file(void);

/** @brief pp_object_manager_change_properties_in_file: Change the current object properties in the file
 * 
 * This functions changes the current object properties in the file.
 * 
 * @return
 */
void pp_object_manager_change_properties_in_file(void);

/** @brief pp_object_manager_change_alarm_data_in_file: Change the current object alarm data in the file
 * 
 * This functions changes the current object alarm data in the file. Ths function seeks for alarm properties
 * in the file and overwrite existing metadata with provided as an input parameter. If alarm properties cannot
 * be found in the file then in creates new alarm properties section in the file.
 * 
 * @param[in]   alarm  (alarm_mode_args_t) Alarm properties to store in file
 * 
 * @return
 */
void pp_object_manager_change_alarm_data_in_file(alarm_mode_args_t alarm);

/** @brief pp_object_manager_get_alarm_data_from_file: Get alarm data from object file.
 * 
 * This functions read the object alarm data from file and sets 'alarm_p'
 * input argument with it.
 * 
 * @param[in]   id      (uint64_t) object ID
 * @param[out]  alarm_p (alarm_mode_args_t*) Pointer to alarm variable to stare data
 * 
 * @return
 */
esp_err_t pp_object_manager_get_alarm_data_from_file(uint64_t id, alarm_mode_args_t *alarm_p);

/** @brief pp_object_manager_check_type: Check the object type by its uuid.
 * 
 * This functions checks the object type by its uuid and return its coding.
 *      ALARM_TYPE      = 0
 *      RINGTONE_TYPE   = 1
 * 
 * @param[in]   uuid  (uint8_t*) Pointer to arrays with uuid
 * 
 * @return   (object_type_t)  Object type
 */
object_type_t pp_object_manager_check_type(uint8_t *uuid);

/** @brief pp_object_manager_print_file: Print the current object file content to UART.
 * 
 * This functions prints the current file content to UART.
 * 
 * @return
 */
void pp_object_manager_print_file(void);

/** @brief pp_object_manager_printf_alarm_info: Print the current object file alarm properties content to UART.
 * 
 * This functions print the current file alarm properties content to UART.
 * 
 * @return
 */
void pp_object_manager_printf_alarm_info(void);

/** @brief pp_object_manager_print_current_object: Print the current object data to UART.
 * 
 * This functions print the current object data to UART.
 * 
 * @return
 */
void pp_object_manager_print_current_object(void);



/***************************************************************************************************************/
/* Functions - Object list filtering and order */

/** @brief pp_object_list_make_list: Filters and sorts the list using current filter and order options.
 * 
 * This functions performs filtering and sorting of the object lists by using current choosen
 * filtering and ordering options.
 * 
 * @return
 */
void pp_object_list_make_list(void);

/** @brief pp_object_list_get_filter: Get current filter options
 * 
 * This functions return current filter options.
 * 
 * @return  (ListFilter_t*) Pointer to the filter object
 */
ListFilter_t* pp_object_list_get_filter(void);

/** @brief pp_object_list_get_order: Get current order options
 * 
 * This functions return current order options.
 * 
 * @return  (uint8_t) Current order code.
 */
uint8_t pp_object_list_get_order(void);

/** @brief pp_object_list_get_how_many: Get quantity of all objects
 * 
 * This functions return quantity of all objects.
 * 
 * @return  (uint8_t) Quantity of all objects.
 */
uint8_t pp_object_list_get_how_many(void);

/** @brief pp_object_list_get_objects_array: Get pointer to object array
 * 
 * This functions return pointer to object array.
 * 
 * @return  (object_id_array_t *) Pointer to object array.
 */
object_id_array_t * pp_object_list_get_objects_array(void);

#endif