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

/* Headers */
#include "pp_object_manager.h"

/* Macros */
#define TAG "OBJECT_MANAGER"

/* Declarations */
static FILE* pp_object_manager_open_file(const char* option,  uint64_t id);
static void pp_object_manager_truncate_rest(uint64_t id, uint32_t offset);
static void pp_object_manager_set_current_object_from_file(uint64_t id)
static bool pp_seekfor(FILE *stream, const char* str, fpos_t *pos);
static char* id_to_string(char* bfr, uint64_t id)

static void pp_object_list_filter(filter_function fun);
static void pp_object_list_sort(compare_function fun, bool asc);

static int pp_object_list_compare_name(uint64_t rID, uint64_t lID, bool asc);
static int pp_object_list_compare_type(uint64_t rID, uint64_t lID, bool asc);
static int pp_object_list_compare_size(uint64_t rID, uint64_t lID, bool asc);

static char* pp_object_list_read_name_from_file(char* dest, uint64_t id);
static uint8_t* pp_object_list_read_type_from_file(uint8_t* dest, uint64_t id);
static uint32_t pp_object_list_read_current_size_from_file(uint64_t id);

static bool pp_object_list_no_filter(uint64_t id);
static bool pp_object_list_name_starts_with(uint64_t id);
static bool pp_object_list_name_ends_with(uint64_t id);
static bool pp_object_list_name_containts(uint64_t id);
static bool pp_object_list_name_is_exactly(uint64_t id);
static bool pp_object_list_object_type(uint64_t id);
static bool pp_object_list_current_size_between(uint64_t id);
static bool pp_object_list_alloc_size_between(uint64_t id);
static bool pp_object_list_marked_objects(uint64_t id);

/***************************************************************************************************************/
/* Types */
typedef bool (*filter_function)(uint64_t);
typedef int (*compare_function)(uint64_t, uint64_t, bool);       //par bool: 1-ascending, 0-descending

/*  Variables */
static object_t current_object;
    
static file_transfer_t file_transfer = {
    .bytes_done = 0,
    .status = WAIT_FOR_ACTION,
};

/** @brief filter_func_table: Filter decision table
 * 
 * This array is a filter decision array. It consists of pointer to functions 
 * for every type of supported object filtering
 *
 */
const filter_function filter_func_table[FILTER_RANGE] = {
    pp_object_list_no_filter,               // NO_FILTER                = 0x00
    pp_object_list_name_starts_with,        // NAME_STARTS_WITH         = 0x01
    pp_object_list_name_ends_with,          // NAME_ENDS_WITH           = 0x02
    pp_object_list_name_containts,          // NAME_CONTAINS            = 0x03
    pp_object_list_name_is_exactly,         // NAME_IS_EXACTLY          = 0x04
    pp_object_list_object_type,             // OBJECT_TYPE              = 0x05
    NULL,                                   // RESERVED
    NULL,                                   // RESERVED
    pp_object_list_current_size_between,    // CURRENT_SIZE_BETWEEN     = 0x08
    pp_object_list_alloc_size_between,      // ALLOC_SIZE_BETWEEN       = 0x09
    pp_object_list_marked_objects           // MARKED_OBJECTS           = 0x0A
};

/** @brief compare_func_table: Compare decision table
 * 
 * This array is a compare decision array. It consists of pointer to functions 
 * for every type of supported comparing used to sort the objects list
 *
 */
const compare_function compare_func_table[ORDER_RANGE] = {
    NULL,                                   // RESERVED
    pp_object_list_compare_name,            // NAME_ASC                        0x01
    pp_object_list_compare_type,            // TYPE_ASC                        0x02
    pp_object_list_compare_size,            // CURRENT_SIZE_ASC                0x03
    NULL,                                   // RESERVED
    NULL,                                   // RESERVED
    NULL,                                   // RESERVED
    NULL,                                   // RESERVED
    NULL,                                   // RESERVED
    NULL,                                   // RESERVED
    NULL,                                   // RESERVED
    pp_object_list_compare_name,            // NAME_DESC                       0x11
    pp_object_list_compare_type,            // TYPE_DESC                       0x12
    pp_object_list_compare_size             // CURRENT_SIZE_DESC               0x13
};

/** @brief compare_func_table_asc: Order decision table
 * 
 * This array is a order decision array. It decides if the order of the 
 * choosen sorting is asceding or descending
 *
 */
bool compare_func_table_asc[ORDER_RANGE] = {
    false,
    true,
    true,
    true,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    true,
    true,
    true,
}

static ListFilter_t filter;     // Current objects filter
static uint8_t order;           // Current objects order

static object_id_array_t object_list[MAX_OBJECTS];  // List of objects
static uint32_t alarm_count = 0;
static uint32_t ringtone_count = 0;
static uint64_t max_id = 0;                 // the greatest id in object list
static int32_t unfiltered_end_idx = -1;     // index of last unfiltered object

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
void pp_object_manager_init(void)
{   
    alarm_count = 0;
    ringtone_count = 0;
    max_id = 0;
    unfiltered_end_idx = -1;

    order = 0;
    filter.type = 0;
    filter.par_length = 0;

    ESP_LOGI(SDCARD_TAG, "Initializing sd card");
    esp_vfs_fat_sdmmc_mount_config_t mount_config = 
    {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t *card;
    const char mount_point[] = MOUNT_POINT;
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = 10000;          // TODO: Change in future to higher value
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    slot_config.width = 4;
    slot_config.cmd = 16;
    slot_config.clk = 15;
    slot_config.d0 = 17;
    slot_config.d1 = 5;
    slot_config.d2 = 6;
    slot_config.d3 = 7;
    slot_config.cd = 4;
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

#ifdef FORMAT_SD
    ESP_LOGI(SDCARD_TAG, "Formating sd card");
    ESP_ERROR_CHECK(esp_vfs_fat_sdcard_format(mount_point, card));
#endif

    ESP_LOGI(SDCARD_TAG, "Mounting filesystem");
    ESP_ERROR_CHECK(esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card));

    FRESULT fr = f_stat(ALARMS_PATH, NULL);

    if(fr == FR_NO_PATH)
    {
        FRESULT res = f_mkdir(ALARMS_PATH);

        if(res != FR_OK)
        {
            ESP_LOGE(TAG, "Can't create \\alarms directory");
            ESP_ERROR_CHECK(ESP_FAIL);
        }
    }

    DIR dir;
    FILINFO fno;
    FRESULT fr = f_opendir(&dir, ALARMS_PATH);

    if (fr != FR_OK)
    {
        ESP_LOGE(TAG, "Can't open \\alarms directory");
        ESP_ERROR_CHECK(ESP_FAIL);
    }

    while(true)
    {
        fr = f_readdir(&dir, &fno);                   /* Read a directory item */
        if (fr != FR_OK || fno.fname[0] == 0) 
        {
            break;  /* Error or end of dir */
        }
        else    /* File */
        {
            uint64_t id = strtoull(fno.fname, NULL, 16);
            uint8_t type[ESP_UUID_LEN_128];
            pp_object_list_read_type_from_file(type, id)
            pp_object_list_add_by_id(pp_object_manager_check_type(type), id);
        }
    }

    fr = f_closedir(&dir);

    if (fr != FR_OK)
    {
        ESP_LOGE(TAG, "Can't close \\alarms directory");
        ESP_ERROR_CHECK(ESP_FAIL);
    }

    current_object.index = -1;

    pp_object_list_make_list();
}

/** @brief pp_object_manager_get_object: Get pointer to current object
 * 
 * This functions provides pointer to current object
 * 
 * @return  (object_t*) Pointer to current object
 */
object_t* pp_object_manager_get_object(void)
{
    return &current_object;
}

/** @brief pp_object_manager_is_object_empty: Check if current object is empty
 * 
 * This functions checks if current object is empty.
 * 
 * @return  (bool) True if current object is empty
 *                 False otherwise
 */
bool pp_object_manager_is_object_empty(void)
{
    if(current_object.index < 0) return true;
    return false;
}

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
oacp_op_code_result_t pp_object_manager_create_object(uint32_t size, esp_bt_uuid_t type)
{
    ESP_LOGI(OBJECT_TAG, "Requested object type UUID:");
    if(type.len == ESP_UUID_LEN_16) ESP_LOGI(OBJECT_TAG, "%02x", type.uuid.uuid16);
    else if(type.len == ESP_UUID_LEN_128) ESP_LOG_BUFFER_HEX_LEVEL(OBJECT_TAG, type.uuid.uuid128, ESP_UUID_LEN_128, ESP_LOG_INFO);


    if(type.len == ESP_UUID_LEN_16)
    {
        ESP_LOGE(OBJECT_TAG, "Unsupported object type with 16-bit UUID");
        return OACP_RES_UNSUPPORTED_TYPE;
    }

    object_type_t ret_type = pp_object_manager_check_type(type.uuid.uuid128);

    switch(ret_type)
    {
        case ALARM_TYPE:
            ESP_LOGI(OBJECT_TAG, "Requested object type: Alarm file");
            break;

        case RINGTONE_TYPE:
            ESP_LOGI(OBJECT_TAG, "Requested object type: Ringtone file");
            break;

        default:
            ESP_LOGE(OBJECT_TAG, "Unsupported object type with 128-bit UUID");
            return OACP_RES_UNSUPPORTED_TYPE;
    }

    ESP_LOGI(OBJECT_TAG, "Creating list object");
    uint64_t new_id = pp_object_list_add(ret_type);
    ESP_LOGI(OBJECT_TAG, "Object created, ID: %" PRIx64, new_id);

    current_object.size = 0;
    current_object.alloc_size = 0;
    current_object.name[0] = '\0';
    current_object.name_len = 0;
    current_object.type.len = ESP_UUID_LEN_128;
    memcpy(current_object.type.uuid.uuid128, type.uuid.uuid128, ESP_UUID_LEN_128);
    current_object.id = new_id;
    current_object.properties = PROPERTY_ALL;

    ESP_LOGI(OBJECT_TAG, "Creating file on SD Card");
    FILE* f = pp_object_manager_open_file("w+", new_id);

    fprintf(f, "Size: %x\n", current_object.size);
    fprintf(f, "Allocated size: %x\n", current_object.alloc_size);
    fprintf(f, "Name length: 0\n");
    fprintf(f, "Name: \n");
    fprintf(f, "UUID type: %u\n", 128);
    fprintf(f, "UUID: ");

    for(int i=15; i>=0; i--)
    {
        fprintf(f, "%02x", type.uuid.uuid128[i]);
    }
    fprintf(f, "\n");
    fprintf(f, "Properties: %x\n", PROPERTY_ALL_WITHOUT_MARK);
    fclose(f);
    ESP_LOGI(OBJECT_TAG, "File created: %" PRIx64, new_id);

    switch(ret_type)
    {
        case ALARM_TYPE:
            current_object.set_custom_object = false;
            break;

        case RINGTONE_TYPE:
            ESP_LOGI(OBJECT_TAG, "Ringtone files not supported yet");
            break;
    }

    ESP_LOGI(OBJECT_TAG, "ID inserted into the list");
    filter.type = NO_FILTER;
    pp_object_list_make_list();
    current_object.index = pp_object_list_search(new_id, false);
    
    return OACP_RES_SUCCESS;
}

/** @brief pp_object_manager_delete_object: Delete object
 * 
 * This functions deletes object current_object. Current object is empty
 * after succes of this operation.
 * 
 * @return  (oacp_op_code_result_t) Operation result code
 */
oacp_op_code_result_t pp_object_manager_delete_object(void)
{
    if(pp_object_manager_is_object_empty())
    {
        ESP_LOGE(OBJECT_TAG, "Invalid current object");
        return OACP_RES_INVALID_OBJECT;
    }

    if((current_object.properties & PROPERTY_DELETE) == 0)
    {
        ESP_LOGE(OBJECT_TAG, "Procedure not permitted");
        return OACP_RES_PROCEDURE_NOT_PERMIT;
    }

    char file[20];
    strcpy(file, MOUNT_POINT);
    strcat(file, "/");
    itoa(current_object.id, &file[8], 16);
    ESP_LOGI(OBJECT_TAG, "File to remove: %s", file);
    remove(file);
    ESP_LOGI(OBJECT_TAG, "File removed");

    ESP_LOGI(OBJECT_TAG, "ID to remove from list: %llx", current_object.id);
    pp_object_list_delete_by_id(current_object.id);
    ESP_LOGI(OBJECT_TAG, "ID removed from list");
    pp_object_list_make_list();
    current_object.index = -1;

    return OACP_RES_SUCCESS;
}

/** @brief pp_object_manager_next_object: Sets first object in object list as current object.
 * 
 * This functions sets first object in object list as current object.
 * 
 * @return  (olcp_op_code_result_t) Operation result code
 */
olcp_op_code_result_t pp_object_manager_first_object(void)
{
    if(unfiltered_end_idx < 0)
    {
        ESP_LOGI(OBJECT_TAG, "List is empty");
        return OLCP_RES_SUCCESS;
    }

    pp_object_manager_set_current_object_from_file(object_list[0]->id);
    current_object.index = 0;
    
    return OLCP_RES_SUCCESS;
}

 /** @brief pp_object_manager_next_object: Sets last object in object list as current object.
 * 
 * This functions sets last object in object list as current object.
 * 
 * @return  (olcp_op_code_result_t) Operation result code
 */
olcp_op_code_result_t pp_object_manager_last_object(void)
{
    if(unfiltered_end_idx < 0)
    {
        ESP_LOGI(OBJECT_TAG, "List is empty");
        return OLCP_RES_NO_OBJECT;
    }

    pp_object_manager_set_current_object_from_file(object_list[unfiltered_end_idx]->id);
    current_object.index = unfiltered_end_idx;
    return OLCP_RES_SUCCESS;
}

/** @brief pp_object_manager_next_object: Sets next object in object list as current object.
 * 
 * This functions sets next object in object list as current object.
 * 
 * @return  (olcp_op_code_result_t) Operation result code
 */
olcp_op_code_result_t pp_object_manager_next_object(void)
{
    if(pp_object_manager_is_object_empty())
    {
        ESP_LOGI(OBJECT_TAG, "Current object is invalid");
        return OLCP_RES_OPERATION_FAILED;
    }

    if(current_object.index == unfiltered_end_idx)
    {
        ESP_LOGI(OBJECT_TAG, "Next object is out of the bonds");
        return OLCP_RES_OUT_OF_THE_BONDS;
    }

    pp_object_manager_set_current_object_from_file(object_list[current_object.index + 1]->id);
    ++current_object.index;
    return OLCP_RES_SUCCESS;
}

/** @brief pp_object_manager_previous_object: Sets previous object in object list as current object.
 * 
 * This functions sets previous object in object list as current object.
 * 
 * @return  (olcp_op_code_result_t) Operation result code
 */
olcp_op_code_result_t pp_object_manager_previous_object(void)
{
    if(pp_object_manager_is_object_empty())
    {
        ESP_LOGI(OBJECT_TAG, "Current object is invalid");
        return OLCP_RES_OPERATION_FAILED;
    }

    if(current_object.index == 0)
    {
        ESP_LOGI(OBJECT_TAG, "Previous object is out of the bonds");
        return OLCP_RES_OUT_OF_THE_BONDS;
    }

    pp_object_manager_set_current_object_from_file(object_list[current_object.index - 1]->id);
    --current_object.index;
    return OLCP_RES_SUCCESS;
}

/** @brief pp_object_manager_goto_object: Search object with specified ID and set it as current object.
 * 
 * This functions search object with specified ID and set it as current object. The current object
 * remains the same if an object with desired ID does not exist.
 * 
 * @param[in]   id  (uint64_t) ID of an object
 * 
 * @return  (olcp_op_code_result_t) Operation result code
 */
olcp_op_code_result_t pp_object_manager_goto_object(uint64_t id)
{
    if(unfiltered_end_idx < 0)
    {
        ESP_LOGI(OBJECT_TAG, "No objects on the server");
        return OLCP_RES_NO_OBJECT;
    }

    int index = pp_object_list_search(id, false);

    if(index < 0)
    {
        ESP_LOGI(OBJECT_TAG, "Object not found");
        return OLCP_RES_OBJECT_NOT_FOUND;
    }

    if(filter.type != NO_FILTER)
    {
        filter.type = NO_FILTER;
        pp_object_list_make_list();
    }

    pp_object_manager_set_current_object_from_file(id);
    current_object.index = pp_object_list_search(id, false);
    return OLCP_RES_SUCCESS;
}

/** @brief pp_object_manager_order_object: Change an order of the object list and reaarange object list.
 * 
 * This functions change an order of the object list and reaarange object list.
 * 
 * @param[in]   type  (uint8_t) type of an object
 * 
 * @return  (olcp_op_code_result_t) Operation result code
 */
olcp_op_code_result_t pp_object_manager_order_object(uint8_t type)
{
    if(type == 0 || (type >= 0x04 && type <= 0x10) || type >= 0x14)
    {
        return OLCP_RES_INVALID_PAR;
    }
    
    if(unfiltered_end_idx < 0)
    {
        ESP_LOGI(TAG, "No objects on the server");
        return OLCP_RES_NO_OBJECT;
    }

    ESP_LOGI(TAG, "Order type: %x", order);
    order = type;
    pp_object_list_make_list();


    if(!pp_object_manager_is_object_empty())
    {
        current_object.index = pp_object_list_search(current_object.id, true);
    }

    return OLCP_RES_SUCCESS;
}

/** @brief pp_object_manager_request_number: Provides number of objects in list.
 * 
 * This functions provides number of objects in list. If the filter is already applied on the
 * object list then this function counts only object in filtered list.
 * 
 * @param[in]   type  (uint8_t) type of an object
 * 
 * @return  (olcp_op_code_result_t) Operation result code
 */
olcp_op_code_result_t pp_object_manager_request_number(uint32_t *number)
{
    *number = unfiltered_end_idx + 1;
    ESP_LOGI(OBJECT_TAG, "Number of objects: %" PRIu32, *number);
    return OLCP_RES_SUCCESS;
}

/** @brief pp_object_manager_clear_marking: Clear marking in objects of object list
 * 
 * This functions clears marking in objects of object list. If the filter is already applied on the
 * object list then this function clears marking only in objects in filtered list.
 * 
 * @return  (olcp_op_code_result_t) Operation result code
 */
olcp_op_code_result_t pp_object_manager_clear_marking(void)
{
    for(int i = 0; i <= unfiltered_end_idx; ++i)
    {
        FILE* f = pp_object_manager_open_file("r+", object_list[i].id);

        char line[70];

        fgets(line, sizeof(line), f);
        fgets(line, sizeof(line), f);
        fgets(line, sizeof(line), f);
        fgets(line, sizeof(line), f);
        fgets(line, sizeof(line), f);
        fgets(line, sizeof(line), f);

        fseek(f, strlen("Properties: 000000"), SEEK_CUR);
        fpos_t position;
        fgetpos( f, &position );
        char property_bit_str[3];
        property_bit_str[0] = (char)fgetc(f);
        property_bit_str[1] = (char)fgetc(f);
        property_bit_str[2] = '\0';
        char *ptr;

        uint8_t property_bit = strtoll(property_bit_str, &ptr, 16);
        property_bit &= ~PROPERTY_MARK;

        fsetpos(f, &position );
        fprintf(f, "%02x", property_bit);

        fclose(f);
    }

    ESP_LOGI(OBJECT_TAG, "Clearing markings done");
    return OLCP_RES_SUCCESS;
}

/** @brief pp_object_manager_change_name_in_file: Change the current object name in the file
 * 
 * This functions changes the current object name in the file.
 * 
 * @return
 */
void pp_object_manager_change_name_in_file(void)
{
    FILE* f = pp_object_manager_open_file("r+", current_object.id);

    char line[70];

    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);
    fseek(f, strlen("Name: "), SEEK_CUR);
    fputs(current_object.name, f);

    for(uint8_t i=0; i<NAME_LEN_MAX-strlen(current_object.name)-1; i++)
    {
        fputc('*', f);
    }

    fgets(line, sizeof(line), f);
    fseek(f, strlen("Name length: "), SEEK_CUR);

    char name_len_string[5];
    itoa(current_object.name_len, name_len_string, 10);

    if(strlen(name_len_string) == 1)
    {
        fputc('0', f);
    }
    fputs(name_len_string, f);
    fclose(f);
}

/** @brief pp_object_manager_change_properties_in_file: Change the current object properties in the file
 * 
 * This functions changes the current object properties in the file.
 * 
 * @return
 */
void pp_object_manager_change_properties_in_file(void)
{
    FILE* f = pp_object_manager_open_file("r+", current_object.id);

    fseek(f, 0, SEEK_SET);

    char line[70];

    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);
    fseek(f, strlen("Properties: 000000"), SEEK_CUR);

    if(current_object.properties <= 0xF)
    {
        fputc('0', f);
    }
    itoa(current_object.properties, line, 16);
    fputs(line, f);
    fputc('\n', f);

    fclose(f);
}

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
void pp_object_manager_change_alarm_data_in_file(alarm_mode_args_t *alarm)
{
    FILE* f = pp_object_manager_open_file("r+", current_object.id);

    fseek(f, 0, SEEK_SET);
    fpos_t alarmPosition;
    bool found = pp_seekfor(f, "ALARM PROPERTIES\n", &alarmPosition);
    fsetpos(f, &alarmPosition);

    if(!found) fprintf(f, "\n");
    fprintf(f, "ALARM PROPERTIES\n");
    fprintf(f, "Mode: %02x\n", alarm->mode);
    fprintf(f, "Enable: %02x\n", alarm->enable);
    fprintf(f, "Description length: %01x\n", alarm->desc_len);
    fprintf(f, "Description: %s\n", alarm->desc);
    fprintf(f, "Hour: %02x\n", alarm->hour);
    fprintf(f, "Minute: %02x\n", alarm->minute);
    
    switch(alarm.mode) 
    {
        case ALARM_SINGLE_MODE:
        {
            fprintf(f, "Day: %02x\n", alarm->args.single_alarm_args.day);
            fprintf(f, "Month: %02x\n", alarm->args.single_alarm_args.month);
            fprintf(f, "Year: %02x\n", alarm->args.single_alarm_args.year);
            break;
        }
            
        case ALARM_WEEKLY_MODE:
        {
            fprintf(f, "Days: %02x\n", alarm->args.days);
            break;
        }
            
        case ALARM_MONTHLY_MODE:
        {
            fprintf(f, "Day: %02x\n", alarm->args.day);
            break;
        }
            
        case ALARM_YEARLY_MODE:
        {
            fprintf(f, "Day: %02x\n", alarm->args.yearly_alarm_args.day);
            fprintf(f, "Month: %02x\n", alarm->args.yearly_alarm_args.month);
            break;
        }
    }

    fprintf(f, "Volume: %02x\n", alarm->volume);

    uint32_t truncate_offset = ftell(f);
    fseek(f, 0, SEEK_SET);
    fclose(f);
    pp_object_manager_truncate_rest(current_object.id, truncate_offset);
}

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
esp_err_t pp_object_manager_get_alarm_data_from_file(uint64_t id, alarm_mode_args_t *alarm_p)
{
    FILE* f = pp_object_manager_open_file("r", id);

    fpos_t pos;
    bool found = pp_seekfor(f, "ALARM PROPERTIES\n", &pos);

    if(!found)
    {
        return ESP_ERR_NOT_FOUND;
    }

    char line[70];
    char *ptr;

    fgets(line, sizeof(line), f);
    alarm_p->mode = strtol(&line[strlen("Mode: ") ], &ptr, 16);

    fgets(line, sizeof(line), f);
    alarm_p->enable = strtol(&line[strlen("Enable: ") ], &ptr, 16);

    fgets(line, sizeof(line), f);
    alarm_p->desc_len = strtol(&line[strlen("Description length: ") ], &ptr, 16);

    fgets(line, sizeof(line), f);
    strncpy((char*)alarm_p->desc, (char*)&line[strlen("Description: ")], alarm_p->desc_len);
    alarm_p->desc[alarm_p->desc_len] = '\0';

    fgets(line, sizeof(line), f);
    alarm_p->hour = strtol(&line[strlen("Hour: ") ], &ptr, 16);

    fgets(line, sizeof(line), f);
    alarm_p->minute = strtol(&line[strlen("Minute: ") ], &ptr, 16);

    switch(alarm_p->mode) 
    {
        case ALARM_SINGLE_MODE:
        {
            fgets(line, sizeof(line), f);
            alarm_p->args.single_alarm_args.day = strtol(&line[strlen("Day: ") ], &ptr, 16);

            fgets(line, sizeof(line), f);
            alarm_p->args.single_alarm_args.month = strtol(&line[strlen("Month: ") ], &ptr, 16);

            fgets(line, sizeof(line), f);
            alarm_p->args.single_alarm_args.year = strtol(&line[strlen("Year: ") ], &ptr, 16);
            break;
        }
            
        case ALARM_WEEKLY_MODE:
        {
            fgets(line, sizeof(line), f);
            alarm_p->args.days = strtol(&line[strlen("Days: ") ], &ptr, 16);
            break;
        }
            
        case ALARM_MONTHLY_MODE:
        {
            fgets(line, sizeof(line), f);
            alarm_p->args.day = strtol(&line[strlen("Day: ") ], &ptr, 16);
            break;
        }
            
        case ALARM_YEARLY_MODE:
        {
            fgets(line, sizeof(line), f);
            alarm_p->args.yearly_alarm_args.day = strtol(&line[strlen("Day: ") ], &ptr, 16);

            fgets(line, sizeof(line), f);
            alarm_p->args.yearly_alarm_args.month = strtol(&line[strlen("Month: ") ], &ptr, 16);
            
            break;
        }
    }

    fgets(line, sizeof(line), f);
    alarm_p->volume = strtol(&line[strlen("Volume: ") ], &ptr, 16);
    
    fclose(f);

    return ESP_OK;
}

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
object_type_t pp_object_manager_check_type(uint8_t *uuid)
{
    if(memcmp(uuid, alarm_type_uuid, ESP_UUID_LEN_128) == 0) return ALARM_TYPE;
    else if(memcmp(uuid, ringtone_type_uuid, ESP_UUID_LEN_128) == 0) return RINGTONE_TYPE;
    else return -1;
}

/** @brief pp_object_manager_print_file: Print the current object file content to UART.
 * 
 * This functions prints the current file content to UART.
 * 
 * @return
 */
void pp_object_manager_print_file(void)
{
    if(pp_object_manager_is_object_empty())
    {
        return;
    }

    FILE* f = pp_object_manager_open_file("r", current_object.id);

    char line[70];

    while(fgets(line, sizeof(line), f))
    {
        printf("%s", line);
    }

    fclose(f);
}

/** @brief pp_object_manager_printf_alarm_info: Print the current object file alarm properties content to UART.
 * 
 * This functions print the current file alarm properties content to UART.
 * 
 * @return
 */
void pp_object_manager_printf_alarm_info(void)
{
    if(pp_object_manager_is_object_empty()) return;

    FILE* f = pp_object_manager_open_file("r", current_object.id);
    fpos_t pos;
    bool found = pp_seekfor(f, "ALARM PROPERTIES\n", &pos);
    fclose(f);

    if(found)
    {
        alarm_mode_args_t alarm = get_alarm_values();
        printf("State: %s\n", alarm.enable?"enabled":"disabled");

        switch(alarm.mode)
        {
            case ALARM_SINGLE_MODE:
                printf("Mode: single\n");
                break;

            case ALARM_WEEKLY_MODE:
                printf("Mode: weekly\n");
                break;

            case ALARM_MONTHLY_MODE:
                printf("Mode: monthly\n");
                break;

            case ALARM_YEARLY_MODE:
                printf("Mode: yearly\n");
                break;
        }

        printf("Description length: %u\n", alarm.desc_len);
        printf("Description: %s\n", alarm.desc);
        printf("Hour: %u\n", alarm.hour);
        printf("Minute: %u\n", alarm.minute);

        switch(alarm.mode)
        {
            case ALARM_SINGLE_MODE:
                printf("Day: %u\n", alarm.args.single_alarm_args.day);
                printf("Month: %u\n", alarm.args.single_alarm_args.month);
                printf("Year: %u\n", alarm.args.single_alarm_args.year);
                break;
            case ALARM_WEEKLY_MODE:
                printf("Days: %u\n", alarm.args.days);
                break;
            case ALARM_MONTHLY_MODE:
                printf("Day: %u\n", alarm.args.day);
                break;
            case ALARM_YEARLY_MODE:
                printf("Day: %u\n", alarm.args.yearly_alarm_args.day);
                printf("Month: %u\n", alarm.args.yearly_alarm_args.month);
                break;
        }
        
        printf("Volume: %d\n", alarm.volume);
    }
}

/** @brief pp_object_manager_print_current_object: Print the current object data to UART.
 * 
 * This functions print the current object data to UART.
 * 
 * @return
 */
void pp_object_manager_print_current_object(void)
{
    if(pp_object_manager_is_object_empty())
    {
        return;
    }

    printf("\nSize: 0x%x\n", current_object.size);
    printf("Allocated size: 0x%x\n", current_object.alloc_size);
    printf("Name: %s\n", current_object.name);
    printf("Name length: %u\n", current_object.name_len);
    printf("UUID type: %u\n", current_object.type.len);
    printf("UUID: ");

    for(int i=15; i>=0; i--)
    {
        printf("%02x", current_object.type.uuid.uuid128[i]);
    }
    printf("\n");
    printf("ID: %llx\n", current_object.id);
    printf("Properties: 0x%" PRIx32 "\n\n", current_object.properties);

    if(pp_object_manager_check_type(current_object.type.uuid.uuid128) == ALARM_TYPE && current_object.set_custom_object)
    {
        pp_object_manager_printf_alarm_info();
    }
}

/** @brief pp_object_manager_open_file: Open a file by object ID
 * 
 * This functions opens a file by object ID with specified open options and
 * returns a pointer to opened file.
 * 
 * @return   (FILE*)  Pointer to opened file
 */
static FILE* pp_object_manager_open_file(const char* option,  uint64_t id)
{
    char file[20];
    strcpy(file, MOUNT_POINT);
    strcat(file, "/");
    itoa(id, &file[8], 16);
    ESP_LOGI(OBJECT_TAG, "Opened file path: %s", file);

    FILE* f = fopen(file, option);
    if(f == NULL)
    {
        ESP_ERROR_CHECK(ESP_FAIL);
    }

    return f;
}

/** @brief pp_object_manager_truncate_rest: Truncate the content of the object file content from profided offset
 * 
 * This functions truncates the content of the object file content from profided offset.
 * 
 * @param[in]   id      (uint64_t) Object ID
 * @param[in]   offset  (uint32_t) File offset
 * 
 * @return
 */
static void pp_object_manager_truncate_rest(uint64_t id, uint32_t offset)
{
    char file[20];
    strcpy(file, MOUNT_POINT);
    strcat(file, "/");
    itoa(id, &file[8], 16);
    truncate(file, offset);
    ESP_LOGI(OBJECT_TAG, "File: %s content truncated from offset %" PRIu32, file, offset);
}

/** @brief pp_object_manager_set_current_object_from_file: Set current object with values stored in file
 * 
 * This functions sets current object with values of object stored in file with provided ID.
 * 
 * @param[in]   id  (uint64_t) type of an object
 * 
 * @return
 */
static void pp_object_manager_set_current_object_from_file(uint64_t id)
{
    FILE* f = pp_object_manager_open_file("r", id);

    char line[50];
    char *ptr;

    fgets(line, sizeof(line), f);
    uint32_t size = strtol(&line[6], &ptr, 16);
    current_object.size = size;

    fgets(line, sizeof(line), f);
    uint32_t alloc_size = strtol(&line[16], &ptr, 16);
    current_object.alloc_size = alloc_size;

    fgets(line, sizeof(line), f);

    char name_len_str[20];
    fgets(name_len_str, sizeof(name_len_str), f);
    uint8_t name_len = atoi(&name_len_str[13]);
    current_object.name_len = name_len;

    strncpy(current_object.name, &line[6], name_len);
    current_object.name[name_len] = '\0';

    fgets(line, sizeof(line), f);
    uint8_t uuid_type = atoi(&line[11]);
    current_object.type.len = uuid_type;

    fgets(line, sizeof(line), f);
    char uuid_byte_str[3];
    uint16_t uuid_byte;
    for(int i=15; i>=0; i--)
    {
        strncpy(uuid_byte_str, &line[6+(15-i)*2], 2);
        uuid_byte_str[2] = '\0';
        uuid_byte = strtol(uuid_byte_str, &ptr, 16);
        current_object.type.uuid.uuid128[i] = uuid_byte;
    }

    fgets(line, sizeof(line), f);
    current_object.properties = strtol(&line[12], &ptr, 16);

    current_object.id = id;

    object_type_t ret_type = pp_object_manager_check_type(current_object.type.uuid.uuid128);

    switch(ret_type)
    {
        case ALARM_TYPE:
        {
            fpos_t pos;
            bool found = pp_seekfor(f, "ALARM PROPERTIES\n", &pos);
            if(!found)
            {
                current_object.set_custom_object = false;
                break;
            }

            current_object.set_custom_object = true;

            char line[70];
            char *ptr;

            fgets(line, sizeof(line), f);
            current_alarm.mode = strtol(&line[strlen("Mode: ") ], &ptr, 16);

            fgets(line, sizeof(line), f);
            current_alarm.enable = strtol(&line[strlen("Enable: ") ], &ptr, 16);

            fgets(line, sizeof(line), f);
            current_alarm.desc_len = strtol(&line[strlen("Description length: ") ], &ptr, 16);

            fgets(line, sizeof(line), f);
            strncpy((char*)current_alarm.desc, (char*)&line[strlen("Description: ")], current_alarm.desc_len);
            current_alarm.desc[current_alarm.desc_len] = '\0';

            fgets(line, sizeof(line), f);
            current_alarm.hour = strtol(&line[strlen("Hour: ") ], &ptr, 16);

            fgets(line, sizeof(line), f);
            current_alarm.minute = strtol(&line[strlen("Minute: ") ], &ptr, 16);

            switch(current_alarm.mode) 
            {
                case ALARM_SINGLE_MODE:
                {
                    fgets(line, sizeof(line), f);
                    current_alarm.args.single_alarm_args.day = strtol(&line[strlen("Day: ") ], &ptr, 16);

                    fgets(line, sizeof(line), f);
                    current_alarm.args.single_alarm_args.month = strtol(&line[strlen("Month: ") ], &ptr, 16);

                    fgets(line, sizeof(line), f);
                    current_alarm.args.single_alarm_args.year = strtol(&line[strlen("Year: ") ], &ptr, 16);
                    break;
                }
                    
                case ALARM_WEEKLY_MODE:
                {
                    fgets(line, sizeof(line), f);
                    current_alarm.args.days = strtol(&line[strlen("Days: ") ], &ptr, 16);
                    break;
                }
                    
                case ALARM_MONTHLY_MODE:
                {
                    fgets(line, sizeof(line), f);
                    current_alarm.args.day = strtol(&line[strlen("Day: ") ], &ptr, 16);
                    break;
                }
                    
                case ALARM_YEARLY_MODE:
                {
                    fgets(line, sizeof(line), f);
                    current_alarm.args.yearly_alarm_args.day = strtol(&line[strlen("Day: ") ], &ptr, 16);

                    fgets(line, sizeof(line), f);
                    current_alarm.args.yearly_alarm_args.month = strtol(&line[strlen("Month: ") ], &ptr, 16);
                    
                    break;
                }
            }

            fgets(line, sizeof(line), f);
            current_alarm.volume = strtol(&line[strlen("Volume: ") ], &ptr, 16);
            
            break;
        }

        case RINGTONE_TYPE:
            ESP_LOGI(OBJECT_TAG, "Requested object type: Ringtone file");
            break;
    }

    fclose(f);
}

/** @brief pp_seekfor: Seeks for provided string in the file.
 * 
 * This functions seeks for provided string in the file and sets its position in 'pos' input argument.
 * 
 * @param[in]   stream  (FILE*)         Pointer to file
 * @param[in]   str     (const char*)   String to seek
 * @param[out]  pos     (fpos_t*)       Position of the string
 * 
 * @return   (bool)  True if string found, False otherwise
 */
static bool pp_seekfor(FILE *stream, const char* str, fpos_t *pos)
{
    if(stream == NULL || str == NULL || pos == NULL)
    {
        ESP_ERROR_CHECK(ESP_FAIL);
    }

    char line[70];
    bool present = false;
    while(fgets(line, sizeof(line), stream))
    {
        if(strcmp(line, str)==0)
        {
            present = true;
            break;
        }
        fgetpos(stream, pos);
    }

    return present;
}

/** @brief id_to_string: Convert object ID into decimal string.
 * 
 * This functions converts object ID into decimal string.
 * 
 * @param[in]   bfr (char*)     Pointer to string to store
 * @param[in]   id  (uint64_t)  Object ID
 * 
 * @return   (bool)  True if string found, False otherwise
 */
static char* id_to_string(char* bfr, uint64_t id)
{
    if(bfr == NULL)
    {
        ESP_ERROR_CHECK(ESP_FAIL);
    }

    uint8_t digit_count = 0;

    do
    {
        bfr[19 - digit_count] = (char)((id % 10) + '0');
        id = id / 10;
        ++digit_count;

    } while(id);

    for(int i = 0; i < digit_count; ++i)
    {
        bfr[i] = bfr[20-digit_count];
    }

    bfr[digit_count] = '\0';

    return bfr;
}

/***************************************************************************************************************/
/* Functions - Object list filtering and order */

/** @brief pp_object_list_get_new_id: Get new ID for an object
 * 
 * This functions searches for the highest ID in object list and provides new incremented value.
 * 
 * @return (uint64_t) 0 if object list is empty otherwise new the highest object ID
 */
uint64_t pp_object_list_get_new_id(void)
{
    uint64_t max = 0;

    if(alarm_count + ringtone_count == 0)
    {
        max_id = 0;
        return 0;
    }

    if(int i = 0; i < alarm_count + ringtone_count; ++i)
    {
        if(object_list[i].id > max)
        {
            max = object_list[i].id;
        }
    }

    max_id = max + 1;
    return max_id;
}

/** @brief pp_object_list_delete: Delete object from object list at choosen index
 * 
 * This functions deletes an object at the choosen index by moving there the last object in list.
 * !!! This function destroys filtering and order of the list so the function pp_filter_order_make_list
 * has to be called after this function.
 * 
 * @param[in]   index  (uint32_t) Object index in the object list
 * 
 * @return (esp_err_t) ESP_FAIL if index out of range.
 */
esp_err_t pp_object_list_delete(uint32_t index)
{
    if(index >= alarm_count + ringtone_count)
    {
        return ESP_FAIL;
    }

    if(object_list[index].type == ALARM_TYPE)
    {
        --alarm_count;
    }
    else if(object_list[index].type == RINGTONE_TYPE)
    {
        --ringtone_count;
    }

    object_id_array_t temp = object_list[index];
    object_list[index] = object_list[alarm_count + ringtone_count - i];

    if(temp.id == max_id)
    {
        max_id = pp_object_list_get_new_id();
    }
    
    return ESP_OK;
}

/** @brief pp_object_list_delete_by_id: Delete object with specified ID from object list
 * 
 * This functions deletes an object with specified ID by moving there the last object in list.
 * !!! This function destroys filtering and order of the list so the function pp_filter_order_make_list
 * has to be called after this function.
 * 
 * @param[in]   id  (uint64_t) ID of an object
 * 
 * @return (esp_err_t) ESP_FAIL if ID not found
 */
esp_err_t pp_object_list_delete_by_id(uint64_t id)
{
    for(int i = 0; i < alarm_count + ringtone_count; ++i)
    {
        if(object_list[i].id == id)
        {
            object_id_array_t temp = object_list[i];
            object_list[i] = object_list[alarm_count + ringtone_count - i];

            if(temp.id == max_id)
            {
                max_id = pp_object_list_get_new_id();
            }

            if(object_list[i].type == ALARM_TYPE)
            {
                --alarm_count;
            }
            else if(object_list[i].type == RINGTONE_TYPE)
            {
                --ringtone_count;
            }
            else
            {
                break;
            }

            return ESP_OK;
        }
    }
    
    return ESP_FAIL;
}

/** @brief pp_object_list_add: Adds new object with specified type to the list.
 * 
 * This functions adds an object with specified type at the of the object list.
 * !!! This function destroys filtering and order of the list so the function pp_filter_order_make_list
 * has to be called after this function.
 * 
 * @param[in]   type  (object_type_t) Type of an object
 * 
 * @return (uint64_t) ID of an object
 */
uint64_t pp_object_list_add(object_type_t type)
{
    ++max_id;
    objects[alarm_count + ringtone_count].type = type;
    objects[alarm_count + ringtone_count].id = max_id;

    if(type == ALARM_TYPE)
    {
        ++alarm_count;
    }
    else if(type == RINGTONE_TYPE)
    {
        ++ringtone_count;
    }

    return max_id;
}

/** @brief pp_object_list_add_by_id: Adds new object with specified type and ID to the list.
 * 
 * This functions adds an object with specified type and ID at the of the object list.
 * !!! This function destroys filtering and order of the list so the function pp_filter_order_make_list
 * has to be called after this function. It also does not check if an object with provided ID is already
 * in the object list. This function is meant to be used only during initialization to create list of objects
 * from NVM after power-on.
 * 
 * @param[in]   type  (object_type_t) Type of an object
 * 
 * @return (uint64_t) ID of an object
 */
uint64_t pp_object_list_add_by_id(object_type_t type, uint64_t id)
{
    objects[alarm_count + ringtone_count].type = type;
    objects[alarm_count + ringtone_count].id = id;

    if(type == ALARM_TYPE)
    {
        ++alarm_count;
    }
    else if(type == RINGTONE_TYPE)
    {
        ++ringtone_count;
    }

    if(id > max_id) max_id = id;

    return id;
}

/** @brief pp_object_list_search: Search object with specified ID in object list
 * 
 * This functions searches for on object with specified ID in the filtered or all object list
 * and return its index.
 * 
 * @param[in]   id  (uint64_t) ID of an object
 * @param[in]   filtered  (bool) True - search in list after filter
 *                               False - search in all objects
 * 
 * @return (int) -1 if object not found, otherwise index of an object
 */
int pp_object_list_search(uint64_t id, bool filtered)
{
    int idx = -1;
    int end_idx;

    if(filtered)
    {
        end_idx = unfiltered_end_idx;
    }
    else
    {
        end_idx = alarm_count + ringtone_count - 1;
    }

    for(int i = 0; i <= end_idx; ++i)
    {
        if(object_list[i].id == id)
        {
            idx = i;
            break;
        }
    }

    return idx;
}

/** @brief pp_object_list_make_list: Filters and sorts the list using current filter and order options.
 * 
 * This functions performs filtering and sorting of the object lists by using current choosen
 * filtering and ordering options.
 * 
 * @return
 */
void pp_object_list_make_list(void)
{
    ESP_LOGI(TAG, "ORDER OP Code: %x", order);
    ESP_LOGI(TAG, "Filter OP Code: %x", filter.type);

    if(filter.type < FILTER_RANGE && filter_func_table[filter.type]) pp_object_list_filter(filter_func_table[filter.type]);
    if(order < ORDER_RANGE && compare_func_table[filter.type]) pp_object_list_sort(compare_func_table[order], compare_func_table_asc[order]);

    ESP_LOGE(TAG, "Sorting and filtering done");
}

/** @brief pp_object_list_sort: Sorts the list with choosen compare function and order.
 * 
 * This functions performs sorting of the object lists by using current choosen
 * sorting and order options.
 * 
 * @param[in]   fun  (compare_function) Pointer to the compare function
 * @param[in]   asc  (bool) True if ascending, False if descending
 * 
 * @return
 */
static void pp_object_list_sort(compare_function fun, bool asc)
{
    int swapped;
    uint32_t r_idx = unfiltered_end_idx >= 0 ? 0 : -1;
    uint32_t l_idx = -1;

    /* Checking for empty list */
    if (r_idx < 0>)
        return;

    do
    {
        swapped = 0;
        r_idx = unfiltered_end_idx >= 0 ? 0 : -1;

        while (r_idx + 1 <= unfiltered_end_idx && r_idx + 1 != l_idx)
        {
            if (fun(object_list[r_idx].id, object_list[r_idx + 1].id, asc) > 0)
            {
                object_id_array_t temp = object_list[r_idx];
                object_list[r_idx] = object_list[r_idx + 1];
                object_list[r_idx + 1] = temp;

                swapped = 1;
            }
            ++r_idx;
        }
        l_idx = r_idx;
    }
    while (swapped);

}

/** @brief pp_object_list_compare_name: Object name compare function
 * 
 * This functions performs comparison of the names of 2 objects with provided IDs and order
 * 
 * @param[in]   rID  (uint64_t) ID of the first object
 * @param[in]   lID  (uint64_t) ID of the second object
 * @param[in]   asc  (bool) ID True if ascending, False if descending order
 * 
 * @return  (int) 0 if the same, 
 *                > 0 if first object after the second in alhanumeric order
 *                < 0 if before object after the second in alhanumeric order
 */
static int pp_object_list_compare_name(uint64_t rID, uint64_t lID, bool asc)
{
    char rName[NAME_LEN_MAX];
    char lName[NAME_LEN_MAX];

    pp_object_list_read_name_from_file(rName, rID);
    pp_object_list_read_name_from_file(lName, lID);

    uint8_t rName_len = strlen(rName);
    uint8_t lName_len = strlen(lName);
    uint8_t len = (rName_len-lName_len)?lName_len:rName_len;
    int16_t cmp;
    bool right_is_first;
    bool compared = false;

    for(int i=0; i<len; i++)
    {
        cmp = tolower(rName[i]) - tolower(lName[i]);
        if(cmp < 0)
        {
            right_is_first = true;
            compared = true;
            break;
        }
        else if(cmp > 0)
        {
            right_is_first = false;
            compared = true;
            break;
        }
        else
        {

        }
    }

    if(compared == false)
    {
        for(int i=0; i<len; i++)
        {
            if(rName[i] > lName[i])
            {
                right_is_first = true;
                compared = true;
                break;
            }
            else if(rName[i] < lName[i])
            {
                right_is_first = false;
                compared = true;
                break;
            }
        }
    }

    if(compared == false)
    {
        if(rName_len < lName_len)
        {
            right_is_first = true;
        }
        else
        {
            right_is_first = false;
        }
    }

    if(right_is_first) cmp = -1;
    else cmp = 1;
    if(asc == false) cmp = -cmp;

    return cmp;
}

/** @brief pp_object_list_compare_type: Object type compare function
 * 
 * This functions performs comparison of the type of 2 objects with provided IDs and order
 * 
 * @param[in]   rID  (uint64_t) ID of the first object
 * @param[in]   lID  (uint64_t) ID of the second object
 * @param[in]   asc  (bool) ID True if ascending, False if descending order
 * 
 * @return  (int) 0 if the same, 
 *                > 0 if type of first object is greater (lower if order descending)
 *                < 0 if type of first object is lower (greater if order descending)
 */
static int pp_object_list_compare_type(uint64_t rID, uint64_t lID, bool asc)
{
    uint8_t rType[16];
    uint8_t lType[16];

    pp_object_list_read_type_from_file(rType, rID);
    pp_object_list_read_type_from_file(lType, lID);

    int cmp = memcmp(rType, lType, 16);
    if(asc == false) cmp = -cmp;

    return cmp;
}

/** @brief pp_object_list_compare_size: Object size compare function
 * 
 * This functions performs comparison of the size of 2 objects with provided IDs and order
 * 
 * @param[in]   rID  (uint64_t) ID of the first object
 * @param[in]   lID  (uint64_t) ID of the second object
 * @param[in]   asc  (bool) ID True if ascending, False if descending order
 * 
 * @return  (int) 0 if the same, 
 *                > 0 if size of first object is greater (lower if order descending)
 *                < 0 if size of first object is lower (greater if order descending)
 */
static int pp_object_list_compare_size(uint64_t rID, uint64_t lID, bool asc)
{
    uint32_t rSize = pp_object_list_read_current_size_from_file(rID);
    uint32_t lSize = pp_object_list_read_current_size_from_file(lID);

    int cmp;
    if(rSize>lSize) cmp = 1;
    else if(rSize<lSize) cmp = -1;
    else cmp = 0;

    if(asc == false) cmp = -cmp;

    return cmp;
}

/** @brief pp_object_list_read_name_from_file: Reads object name from the file
 * 
 * This functions reads name of the object with specified ID from the file
 * and stores the result.
 * 
 * @param[in]   id    (uint64_t) ID of the object
 * @param[out]  dest  (char*) Pointer to where object name will be stored
 * 
 * @return  (char*) Pointer to the stored object name
 */
static char* pp_object_list_read_name_from_file(char* dest, uint64_t id)
{
    if(dest == NULL)
    {
        ESP_ERROR_CHECK(ESP_FAIL);
    }

    FILE* f = pp_object_manager_open_file("r", id);

    char line[50];

    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);

    char name_len_str[20];
    fgets(name_len_str, sizeof(name_len_str), f);
    uint8_t name_len = atoi(&name_len_str[13]);

    strncpy(dest, &line[6], name_len);
    dest[name_len] = '\0';
    fclose(f);

    return dest;
}

/** @brief pp_object_list_read_type_from_file: Reads object type from the file
 * 
 * This functions reads type of the object with specified ID from the file
 * and stores the result.
 * 
 * @param[in]   id    (uint64_t) ID of the object
 * @param[out]  dest  (uint8_t*) Pointer to where object type will be stored
 * 
 * @return  (uint8_t*) Pointer to the stored object type
 */
static uint8_t* pp_object_list_read_type_from_file(uint8_t* dest, uint64_t id)
{
    if(dest == NULL)
    {
        ESP_ERROR_CHECK(ESP_FAIL);
    }

    FILE* f = pp_object_manager_open_file("r", id);
    char line[50];

    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);

    char *ptr;
    char uuid_byte_str[3];
    uint8_t uuid_byte;
    for(int i=15; i>=0; i--)
    {
        strncpy(uuid_byte_str, &line[6+(15-i)*2], 2);
        uuid_byte_str[2] = '\0';
        uuid_byte = strtol(uuid_byte_str, &ptr, 16);
        dest[i] = uuid_byte;
    }

    fclose(f);

    return dest;
}

/** @brief pp_object_list_read_current_size_from_file: Reads object current size from the file
 * 
 * This functions reads current size of the object with specified ID from the file
 * and stores the result.
 * 
 * @param[in]   id    (uint64_t) ID of the object
 * 
 * @return  (uint32_t) Current size of the object
 */
static uint32_t pp_object_list_read_current_size_from_file(uint64_t id)
{
    FILE* f = pp_object_manager_open_file("r", id);
    char line[50];

    fgets(line, sizeof(line), f);

    char *ptr;
    uint32_t size = strtol(&line[6], &ptr, 16);

    fclose(f);

    return size;
}

/** @brief pp_object_list_filter: Filters the list with choosen filter function.
 * 
 * This functions performs filtering of the object lists by using current choosen
 * filtering option
 * 
 * @param[in]   fun  (filter_function) Pointer to the filter function
 * 
 * @return
 */
static void pp_object_list_filter(filter_function fun)
{
    uint32_t end_idx = alarm_count + ringtone_count - 1;
    uint32_t idx = 0;

    while(idx <= end_idx)
    {
        if(!fun(object_list[idx].id))
        {
            object_id_array_t temp = object_list[idx];
            object_list[idx] = object_list[end_idx];
            object_list[end_idx] = temp;
            --end_idx;
        }
        else
        {
            ++idx;
        }
    }

    unfiltered_end_idx = end_idx;
}

/** @brief pp_object_list_no_filter: No filter
 * 
 * This functions just returns true, because there is no filter.
 * 
 * @param[in]   id    (uint64_t) ID of the object - Not used
 * 
 * @return  (bool) Always true - no filter
 */
static bool pp_object_list_no_filter(uint64_t id)
{
    return true;
}

/** @brief pp_object_list_name_starts_with: Check if name of the object starts with filter argument
 * 
 * This functions reads the name of an object with specified ID from the file
 * and checks if the name starts with filter argument.
 * 
 * @param[in]   id    (uint64_t) ID of the object
 * 
 * @return  (bool) True if name starts with filter string, False otherwise
 */
static bool pp_object_list_name_starts_with(uint64_t id)
{
    char name[NAME_LEN_MAX];
    pp_read_name_from_file(name, id);
    uint8_t name_len = strlen(name);

    if(filter.par_length > name_len)
    {
        return false;
    }

    if(strncmp(name, (char*)filter.parameter, filter.par_length))
    {
        return false;
    }

    return true;
}

/** @brief pp_object_list_name_ends_with: Check if name of the object ends with filter argument
 * 
 * This functions reads the name of an object with specified ID from the file
 * and checks if the name ends with filter argument.
 * 
 * @param[in]   id    (uint64_t) ID of the object
 * 
 * @return  (bool) True if name ends with filter string, False otherwise
 */
static bool pp_object_list_name_ends_with(uint64_t id)
{
    char name[NAME_LEN_MAX];
    pp_read_name_from_file(name, id);
    uint8_t name_len = strlen(name);

    if(filter.par_length > name_len)
    {
        return false;
    }

    if(strncmp(&name[name_len-filter.par_length], (char*)filter.parameter, filter.par_length))
    {
        return false;
    }

    return true;
}

/** @brief pp_object_list_name_containts: Check if name of the object contains filter argument
 * 
 * This functions reads the name of an object with specified ID from the file
 * and checks if the name contains filter argument.
 * 
 * @param[in]   id    (uint64_t) ID of the object
 * 
 * @return  (bool) True if name contains filter string, False otherwise
 */
static bool pp_object_list_name_containts(uint64_t id)
{
    char name[NAME_LEN_MAX];
    pp_read_name_from_file(name, id);
    uint8_t name_len = strlen(name);

    if(filter.par_length > name_len)
    {
        return false;
    }

    char name_search[NAME_LEN_MAX];
    strncpy(name_search, (char*)filter.parameter, filter.par_length);
    name_search[filter.par_length] = '\0';

    if(strstr(name, name_search) == NULL)
    {
        return false;
    }

    return true;
}

/** @brief pp_object_list_name_is_exactly: Check if name of the object is the same as filter argument
 * 
 * This functions reads the name of an object with specified ID from the file
 * and checks if the name is the same as filter argument.
 * 
 * @param[in]   id    (uint64_t) ID of the object
 * 
 * @return  (bool) True if name is the same as filter argument, False otherwise
 */
static bool pp_object_list_name_is_exactly(uint64_t id)
{
    char name[NAME_LEN_MAX];
    pp_read_name_from_file(name, id);
    uint8_t name_len = strlen(name);

    if(filter.par_length != name_len)
    {
        return false;
    }

    char name_search[NAME_LEN_MAX];
    strncpy(name_search, (char*)filter.parameter, filter.par_length);
    name_search[filter.par_length] = '\0';

    if(strcmp(name, name_search))
    {
        return false;
    }

    return true;
}

/** @brief pp_object_list_object_type: Check if type of the object is the same as filter argument
 * 
 * This functions reads the name of an object with specified ID from the file
 * and checks if the type is the same as filter argument.
 * 
 * @param[in]   id    (uint64_t) ID of the object
 * 
 * @return  (bool) True if type is the same as filter argument, False otherwise
 */
static bool pp_object_list_object_type(uint64_t id)
{
    uint8_t uuid[16];
    pp_read_type_from_file(uuid, id);

    if(memcmp(uuid, filter.parameter, 16))
    {
        return false;
    }

    return true;
}

/** @brief pp_object_list_current_size_between: Check if current size of the object is between filter argument range
 * 
 * This functions reads the name of an object with specified ID from the file
 * and checks if the current size is between filter argument range
 * 
 * @param[in]   id    (uint64_t) ID of the object
 * 
 * @return  (bool) True if current size in filter argument range, False otherwise
 */
static bool pp_object_list_current_size_between(uint64_t id)
{
    uint32_t current_size = pp_read_current_size_from_file(id);
    uint32_t size_left, size_right;

    memcpy(&size_left, filter.parameter, 4);
    memcpy(&size_right, &filter.parameter[4], 4);

    if(current_size < size_left || current_size > size_right)
    {
        return true;
    }

    return false;
}

/** @brief pp_object_list_alloc_size_between: Check if allocated size of the object is between filter argument range
 * 
 * This functions reads the name of an object with specified ID from the file
 * and checks if the allocated size is between filter argument range
 * 
 * @param[in]   id    (uint64_t) ID of the object
 * 
 * @return  (bool) True if allocated size in filter argument range, False otherwise
 */
static bool pp_object_list_alloc_size_between(uint64_t id)
{
    FILE* f = pp_object_manager_open_file("r", id);
    char line[50];

    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);

    char *ptr;
    uint32_t alloc_size = strtol(&line[16], &ptr, 16);
    fclose(f);

    uint32_t size_left, size_right;

    memcpy(&size_left, filter.parameter, 4);
    memcpy(&size_right, &filter.parameter[4], 4);

    if(alloc_size < size_left || alloc_size > size_right)
    {
        return false;
    }

    return true;
}

/** @brief pp_object_list_marked_objects: Check if object is marked
 * 
 * This functions reads the properties of an object with specified ID from the file
 * and checks if the object is marked.
 * 
 * @param[in]   id    (uint64_t) ID of the object
 * 
 * @return  (bool) True if object is marked, False otherwise
 */
static bool pp_object_list_marked_objects(uint64_t id)
{
    FILE* f = pp_object_manager_open_file("r", id);
    char line[50];

    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);

    char *ptr;
    uint32_t properties = strtol(&line[12], &ptr, 16);

    fclose(f);

    if(properties & PROPERTY_MARK)
    {
        return false;
    }

    return true;
}

/** @brief pp_object_list_get_filter: Get current filter options
 * 
 * This functions return current filter options.
 * 
 * @return  (ListFilter_t*) Pointer to the filter object
 */
ListFilter_t* pp_object_list_get_filter(void)
{
    return &filter;
}

/** @brief pp_object_list_get_order: Get current order options
 * 
 * This functions return current order options.
 * 
 * @return  (uint8_t) Current order code.
 */
uint8_t pp_object_list_get_order(void)
{
    return order;
}

/** @brief pp_object_list_get_how_many: Get quantity of all objects
 * 
 * This functions return quantity of all objects.
 * 
 * @return  (uint8_t) Quantity of all objects.
 */
uint8_t pp_object_list_get_how_many(void)
{
    return alarm_count + ringtone_count = 0;
}

/** @brief pp_object_list_get_objects_array: Get pointer to object array
 * 
 * This functions return pointer to object array.
 * 
 * @return  (object_id_array_t *) Pointer to object array.
 */
object_id_array_t * pp_object_list_get_objects_array(void)
{
    return object_list;
}
