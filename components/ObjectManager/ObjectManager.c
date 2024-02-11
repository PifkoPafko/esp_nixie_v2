#include "ObjectManager.h"
#include "ObjectManagerIdList.h"
#include "ObjectTransfer_defs.h"
#include "FilterOrder.h"
#include "project_defs.h"

#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "esp_log.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "FreeRTOSConfig.h"

#include "time.h"
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#define OBJECT_TAG "FILESYSTEM"
#define MAX_FILES_NUMBER 5

static object_t *current_object = NULL;
    
static file_transfer_t file_transfer = {
    .bytes_done = 0,
    .status = WAIT_FOR_ACTION,
};

uint8_t alarm_type_uuid[ESP_UUID_LEN_128] = {0x02, 0x00, 0x12, 0xAC, 0x42, 0x02, 0x61, 0xA2, 0xED, 0x11, 0xBA, 0x29, 0xB8, 0x13, 0x08, 0xCC};
uint8_t ringtone_type_uuid[ESP_UUID_LEN_128] = {0x03, 0x00, 0x12, 0xAC, 0x42, 0x02, 0x61, 0xA2, 0xED, 0x11, 0xBA, 0x29, 0xB8, 0x13, 0x08, 0xCC};
static char* id_to_string(char* bfr, uint64_t id);
static void ObjectManager_print_file();
static void ObjectManager_print_current_object();
static void ObjectManager_set_current_object_from_file(uint64_t id);

static esp_err_t ObjectManager_init_list()
{
    FILE* stream = fopen(FILE_LIST_NAME, "a+");
    fseek( stream, 0, SEEK_SET );
    char line[50];
    uint64_t id;
    char *ptr;

    while(fgets(line, sizeof(line), stream))
    {
        ESP_LOGI(OBJECT_TAG, "Line: %s", line);
        id = strtoull(line, &ptr, 16);

        if(id)
        {
            ESP_LOGI(OBJECT_TAG, "File found: " MOUNT_POINT "%llx.txt", id);
            ObjectManager_list_add_by_id(id);
            
        }
    }
    fclose(stream);

    FilterOrder_make_list();

    return ESP_OK;
}

esp_err_t ObjectManager_init(void)
{   
    
    esp_err_t ret = ObjectManager_init_list();
    if (ret)
    {
        ESP_LOGE(OBJECT_TAG, "Object Manager list initialization fail. err=%d", ret);
        return ret;
    }

    return ESP_OK;
}

object_t* ObjectManager_get_object(void)
{
    return current_object;
}

void ObjectManager_null_current_object(void)
{
    current_object = NULL;
}

esp_err_t ObjectManager_create_object(uint32_t size, esp_bt_uuid_t type, oacp_op_code_result_t *result)
{
    ESP_LOGI(OBJECT_TAG, "Requested object type UUID:");
    if(type.len == ESP_UUID_LEN_16) ESP_LOGI(OBJECT_TAG, "%02x", type.uuid.uuid16);
    else if(type.len == ESP_UUID_LEN_128) ESP_LOG_BUFFER_HEX_LEVEL(OBJECT_TAG, type.uuid.uuid128, ESP_UUID_LEN_128, ESP_LOG_INFO);


    if(type.len == ESP_UUID_LEN_16)
    {
        ESP_LOGE(OBJECT_TAG, "Unsupported object type with 16-bit UUID");
        *result = OACP_RES_UNSUPPORTED_TYPE;
        return ESP_OK;
    }

    int ret_type = ObjectManager_check_type(type.uuid.uuid128);

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
            *result = OACP_RES_UNSUPPORTED_TYPE;
            return ESP_OK;
    }

    *result = OACP_RES_SUCCESS;

    ESP_LOGI(OBJECT_TAG, "Creating list object");
    object_id_list_t* object = ObjectManager_list_add();
    ESP_LOGI(OBJECT_TAG, "Object created, ID: %" PRIx64, object->id);

    if(current_object == NULL)
    {
        current_object = (object_t*)malloc(sizeof(object_t));
    }

    current_object->size = 0;
    current_object->alloc_size = 0;
    current_object->name[0] = '\0';
    current_object->name_len = 0;
    current_object->type.len = ESP_UUID_LEN_128;
    memcpy(current_object->type.uuid.uuid128, type.uuid.uuid128, ESP_UUID_LEN_128);
    current_object->id = object->id;
    current_object->properties = PROPERTY_ALL;

    ESP_LOGI(OBJECT_TAG, "Creating file on SD Card");
    FILE* f = ObjectManager_open_file("w+", object->id);

    fprintf(f, "Size: %x\n", current_object->size);
    fprintf(f, "Allocated size: %x\n", current_object->alloc_size);
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
    ESP_LOGI(OBJECT_TAG, "File created: %" PRIx64, object->id);

    
    f = fopen(FILE_LIST_NAME, "a+");

    char id_string[20];
    fprintf(f, "%s\n", id_to_string(id_string, object->id));
    fclose(f);

    switch(ret_type)
    {
        case ALARM_TYPE:
            current_object->set_custom_object = false;
            break;

        case RINGTONE_TYPE:
            ESP_LOGI(OBJECT_TAG, "Ringtone files not supported yet");
            break;
    }

    ESP_LOGI(OBJECT_TAG, "ID inserted into the list");

    FilterOrder_make_list();
    ObjectManager_print_current_object();

    return ESP_OK;
}

esp_err_t ObjectManager_delete_object(oacp_op_code_result_t *result)
{
    if(current_object == NULL)
    {
        ESP_LOGE(OBJECT_TAG, "Invalid current object");
        *result = OACP_RES_INVALID_OBJECT;
        return ESP_OK;
    }

    if((current_object->properties & PROPERTY_DELETE) == 0)
    {
        ESP_LOGE(OBJECT_TAG, "Procedure not permitted");
        *result = OACP_RES_PROCEDURE_NOT_PERMIT;
        return ESP_OK;
    }

    char file[20];
    strcpy(file, MOUNT_POINT);
    strcat(file, "/");
    itoa(current_object->id, &file[8], 16);
    ESP_LOGI(OBJECT_TAG, "File to remove: %s", file);
    remove(file);
    ESP_LOGI(OBJECT_TAG, "File removed");

    ESP_LOGI(OBJECT_TAG, "Deleting id from id file list");
    FILE* file_src = fopen(FILE_LIST_NAME, "r");

    char line[50];
    char *ptr;

    unsigned int line_number = 0;

    while(fgets(line, sizeof(line), file_src))
    {
        line_number++;
        if (current_object->id == strtol(line, &ptr, 16))
        {
            ESP_LOGI(OBJECT_TAG, "Line number: %u", line_number);
            break;
        }
    }
    fseek(file_src, 0, SEEK_SET);

    unsigned int line_counter = 0;
    FILE* file_dest = fopen(TEMP_FILE_PATH, "w");
    while(fgets(line, sizeof(line), file_src))
    {
        line_counter++;
        if (line_number != line_counter)
        {
            fputs(line, file_dest);
        }
    }

    fclose(file_src);
    fclose(file_dest);
    remove(FILE_LIST_NAME);
    rename(TEMP_FILE_PATH, FILE_LIST_NAME);

    *result = OACP_RES_SUCCESS;

    ESP_LOGI(OBJECT_TAG, "ID to remove from list: %llx", current_object->id);
    ObjectManager_list_delete_by_id(current_object->id);
    ESP_LOGI(OBJECT_TAG, "ID removed from list");
    FilterOrder_make_list();

    return ESP_OK;
}

esp_err_t ObjectManager_change_name_in_file()
{
    FILE* f = ObjectManager_open_file("r+", current_object->id);

    char line[70];

    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);
    fseek(f, strlen("Name: "), SEEK_CUR);
    fputs(current_object->name, f);

    for(uint8_t i=0; i<NAME_LEN_MAX-strlen(current_object->name)-1; i++)
    {
        fputc('*', f);
    }

    fgets(line, sizeof(line), f);
    fseek(f, strlen("Name length: "), SEEK_CUR);

    char name_len_string[5];
    itoa(current_object->name_len, name_len_string, 10);

    if(strlen(name_len_string) == 1)
    {
        fputc('0', f);
    }
    fputs(name_len_string, f);
    fclose(f);

    ObjectManager_print_current_object();

    return ESP_OK;
}

esp_err_t ObjectManager_first_object(olcp_op_code_result_t *result)
{
    object_id_list_t* object = ObjectManager_sort_list_first_elem();

    if(object == NULL)
    {
        ESP_LOGI(OBJECT_TAG, "List is empty");
        *result = OLCP_RES_NO_OBJECT;
        return ESP_OK;
    }

    if(current_object == NULL)
    {
        current_object = (object_t*)malloc(sizeof(object_t));
    }

    ObjectManager_set_current_object_from_file(object->id);
    *result = OLCP_RES_SUCCESS;

    ObjectManager_print_current_object();
    ObjectManager_print_file();
    return ESP_OK;
}

esp_err_t ObjectManager_last_object(olcp_op_code_result_t *result)
{
    object_id_list_t* object = ObjectManager_sort_list_last_elem();

    if(object == NULL)
    {
        ESP_LOGI(OBJECT_TAG, "List is empty");
        *result = OLCP_RES_NO_OBJECT;
        return ESP_OK;
    }

    if(current_object == NULL)
    {
        current_object = (object_t*)malloc(sizeof(object_t));
    }

    ObjectManager_set_current_object_from_file(object->id);
    *result = OLCP_RES_SUCCESS;

    ObjectManager_print_current_object();
    return ESP_OK;
}

esp_err_t ObjectManager_next_object(olcp_op_code_result_t *result)
{
    if(current_object == NULL)
    {
        ESP_LOGI(OBJECT_TAG, "Current object is invalid");
        *result = OLCP_RES_OPERATION_FAILED;
        return ESP_OK;
    }

    object_id_list_t *object = ObjectManager_list_search(true, current_object->id);

    if(object->next == NULL)
    {
        ESP_LOGI(OBJECT_TAG, "Next object is out of the bonds");
        *result = OLCP_RES_OUT_OF_THE_BONDS;
        return ESP_OK;
    }

    object = object->next;

    ObjectManager_set_current_object_from_file(object->id);
    *result = OLCP_RES_SUCCESS;

    ObjectManager_print_current_object();
    ObjectManager_print_file();
    return ESP_OK;
}

esp_err_t ObjectManager_previous_object(olcp_op_code_result_t *result)
{
    if(current_object == NULL)
    {
        ESP_LOGI(OBJECT_TAG, "Current object is invalid");
        *result = OLCP_RES_OPERATION_FAILED;
        return ESP_OK;
    }

    object_id_list_t *object = ObjectManager_list_search(true, current_object->id);

    if(object->prev == NULL)
    {
        ESP_LOGI(OBJECT_TAG, "Previous object is out of the bonds");
        *result = OLCP_RES_OUT_OF_THE_BONDS;
        return ESP_OK;
    }

    object = object->prev;

    ObjectManager_set_current_object_from_file(object->id);
    *result = OLCP_RES_SUCCESS;

    ObjectManager_print_current_object();
    return ESP_OK;
}

esp_err_t ObjectManager_goto_object(uint64_t id, olcp_op_code_result_t *result)
{
    object_id_list_t *object = ObjectManager_sort_list_first_elem();
    if(object == NULL)
    {
        ESP_LOGI(OBJECT_TAG, "No objects on the server");
        *result = OLCP_RES_NO_OBJECT;
        return ESP_OK;
    }

    object = ObjectManager_list_search(true, id);

    if(object == NULL)
    {
        ESP_LOGI(OBJECT_TAG, "Object not found");
        *result = OLCP_RES_OBJECT_NOT_FOUND;
        return ESP_OK;
    }

    ObjectManager_set_current_object_from_file(object->id);
    *result = OLCP_RES_SUCCESS;

    ObjectManager_print_current_object();
    return ESP_OK;
}

esp_err_t ObjectManager_order_object(uint8_t type, olcp_op_code_result_t *result)
{

    return ESP_OK;
}

esp_err_t ObjectManager_request_number(uint32_t *number, olcp_op_code_result_t *result)
{
    object_id_list_t* object = ObjectManager_sort_list_first_elem();

    while(object)
    {
        (*number)++;
        object = object->next;
    }

    ESP_LOGI(OBJECT_TAG, "Number of objects: %" PRIu32, *number);
    *result = OLCP_RES_SUCCESS;
    return ESP_OK;
}

esp_err_t ObjectManager_clear_marking(olcp_op_code_result_t *result)
{
    object_id_list_t* object = ObjectManager_sort_list_first_elem();

    while(object)
    {
        FILE* f = ObjectManager_open_file("r+", object->id);

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

        object = object->next;
    }


    ESP_LOGI(OBJECT_TAG, "Clearing markings done");
    *result = OLCP_RES_SUCCESS;

    return ESP_OK;
}

esp_err_t ObjectManager_change_properties_in_file()
{
    FILE* f = ObjectManager_open_file("r+", current_object->id);

    fseek(f, 0, SEEK_SET);

    char line[70];

    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);
    fseek(f, strlen("Properties: 000000"), SEEK_CUR);

    if(current_object->properties <= 0xF)
    {
        fputc('0', f);
    }
    itoa(current_object->properties, line, 16);
    fputs(line, f);
    fputc('\n', f);

    fclose(f);

    ObjectManager_print_current_object();

    return ESP_OK;
}

esp_err_t ObjectManager_change_alarm_data_in_file(alarm_mode_args_t alarm)
{
    FILE* f = ObjectManager_open_file("r+", current_object->id);

    fseek(f, 0, SEEK_SET);
    fpos_t alarmPosition;
    bool found = seekfor(f, "ALARM PROPERTIES\n", &alarmPosition);
    fsetpos(f, &alarmPosition);

    if(!found) fprintf(f, "\n");
    fprintf(f, "ALARM PROPERTIES\n");
    fprintf(f, "Mode: %02x\n", alarm.mode);
    fprintf(f, "Enable: %02x\n", alarm.enable);
    fprintf(f, "Description length: %01x\n", alarm.desc_len);
    fprintf(f, "Description: %s\n", alarm.desc);
    fprintf(f, "Hour: %02x\n", alarm.hour);
    fprintf(f, "Minute: %02x\n", alarm.minute);
    
    switch(alarm.mode) 
    {
        case ALARM_SINGLE_MODE:
        {
            fprintf(f, "Day: %02x\n", alarm.args.single_alarm_args.day);
            fprintf(f, "Month: %02x\n", alarm.args.single_alarm_args.month);
            fprintf(f, "Year: %02x\n", alarm.args.single_alarm_args.year);
            break;
        }
            
        case ALARM_WEEKLY_MODE:
        {
            fprintf(f, "Days: %02x\n", alarm.args.days);
            break;
        }
            
        case ALARM_MONTHLY_MODE:
        {
            fprintf(f, "Day: %02x\n", alarm.args.day);
            break;
        }
            
        case ALARM_YEARLY_MODE:
        {
            fprintf(f, "Day: %02x\n", alarm.args.yearly_alarm_args.day);
            fprintf(f, "Month: %02x\n", alarm.args.yearly_alarm_args.month);
            break;
        }
    }

    fprintf(f, "Volume: %02x\n", alarm.volume);

    uint32_t truncate_offset = ftell(f);
    fseek(f, 0, SEEK_SET);
    fclose(f);
    ObjectManager_truncate_rest(current_object->id, truncate_offset);

    ObjectManager_print_current_object();
    ObjectManager_print_file();

    return ESP_OK;
}

static char* id_to_string(char* bfr, uint64_t id)
{
    char number[20];

    if(strcmp(itoa(id>>32, number, 16), "0"))
    {
        strcat(bfr, itoa(id>>32, number, 16));
    }

    strcpy(bfr, itoa((uint32_t)id, number, 16));

    return bfr;
}

static void ObjectManager_print_file()
{
    // if(current_object == NULL)
    // {
    //     return;
    // }

    // FILE* f = ObjectManager_open_file("r", current_object->id);

    // char line[70];

    // while(fgets(line, sizeof(line), f))
    // {
    //     printf("%s", line);
    // }

    // fclose(f);
}

void ObjectManager_printf_alarm_info()
{
    // if(current_object == NULL) return;

    // FILE* f = ObjectManager_open_file("r", current_object->id);
    // fpos_t pos;
    // bool found = seekfor(f, "ALARM PROPERTIES\n", &pos);
    // fclose(f);

    // if(found)
    // {
    //     alarm_mode_args_t alarm = get_alarm_values();
    //     printf("State: %s\n", alarm.enable?"enabled":"disabled");

    //     switch(alarm.mode)
    //     {
    //         case ALARM_SINGLE_MODE:
    //             printf("Mode: single\n");
    //             break;

    //         case ALARM_WEEKLY_MODE:
    //             printf("Mode: weekly\n");
    //             break;

    //         case ALARM_MONTHLY_MODE:
    //             printf("Mode: monthly\n");
    //             break;

    //         case ALARM_YEARLY_MODE:
    //             printf("Mode: yearly\n");
    //             break;
    //     }

    //     printf("Description length: %u\n", alarm.desc_len);
    //     printf("Description: %s\n", alarm.desc);
    //     printf("Hour: %u\n", alarm.hour);
    //     printf("Minute: %u\n", alarm.minute);

    //     switch(alarm.mode)
    //     {
    //         case ALARM_SINGLE_MODE:
    //             printf("Day: %u\n", alarm.args.single_alarm_args.day);
    //             printf("Month: %u\n", alarm.args.single_alarm_args.month);
    //             printf("Year: %u\n", alarm.args.single_alarm_args.year);
    //             break;
    //         case ALARM_WEEKLY_MODE:
    //             printf("Days: %u\n", alarm.args.days);
    //             break;
    //         case ALARM_MONTHLY_MODE:
    //             printf("Day: %u\n", alarm.args.day);
    //             break;
    //         case ALARM_YEARLY_MODE:
    //             printf("Day: %u\n", alarm.args.yearly_alarm_args.day);
    //             printf("Month: %u\n", alarm.args.yearly_alarm_args.month);
    //             break;
    //     }
        
    //     printf("Volume: %d\n", alarm.volume);
    // }
}

static void ObjectManager_print_current_object()
{
    // if(current_object)
    // {
    //     printf("\nSize: 0x%x\n", current_object->size);
    //     printf("Allocated size: 0x%x\n", current_object->alloc_size);
    //     printf("Name: %s\n", current_object->name);
    //     printf("Name length: %u\n", current_object->name_len);
    //     printf("UUID type: %u\n", current_object->type.len);
    //     printf("UUID: ");

    //     for(int i=15; i>=0; i--)
    //     {
    //         printf("%02x", current_object->type.uuid.uuid128[i]);
    //     }
    //     printf("\n");
    //     printf("ID: %llx\n", current_object->id);
    //     printf("Properties: 0x%" PRIx32 "\n\n", current_object->properties);

    //     if(ObjectManager_check_type(current_object->type.uuid.uuid128) == ALARM_TYPE && current_object->set_custom_object)
    //     {
    //         ObjectManager_printf_alarm_info();
    //     }
    // }
}

static void ObjectManager_set_current_object_from_file(uint64_t id)
{
    if(current_object == NULL)
    {
        current_object = (object_t*)malloc(sizeof(object_t));
    }

    FILE* f = ObjectManager_open_file("r", id);

    char line[50];
    char *ptr;

    fgets(line, sizeof(line), f);
    uint32_t size = strtol(&line[6], &ptr, 16);
    current_object->size = size;

    fgets(line, sizeof(line), f);
    uint32_t alloc_size = strtol(&line[16], &ptr, 16);
    current_object->alloc_size = alloc_size;

    fgets(line, sizeof(line), f);

    char name_len_str[20];
    fgets(name_len_str, sizeof(name_len_str), f);
    uint8_t name_len = atoi(&name_len_str[13]);
    current_object->name_len = name_len;

    strncpy(current_object->name, &line[6], name_len);
    current_object->name[name_len] = '\0';

    fgets(line, sizeof(line), f);
    uint8_t uuid_type = atoi(&line[11]);
    current_object->type.len = uuid_type;

    fgets(line, sizeof(line), f);
    char uuid_byte_str[3];
    uint16_t uuid_byte;
    for(int i=15; i>=0; i--)
    {
        strncpy(uuid_byte_str, &line[6+(15-i)*2], 2);
        uuid_byte_str[2] = '\0';
        uuid_byte = strtol(uuid_byte_str, &ptr, 16);
        current_object->type.uuid.uuid128[i] = uuid_byte;
    }

    fgets(line, sizeof(line), f);
    current_object->properties = strtol(&line[12], &ptr, 16);

    current_object->id = id;

    int ret_type = ObjectManager_check_type(current_object->type.uuid.uuid128);

    switch(ret_type)
    {
        case ALARM_TYPE:
        {
            alarm_mode_args_t * alarm_p = get_alarm_pointer();
            fpos_t pos;
            bool found = seekfor(f, "ALARM PROPERTIES\n", &pos);
            if(!found)
            {
                current_object->set_custom_object = false;
                break;
            }

            current_object->set_custom_object = true;

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
            
            break;
        }

        case RINGTONE_TYPE:
            ESP_LOGI(OBJECT_TAG, "Requested object type: Ringtone file");
            break;
    }

    fclose(f);
}

bool seekfor(FILE *stream, const char* str, fpos_t *pos)
{
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

FILE* ObjectManager_open_file(const char* option,  uint64_t id)
{
    char file[20];
    strcpy(file, MOUNT_POINT);
    strcat(file, "/");
    itoa(id, &file[8], 16);
    ESP_LOGI(OBJECT_TAG, "Opened file path: %s", file);

    return fopen(file, option);
}

void ObjectManager_truncate_rest(uint64_t id, uint32_t offset)
{
    char file[20];
    strcpy(file, MOUNT_POINT);
    strcat(file, "/");
    itoa(id, &file[8], 16);
    ESP_LOGI(OBJECT_TAG, "Opened file path: %s", file);
    truncate(file, offset);
}

int ObjectManager_check_type(uint8_t *uuid)
{
    if(memcmp(uuid, alarm_type_uuid, ESP_UUID_LEN_128) == 0) return ALARM_TYPE;
    else if(memcmp(uuid, ringtone_type_uuid, ESP_UUID_LEN_128) == 0) return RINGTONE_TYPE;
    else return -1;
}

void print_all_files()
{
    ESP_LOGI(OBJECT_TAG, "Printing All Files");

    DIR *directory = opendir(MOUNT_POINT);
    if (directory == NULL)
    {
        ESP_LOGE(OBJECT_TAG, "Open mount patn fail.");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(directory)) != NULL)
    {
        ESP_LOGI(OBJECT_TAG, "Found file: %s", entry->d_name);
    }
}
 