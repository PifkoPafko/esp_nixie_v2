#ifndef __OBJECT_MANAGER_H__
#define __OBJECT_MANAGER_H__

#include "esp_err.h"
#include "esp_gatts_api.h"
#include "ObjectTransfer_defs.h"

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

typedef struct alarm{
    bool set;
    bool enable;
    uint8_t mode;
    uint32_t timestamp;
    uint8_t days;
    uint8_t description_len;
    char description[21];
    uint8_t volume;
    bool risingSoundEnable;
    bool napEnable;
    uint8_t napDuration;
    uint8_t napAmount;
} alarm_t;

typedef struct ringtone{
    uint8_t XD;
} ringtone_t;

typedef union params{
    alarm_t alarm;
    ringtone_t ringtone;
}params_t;

typedef struct object{
    size_t size;
    size_t alloc_size;
    char name[NAME_LEN_MAX+1];
    uint8_t name_len;
    esp_bt_uuid_t type;
    uint64_t id;
    uint32_t properties;
    params_t params;
} object_t;

#define ALARM_TYPE 0
#define RINGTONE_TYPE 1

void ObjectManager_init(void);
object_t* ObjectManager_get_object(void);
void ObjectManager_null_current_object(void);
esp_err_t ObjectManager_create_object(uint32_t size, esp_bt_uuid_t type, oacp_op_code_result_t *result);
esp_err_t ObjectManager_delete_object(oacp_op_code_result_t *result);

esp_err_t ObjectManager_first_object(olcp_op_code_result_t *result);
esp_err_t ObjectManager_last_object(olcp_op_code_result_t *result);
esp_err_t ObjectManager_previous_object(olcp_op_code_result_t *result);
esp_err_t ObjectManager_next_object(olcp_op_code_result_t *result);
esp_err_t ObjectManager_goto_object(uint64_t id, olcp_op_code_result_t *result);
esp_err_t ObjectManager_order_object(uint8_t type, olcp_op_code_result_t *result);
esp_err_t ObjectManager_request_number(uint32_t *number, olcp_op_code_result_t *result);
esp_err_t ObjectManager_clear_marking(olcp_op_code_result_t *result);

esp_err_t ObjectManager_change_name_in_file();
esp_err_t ObjectManager_change_properties_in_file();
esp_err_t ObjectManager_change_alarm_data_in_file();
void ObjectManager_printf_alarm_info();
bool seekfor(FILE *stream, const char* str, fpos_t *pos);
FILE* ObjectManager_open_file(const char* option,  uint64_t id);
int ObjectManager_check_type(uint8_t *uuid);

#endif