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
#include "esp_spiffs.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "FreeRTOSConfig.h"

#include "time.h"
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#define OBJECT_TAG "FILESYSTEM"
#define MAX_FILES_NUMBER 5

#define FILE_LIST_NAME "/spiffs/file_id_list.txt"

static object_t *current_object = NULL;

static esp_vfs_spiffs_conf_t conf = {
    .base_path = "/spiffs",
    .partition_label = NULL,
    .max_files = MAX_FILES_NUMBER,
    .format_if_mount_failed = true
};

static uint8_t alarm_type_uuid[ESP_UUID_LEN_128] = {0x02, 0x00, 0x12, 0xAC, 0x42, 0x02, 0x61, 0xA2, 0xED, 0x11, 0xBA, 0x29, 0xB8, 0x13, 0x08, 0xCC};
static uint8_t ringtone_type_uuid[ESP_UUID_LEN_128] = {0x03, 0x00, 0x12, 0xAC, 0x42, 0x02, 0x61, 0xA2, 0xED, 0x11, 0xBA, 0x29, 0xB8, 0x13, 0x08, 0xCC};
static char* id_to_string(char* bfr, uint64_t id);
static void ObjectManager_print_file();
static void ObjectManager_print_current_object();
static void ObjectManager_set_current_object_from_file(uint64_t id);

static esp_err_t ObjectManager_init_directories()
{
    DIR *directory = opendir(MOUNT_POINT);
    if (directory == NULL)
    {
        ESP_LOGE(OBJECT_TAG, "SD Card directory not found");
        return ESP_ERR_NOT_FOUND;
    }

    ESP_LOGI(OBJECT_TAG, "Listing files on SD CARD");
    struct dirent *entry;
    while ((entry = readdir(directory)) != NULL)
    {
        ESP_LOGI(OBJECT_TAG, "  [%d] %s", entry->d_ino, entry->d_name);
    }

    if (directory)
    {
        closedir(directory);
    }

    directory = opendir(ALARMS_PATH);
    if (directory == NULL)
    {
        ESP_LOGW(OBJECT_TAG, "No alarms directory on SC CARD. Creating a new one...");
        int ret = mkdir(ALARMS_PATH, ACCESSPERMS);
        if (ret)
        {
            ESP_LOGE(OBJECT_TAG, "Creating alarms folder failed.");
            return ESP_FAIL;
        }
    }
    else
    {
        closedir(directory);
    }
    
    directory = opendir(RIGNTONES_PATH);
    if (directory == NULL)
    {
        ESP_LOGW(OBJECT_TAG, "No ringtones directory on SC CARD. Creating a new one...");
        int ret = mkdir(RIGNTONES_PATH, ACCESSPERMS);
        if (ret)
        {
            ESP_LOGE(OBJECT_TAG, "Creating ringtones folder failed. err=%d", ret);
            return ESP_FAIL;
        }
    }
    else
    {
        closedir(directory);
    }

    return ESP_OK;
}

static esp_err_t ObjectManager_init_list()
{
    DIR *alarms_directory = opendir(ALARMS_PATH);
    if (alarms_directory == NULL)
    {
        ESP_LOGE(OBJECT_TAG, "Open alarms direction fail.");
        return ESP_ERR_NOT_FOUND;
    }

    DIR *ringtones_directory = opendir(RIGNTONES_PATH);
    if (ringtones_directory == NULL)
    {
        ESP_LOGE(OBJECT_TAG, "Open ringtones direction fail.");
        return ESP_ERR_NOT_FOUND;
    }

    struct dirent *entry;
    while ((entry = readdir(alarms_directory)) != NULL)
    {
        if ( strstr(entry->d_name, ALARM_FILE_TYPE) == NULL )
        {
            ESP_LOGW(OBJECT_TAG, "File %s is not a alarm type: %s. Deleting file.", entry->d_name, ALARM_FILE_TYPE);
            int result = remove(entry->d_name);
            if (result)
            {
                ESP_LOGW(OBJECT_TAG, "Removeing file %s failed.", entry->d_name);
            }
        }

        int result = access(entry->d_name, F_OK);
        ESP_LOGI(OBJECT_TAG, "Check if %s is a file.", entry->d_name);
        if (result)
        {
            continue;
        }
        ESP_LOGI(OBJECT_TAG, "Adding file %s to list.", entry->d_name);

        FILE *alarm_file = fopen(entry->d_name, "r");
        if (alarm_file == NULL)
        {
            ESP_LOGE(OBJECT_TAG, "Opening %s file failed.", entry->d_name);
            continue;
        }

        struct stat file_stats;
        result = fstat(fileno(alarm_file), &file_stats);
        if (result)
        {
            ESP_LOGW(OBJECT_TAG, "Getting file %s stats failed.", entry->d_name);
        }
        else
        {   
            ESP_LOGI(OBJECT_TAG, "File ID = %d.", file_stats.st_dev);
            ObjectManager_list_add_by_id(file_stats.st_dev);
        }

        result = fclose(alarm_file);
        if (result)
        {
            ESP_LOGE(OBJECT_TAG, "Closing %s file failed.", entry->d_name);
        }
    }

    while ((entry = readdir(ringtones_directory)) != NULL)
    {
        if ( strstr(entry->d_name, RINGTONE_FILE_TYPE) == NULL )
        {
            ESP_LOGW(OBJECT_TAG, "File %s is not a alarm type: %s. Deleting file.", entry->d_name, RINGTONE_FILE_TYPE);
            int result = remove(entry->d_name);
            if (result)
            {
                ESP_LOGW(OBJECT_TAG, "Removeing file %s failed.", entry->d_name);
            }
        }

        int result = access(entry->d_name, F_OK);
        ESP_LOGI(OBJECT_TAG, "Check if %s is a file.", entry->d_name);
        if (result)
        {
            continue;
        }
        ESP_LOGI(OBJECT_TAG, "Adding file %s to list.", entry->d_name);

        FILE *ringtone_file = fopen(entry->d_name, "r");
        if (ringtone_file == NULL)
        {
            ESP_LOGE(OBJECT_TAG, "Opening %s file failed.", entry->d_name);
            continue;
        }

        //TODO add to list
        struct stat file_stats;
        result = fstat(fileno(ringtone_file), &file_stats);
        if (result)
        {
            ESP_LOGW(OBJECT_TAG, "Getting file %s stats failed.", entry->d_name);
        }
        else
        {   
            ESP_LOGI(OBJECT_TAG, "File ID = %d.", file_stats.st_dev);
            ObjectManager_list_add_by_id(file_stats.st_dev);
        }

        result = fclose(ringtone_file);
        if (result)
        {
            ESP_LOGE(OBJECT_TAG, "Closing %s file failed.", entry->d_name);
        }
    }

    FilterOrder_make_list();

    return ESP_OK;
}

esp_err_t ObjectManager_init(void)
{   
    
    esp_err_t ret = ObjectManager_init_directories();
    if (ret)
    {
        ESP_LOGE(OBJECT_TAG, "Object Manager directories initialization fail. err=%d", ret);
        return ret;
    }

    ret = ObjectManager_init_list();
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
    size_t total = 0, used = 0;
    esp_spiffs_info(conf.partition_label, &total, &used);

    ESP_LOGI(OBJECT_TAG, "Requested object size: %" PRIu32, size);

    if(size > total - used)
    {
        ESP_LOGE(OBJECT_TAG, "Insufficient Resources - available space: %u", total - used);
        *result = OACP_RES_INSUF_RSR;
        return ESP_OK;
    }

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

    ESP_LOGI(OBJECT_TAG, "Creating object");
    object_id_list_t* object = ObjectManager_list_add();
    ESP_LOGI(OBJECT_TAG, "Object created, ID: %" PRIx64, object->id);

    if(current_object == NULL)
    {
        current_object = (object_t*)malloc(sizeof(object_t));
    }

    current_object->size = 0;
    current_object->alloc_size = ((size/FLASH_PAGE) + 1)*FLASH_PAGE;
    current_object->name[0] = '\0';
    current_object->name_len = 0;
    current_object->type.len = ESP_UUID_LEN_128;
    memcpy(current_object->type.uuid.uuid128, type.uuid.uuid128, ESP_UUID_LEN_128);
    current_object->id = object->id;
    current_object->properties = PROPERTY_ALL;

    ESP_LOGI(OBJECT_TAG, "Creating file on spiffs");

    FILE* f = ObjectManager_open_file("w+", object->id);

    fprintf(f, "Size: 00000000\n");

    char alloc_size_string[9];
    itoa(current_object->alloc_size, alloc_size_string, 16);
    fprintf(f, "Allocated size: ");
    for(int i=0; i<8-strlen(alloc_size_string); i++)
    {
        fputc('0', f);
    }
    fprintf(f, "%x\n", current_object->alloc_size);

    fprintf(f, "Name: ");
    for(uint8_t character = 0; character < NAME_LEN_MAX-1; character++)
    {
        fputc('*', f);
    }
    fputc('\n', f);

    fprintf(f, "Name length: 00\n");
    fprintf(f, "UUID type: %u\n", 128);
    fprintf(f, "UUID: ");

    for(int i=15; i>=0; i--)
    {
        fprintf(f, "%02x", type.uuid.uuid128[i]);
    }

    fprintf(f, "\n");
    fprintf(f, "Properties: 0000000%02x\n", PROPERTY_ALL_WITHOUT_MARK);
    fclose(f);
    ESP_LOGI(OBJECT_TAG, "File created: %" PRIx64 ".txt", object->id);

    char id_string[20];
    ESP_LOGI(OBJECT_TAG, "ID converted to string: %s\n", id_to_string(id_string, object->id));

    f = fopen(FILE_LIST_NAME, "a+");

    fprintf(f, "%s\n", id_to_string(id_string, object->id));
    fclose(f);

    switch(ret_type)
    {
        case ALARM_TYPE:
            current_object->params.alarm.set = false;
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
        ESP_LOGE(OBJECT_TAG, "Procedure not permittet");
        *result = OACP_RES_PROCEDURE_NOT_PERMIT;
        return ESP_OK;
    }

    char file[20];
    strcpy(file, "/spiffs/");
    itoa(current_object->id, &file[8], 16);
    strcat(file, ".txt");
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
    FILE* file_dest = fopen("/spiffs/temp.txt", "w");
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
    rename("/spiffs/temp.txt", FILE_LIST_NAME);

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

esp_err_t ObjectManager_change_alarm_data_in_file()
{
    FILE* f = ObjectManager_open_file("r+", current_object->id);

    fseek(f, 0, SEEK_SET);
    fpos_t alarmPosition;
    bool found = seekfor(f, "ALARM PROPERTIES\n", &alarmPosition);
    fsetpos(f, &alarmPosition);

    if(!found) fprintf(f, "\n");
    fprintf(f, "ALARM PROPERTIES\n");
    fprintf(f, "Enable: %02x\n", current_object->params.alarm.enable);
    fprintf(f, "Mode: %02x\n", current_object->params.alarm.mode);
    fprintf(f, "Timestamp: %08" PRIx32 "\n", current_object->params.alarm.timestamp);
    fprintf(f, "Days: %02x\n", current_object->params.alarm.days);
    fprintf(f, "Description length: %01x\n", current_object->params.alarm.description_len);
    fprintf(f, "Description: ");
    for(int iter_descr=0; iter_descr<current_object->params.alarm.description_len; iter_descr++)
    {
        fputc(current_object->params.alarm.description[iter_descr], f);
    }

    for(int iter_fill=0; iter_fill<20-current_object->params.alarm.description_len; iter_fill++)
    {
        fputc('*', f);
    }
    fputc('\n', f);

    fprintf(f, "Volume: %02x\n", current_object->params.alarm.volume);
    fprintf(f, "Rising sound enable: %02x\n", current_object->params.alarm.risingSoundEnable);
    fprintf(f, "Nap enable: %02x\n", current_object->params.alarm.napEnable);
    fprintf(f, "Nap duration: %02x\n", current_object->params.alarm.napDuration);
    fprintf(f, "Nap amount: %02x\n", current_object->params.alarm.napAmount);

    fclose(f);

    ObjectManager_print_current_object();

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
    if(current_object == NULL)
    {
        return;
    }

    FILE* f = ObjectManager_open_file("r", current_object->id);

    char line[70];

    while(fgets(line, sizeof(line), f))
    {
        printf("%s", line);
    }

    fclose(f);
}

void ObjectManager_printf_alarm_info()
{
    if(current_object == NULL) return;

    FILE* f = ObjectManager_open_file("r", current_object->id);
    fpos_t pos;
    bool found = seekfor(f, "ALARM PROPERTIES\n", &pos);
    fclose(f);

    if(found)
    {
        printf("State: %s\n", current_object->params.alarm.enable?"enabled":"disabled");
        char mode_buff[10];
        switch(current_object->params.alarm.mode)
        {
            case 0:
                strcpy(mode_buff, "single");
                break;
            case 1:
                strcpy(mode_buff, "weekly");
                break;
            case 2:
                strcpy(mode_buff, "monthly");
                break;
            case 3:
                strcpy(mode_buff, "yearly");
                break;
        }
        printf("Mode: %s\n", mode_buff);

        time_t rawtime = current_object->params.alarm.timestamp;
        struct tm  ts;
        ts = *localtime(&rawtime);
        char buf[30];
        strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", &ts);
        printf("DateTime: %s\n", buf);

        if(current_object->params.alarm.mode == 1) printf("Days: %02x\n", current_object->params.alarm.days);

        printf("Description length: %u\n", current_object->params.alarm.description_len);
        printf("Description: %s\n", current_object->params.alarm.description);
        printf("Volume: %d%%\n", current_object->params.alarm.volume*100/255);
        printf("Rising sound: %s\n", current_object->params.alarm.risingSoundEnable?"enabled":"disabled");
        printf("Nap: %s\n", current_object->params.alarm.napEnable?"enabled":"disabled");
        printf("Nap duration: %x minute(s)\n", current_object->params.alarm.napDuration);
        printf("Nap amount: %s\n", current_object->params.alarm.napAmount?strcat(itoa(current_object->params.alarm.napAmount, buf, 10), " time(s)"):"Always");
    }
}

static void ObjectManager_print_current_object()
{
    if(current_object)
    {
        printf("\nSize: 0x%x\n", current_object->size);
        printf("Allocated size: 0x%x\n", current_object->alloc_size);
        printf("Name: %s\n", current_object->name);
        printf("Name length: %u\n", current_object->name_len);
        printf("UUID type: %u\n", current_object->type.len);
        printf("UUID: ");

        for(int i=15; i>=0; i--)
        {
            printf("%02x", current_object->type.uuid.uuid128[i]);
        }
        printf("\n");
        printf("ID: %llx\n", current_object->id);
        printf("Properties: 0x%" PRIx32 "\n\n", current_object->properties);
    }

    if(ObjectManager_check_type(current_object->type.uuid.uuid128) == ALARM_TYPE && current_object->params.alarm.set)
    {
        ObjectManager_printf_alarm_info();
    }
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
            fpos_t pos;
            bool found = seekfor(f, "ALARM PROPERTIES\n", &pos);
            if(!found)
            {
                current_object->params.alarm.set = false;
                break;
            }

            current_object->params.alarm.set = true;
            char line[70];
            fgets(line, sizeof(line), f);

            char *ptr;
            current_object->params.alarm.enable = strtol(&line[strlen("Enable: ") ], &ptr, 16);

            fgets(line, sizeof(line), f);
            current_object->params.alarm.mode = strtol(&line[strlen("Mode: ") ], &ptr, 16);

            fgets(line, sizeof(line), f);
            current_object->params.alarm.timestamp = strtol(&line[strlen("Timestamp: ") ], &ptr, 16);

            fgets(line, sizeof(line), f);
            current_object->params.alarm.days = strtol(&line[strlen("Days: ") ], &ptr, 16);

            fgets(line, sizeof(line), f);
            current_object->params.alarm.description_len = strtol(&line[strlen("Description length: ") ], &ptr, 16);

            fgets(line, sizeof(line), f);
            strncpy((char*)current_object->params.alarm.description, (char*)&line[strlen("Description: ")], current_object->params.alarm.description_len);
            current_object->params.alarm.description[current_object->params.alarm.description_len] = '\0';
            
            fgets(line, sizeof(line), f);
            current_object->params.alarm.volume = strtol(&line[strlen("Volume: ") ], &ptr, 16);

            fgets(line, sizeof(line), f);
            current_object->params.alarm.risingSoundEnable = strtol(&line[strlen("Rising sound enable: ") ], &ptr, 16);

            fgets(line, sizeof(line), f);
            current_object->params.alarm.napEnable = strtol(&line[strlen("Nap enable: ") ], &ptr, 16);

            fgets(line, sizeof(line), f);
            current_object->params.alarm.napDuration = strtol(&line[strlen("Nap duration: ") ], &ptr, 16);

            fgets(line, sizeof(line), f);
            current_object->params.alarm.napAmount = strtol(&line[strlen("Nap amount: ") ], &ptr, 16);
            
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
    strcpy(file, "/spiffs/");
    itoa(id, &file[8], 16);
    strcat(file, ".txt");
    ESP_LOGI(OBJECT_TAG, "Opened file path: %s", file);

    return fopen(file, option);
}

int ObjectManager_check_type(uint8_t *uuid)
{
    if(memcmp(uuid, alarm_type_uuid, ESP_UUID_LEN_128) == 0) return ALARM_TYPE;
    else if(memcmp(uuid, ringtone_type_uuid, ESP_UUID_LEN_128) == 0) return RINGTONE_TYPE;
    else return -1;
}