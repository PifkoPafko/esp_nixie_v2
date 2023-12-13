#include "ObjectTransfer_metadata_write.h"
#include "ObjectManager.h"
#include "ObjectTransfer_attr_ids.h"
#include "ObjectTransfer_defs.h"
#include "ObjectManagerIdList.h"
#include "FilterOrder.h"
#include "esp_err.h"
#include "esp_gatts_api.h"
#include "esp_log.h"

#include "stdlib.h"

#define TAG "WRITE_EVENT"

static esp_err_t ObjectTransfer_write_name(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table);
static esp_err_t ObjectTransfer_write_properties(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table);
static esp_err_t ObjectTransfer_write_list_filter(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table);

static esp_err_t ObjectTransfer_write_OACP(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table);
static esp_err_t ObjectTransfer_write_OACP_CCC(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table);
static esp_err_t ObjectTransfer_write_OACP_Create(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table);
static esp_err_t ObjectTransfer_write_OACP_Delete(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table);
static esp_err_t ObjectTransfer_write_OACP_OP_NS(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table);

static esp_err_t ObjectTransfer_write_OLCP(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table);
static esp_err_t ObjectTransfer_write_OLCP_CCC(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table);
static esp_err_t ObjectTransfer_write_OLCP_First(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table);
static esp_err_t ObjectTransfer_write_OLCP_Last(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table);
static esp_err_t ObjectTransfer_write_OLCP_Next(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table);
static esp_err_t ObjectTransfer_write_OLCP_Previous(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table);
static esp_err_t ObjectTransfer_write_OLCP_Last(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table);
static esp_err_t ObjectTransfer_write_OLCP_Goto(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table);
static esp_err_t ObjectTransfer_write_OLCP_Order(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table);
static esp_err_t ObjectTransfer_write_OLCP_Request_Num(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table);
static esp_err_t ObjectTransfer_write_OLCP_Clear_Marking(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table);
static esp_err_t ObjectTransfer_write_OLCP_OP_NS(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table);

static esp_err_t ObjectTransfer_write_Alarm_Action(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table);
static esp_err_t ObjectTransfer_write_Ringtone_Action(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table);
static esp_err_t ObjectTransfer_write_wifi_action(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table);


static esp_err_t ObjectTransfer_write_OLCP_CCC(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table);

esp_err_t ObjectTranfer_metadata_write_event(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table)
{
    if(param->write.handle == handle_table[OPT_IDX_CHAR_OBJECT_NAME_VAL]) ObjectTransfer_write_name(gatts_if, param, handle_table);
    else if(param->write.handle == handle_table[OPT_IDX_CHAR_OBJECT_PROPERTIES_VAL]) ObjectTransfer_write_properties(gatts_if, param, handle_table);
    else if(param->write.handle == handle_table[OPT_IDX_CHAR_OBJECT_OACP_VAL]) ObjectTransfer_write_OACP(gatts_if, param, handle_table);
    else if(param->write.handle == handle_table[OPT_IDX_CHAR_OBJECT_OACP_IND_CFG]) ObjectTransfer_write_OACP_CCC(gatts_if, param, handle_table);
    else if(param->write.handle == handle_table[OPT_IDX_CHAR_OBJECT_OLCP_VAL]) ObjectTransfer_write_OLCP(gatts_if, param, handle_table);
    else if(param->write.handle == handle_table[OPT_IDX_CHAR_OBJECT_OLCP_IND_CFG]) ObjectTransfer_write_OLCP_CCC(gatts_if, param, handle_table);
    else if(param->write.handle == handle_table[OPT_IDX_CHAR_OBJECT_LIST_FILTER_VAL]) ObjectTransfer_write_list_filter(gatts_if, param, handle_table);
    else if(param->write.handle == handle_table[OPT_IDX_CHAR_OBJECT_ALARM_ACTION_VAL]) ObjectTransfer_write_Alarm_Action(gatts_if, param, handle_table);
    else if(param->write.handle == handle_table[OPT_IDX_CHAR_OBJECT_RINGTONE_ACTION_VAL]) ObjectTransfer_write_Ringtone_Action(gatts_if, param, handle_table);
    else if(param->write.handle == handle_table[OPT_IDX_CHAR_OBJECT_WIFI_ACTION_VAL]) ObjectTransfer_write_wifi_action(gatts_if, param, handle_table);

    return ESP_OK;
}

static esp_err_t ObjectTransfer_write_name(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table)
{
    ESP_LOGI(TAG, "Object Name WRITE EVENT");

    esp_gatt_rsp_t rsp;
    rsp.handle = handle_table[OPT_IDX_CHAR_OBJECT_NAME_VAL];
    esp_err_t ret;

    object_t *object;
    object = ObjectManager_get_object();
    if(object == NULL)
    {
        ESP_LOGE(TAG, "Object not selected");

        if(param->write.need_rsp)
        {
            rsp.handle = handle_table[OPT_IDX_CHAR_OBJECT_NAME_VAL];
            ret = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ERROR_OBJECT_NOT_SELECTED, &rsp);
            return ESP_OK;
        }
    }

    if(param->write.len == 0)
    {
        ESP_LOGE(TAG, "The new name cannot be empty");

        if(param->write.need_rsp)
        {
            esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, INVALID_ATTR_VAL_LENGTH, &rsp);
            return ESP_OK;
        }
    }

    if(param->write.len > NAME_LEN_MAX-1 || param->write.len == 0)
    {
        ESP_LOGE(TAG, "The new name is too long. Max length is %d", NAME_LEN_MAX);

        if(param->write.need_rsp)
        {
            esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, INVALID_ATTR_VAL_LENGTH, &rsp);
            return ESP_OK;
        }
    }

    strncpy(object->name, (const char*)param->write.value, NAME_LEN_MAX);
    object->name_len = strlen(object->name);
    ESP_LOGI("WRITE", "Object name changed to '%s'", object->name);

    ret = esp_ble_gatts_set_attr_value(handle_table[OPT_IDX_CHAR_OBJECT_NAME_VAL], strlen(object->name)+1, (uint8_t*)object->name);

    if(param->write.need_rsp)
    {
        ret = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, STATUS_OK, &rsp);
        if(ret) return ret;
    }
    ObjectManager_change_name_in_file();

    return ESP_OK;
}

static esp_err_t ObjectTransfer_write_properties(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table)
{
    ESP_LOGD(TAG, "Object Properties WRITE EVENT");

    esp_gatt_rsp_t rsp;
    rsp.handle = handle_table[OPT_IDX_CHAR_OBJECT_PROPERTIES_VAL];
    esp_err_t ret;

    uint32_t new_properties;

    if(param->write.len != 4 && param->write.need_rsp)
    {
        ret = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, INVALID_ATTR_VAL_LENGTH, &rsp);
        return ESP_OK;
    }

    memcpy(&new_properties, param->write.value, 4);
    if(new_properties > 0xFF)
    {
        ret = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, WRITE_REQUEST_REJECTED, &rsp);
        return ESP_OK;
    }

    object_t *object;
    object = ObjectManager_get_object();
    if(object == NULL && param->write.need_rsp)
    {
        ESP_LOGE(TAG, "Object not selected");

        rsp.handle = handle_table[OPT_IDX_CHAR_OBJECT_PROPERTIES_VAL];
        ret = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ERROR_OBJECT_NOT_SELECTED, &rsp);
        return ESP_OK;
    }

    object->properties = new_properties;
    ESP_LOGI("WRITE", "Object properties changed to: 0x%" PRIx32, object->properties);
    ret = esp_ble_gatts_set_attr_value(handle_table[OPT_IDX_CHAR_OBJECT_PROPERTIES_VAL], 4, (uint8_t*)&object->properties);

    if(param->write.need_rsp)
    {
        ret = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, STATUS_OK, &rsp);
        if(ret) return ret;
    }

    ObjectManager_change_properties_in_file();

    return ESP_OK;
}

static esp_err_t ObjectTransfer_write_list_filter(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table)
{
    ESP_LOGD(TAG, "Object Properties WRITE EVENT");

    esp_gatt_rsp_t rsp;
    rsp.handle = handle_table[OPT_IDX_CHAR_OBJECT_LIST_FILTER_VAL];
    esp_err_t ret;


    if(param->write.len == 0 && param->write.need_rsp)
    {
        ret = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, INVALID_ATTR_VAL_LENGTH, &rsp);
        return ESP_OK;
    }

    switch(param->write.value[0])
    {
        case NO_FILTER:
            if(param->write.len != 1)
            {
                ret = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, INVALID_ATTR_VAL_LENGTH, &rsp);
                return ESP_OK;
            }
            break;

        case NAME_STARTS_WITH:
            if(param->write.len <= 1 || param->write.len > NAME_LEN_MAX)
            {
                ret = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, INVALID_ATTR_VAL_LENGTH, &rsp);
                return ESP_OK;
            }
            break;

        case NAME_ENDS_WITH:
            if(param->write.len <= 1 || param->write.len > NAME_LEN_MAX)
            {
                ret = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, INVALID_ATTR_VAL_LENGTH, &rsp);
                return ESP_OK;
            }
            break;

        case NAME_CONTAINS:
            if(param->write.len <= 1 || param->write.len > NAME_LEN_MAX)
            {
                ret = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, INVALID_ATTR_VAL_LENGTH, &rsp);
                return ESP_OK;
            }
            break;

        case NAME_IS_EXACTLY:
            if(param->write.len <= 1 || param->write.len > NAME_LEN_MAX)
            {
                ret = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, INVALID_ATTR_VAL_LENGTH, &rsp);
                return ESP_OK;
            }
            break;

        case OBJECT_TYPE:
            if(param->write.len != 17)
            {
                ret = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, INVALID_ATTR_VAL_LENGTH, &rsp);
                return ESP_OK;
            }
            break;

        case CURRENT_SIZE_BETWEEN:
            if(param->write.len != 9)
            {
                ret = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, INVALID_ATTR_VAL_LENGTH, &rsp);
                return ESP_OK;
            }

            uint32_t left_size, right_size;
            memcpy(&left_size, param->write.value, 4);
            memcpy(&right_size, &param->write.value[4], 4);
            if(right_size<left_size)
            {
                ret = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, WRITE_REQUEST_REJECTED, &rsp);
                return ESP_OK;
            }
            break;

        case ALLOC_SIZE_BETWEEN:
            if(param->write.len != 9)
            {
                ret = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, INVALID_ATTR_VAL_LENGTH, &rsp);
                return ESP_OK;
            }
            break;

        case MARKED_OBJECTS:
            if(param->write.len != 1)
            {
                ret = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, INVALID_ATTR_VAL_LENGTH, &rsp);
                return ESP_OK;
            }
            break;

        default:
            ret = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, WRITE_REQUEST_REJECTED, &rsp);
            return ESP_OK;
            break;
    }

    ListFilter_t *filter = FilterOrder_get_filter();
    filter->type = param->write.value[0];
    memcpy(filter->parameter, &param->write.value[1], param->write.len-1);
    //if(filter->type >= 0x01 && filter->type <= 0x04) filter->parameter[param->write.len-1] = '\0';
    filter->par_length = param->write.len-1;

    FilterOrder_make_list();

    if(param->write.need_rsp)
    {
        ret = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, STATUS_OK, &rsp);
        if(ret) return ret;
    }

    return ESP_OK;
}

static esp_err_t ObjectTransfer_write_OACP(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table)
{
    ESP_LOGD(TAG, "Object OACP WRITE EVENT");

    esp_gatt_rsp_t rsp;
    rsp.handle = handle_table[OPT_IDX_CHAR_OBJECT_OACP_VAL];

    if(param->write.len == 0 && param->write.need_rsp)
    {
        ESP_LOGE(TAG, "INVALID ATTR VAL LENGTH");
        ESP_LOGE(TAG, "LEN: %d", param->write.len);

        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, INVALID_ATTR_VAL_LENGTH, &rsp);
        return ESP_OK;
    }

    switch(param->write.value[0])
    {
        case OACP_OP_CODE_CREATE:
            ObjectTransfer_write_OACP_Create(gatts_if, param, handle_table);
            break;

        case OACP_OP_CODE_DELETE:
            ObjectTransfer_write_OACP_Delete(gatts_if, param, handle_table);
            break;

        case OACP_OP_CODE_CALC_SUM:
            ObjectTransfer_write_OACP_OP_NS(gatts_if, param, handle_table);
            break;

        case OACP_OP_CODE_EXECUTE:
            ObjectTransfer_write_OACP_OP_NS(gatts_if, param, handle_table);
            break;

        case OACP_OP_CODE_READ:
            ObjectTransfer_write_OACP_OP_NS(gatts_if, param, handle_table);
            break;

        case OACP_OP_CODE_WRITE:
            ObjectTransfer_write_OACP_OP_NS(gatts_if, param, handle_table);
            break;

        case OACP_OP_CODE_ABORT:
            ObjectTransfer_write_OACP_OP_NS(gatts_if, param, handle_table);
            break;

        default:
            ObjectTransfer_write_OACP_OP_NS(gatts_if, param, handle_table);
            break;
    }

    return ESP_OK;
}

static esp_err_t ObjectTransfer_write_OACP_Create(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table)
{
    esp_gatt_rsp_t rsp;
    rsp.handle = handle_table[OPT_IDX_CHAR_OBJECT_OACP_VAL];

    uint8_t status = STATUS_OK;
    uint8_t indicate_data[2];
    uint8_t indicate_data_len = 0;
    indicate_data[0] = OACP_OP_CODE_RESPONSE;

    if(param->write.len != DATA_LEN_UUID16 && param->write.len != DATA_LEN_UUID128)
    {
        ESP_LOGE(TAG, "INVALID ATTR VAL LENGTH");
        ESP_LOGE(TAG, "LEN: %d", param->write.len);

        status = INVALID_ATTR_VAL_LENGTH;
    }

    if(param->write.need_rsp)
    {
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, &rsp);
    }
    if(status != STATUS_OK) return ESP_OK;

    uint32_t size;
    memcpy(&size, &param->write.value[1], 4);

    esp_bt_uuid_t type;

    if(param->write.len == DATA_LEN_UUID16)
    {
        type.len = ESP_UUID_LEN_16;
        memcpy(&type.uuid.uuid16, &param->write.value[5], ESP_UUID_LEN_16);
    }
    else if(param->write.len == DATA_LEN_UUID128)
    {
        type.len = ESP_UUID_LEN_128;
        memcpy(type.uuid.uuid128, &param->write.value[5], ESP_UUID_LEN_128);
    }

    oacp_op_code_result_t result;
    ObjectManager_create_object(size, type, &result);
    indicate_data_len = 2;
    indicate_data[1] = result;
    esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, handle_table[OPT_IDX_CHAR_OBJECT_OACP_VAL], indicate_data_len, indicate_data, true);

    return ESP_OK;
}

static esp_err_t ObjectTransfer_write_OACP_Delete(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table)
{
    esp_gatt_rsp_t rsp;
    rsp.handle = handle_table[OPT_IDX_CHAR_OBJECT_OACP_VAL];

    uint8_t status = STATUS_OK;
    uint8_t indicate_data[2];
    uint8_t indicate_data_len = 0;
    indicate_data[0] = OACP_OP_CODE_RESPONSE;

    if(param->write.len != 1)
    {
        ESP_LOGE(TAG, "INVALID ATTR VAL LENGTH");
        ESP_LOGE(TAG, "LEN: %d", param->write.len);

        status = INVALID_ATTR_VAL_LENGTH;
    }

    if(param->write.need_rsp)
    {
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, &rsp);
    }
    if(status != STATUS_OK) return ESP_OK;

    uint32_t size;
    memcpy(&size, &param->write.value[1], 4);


    oacp_op_code_result_t result;
    ObjectManager_delete_object(&result);
    indicate_data_len = 2;
    indicate_data[1] = result;
    esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, handle_table[OPT_IDX_CHAR_OBJECT_OACP_VAL], indicate_data_len, indicate_data, true);

    return ESP_OK;
}

static esp_err_t ObjectTransfer_write_OACP_OP_NS(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table)
{
    esp_gatt_rsp_t rsp;
    rsp.handle = handle_table[OPT_IDX_CHAR_OBJECT_OACP_VAL];

    uint8_t indicate_data[3];
    uint8_t indicate_data_len = 0;
    indicate_data[0] = OACP_OP_CODE_RESPONSE;
    indicate_data[1] = param->write.value[0];

    esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, STATUS_OK, &rsp);
    ESP_LOGE(TAG, "OP CODE NOT SUPPORTED");
    indicate_data_len = 3;
    indicate_data[2] = OACP_RES_OP_CODE_NOT_SUPPORTED;
    esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, handle_table[OPT_IDX_CHAR_OBJECT_OACP_VAL], indicate_data_len, indicate_data, true);

    return ESP_OK;
}

static esp_err_t ObjectTransfer_write_OACP_CCC(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table)
{
    if(param->write.len == 2){
        uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
        if (descr_value == 0x0002){
            ESP_LOGI(TAG, "OACP indicate enable");
            uint8_t indicate_data[15];
            for (int i = 0; i < sizeof(indicate_data); ++i)
            {
                indicate_data[i] = i % 0xff;
            }
        }
        else if (descr_value == 0x0000){
            ESP_LOGI(TAG, "OACP indicate disable");
        }else{
            ESP_LOGE(TAG, "unknown descr value");;
        }
    }
    return ESP_OK;
}

static esp_err_t ObjectTransfer_write_OLCP(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table)
{
    ESP_LOGD(TAG, "Object OLCP WRITE EVENT");

    esp_gatt_rsp_t rsp;
    rsp.handle = handle_table[OPT_IDX_CHAR_OBJECT_OLCP_VAL];

    if(param->write.len == 0 && param->write.need_rsp)
    {
        ESP_LOGE(TAG, "INVALID ATTR VAL LENGTH");
        ESP_LOGE(TAG, "LEN: %d", param->write.len);

        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, INVALID_ATTR_VAL_LENGTH, &rsp);
        return ESP_OK;
    }

    switch(param->write.value[0])
    {
        case OLCP_OP_CODE_FIRST:
            ObjectTransfer_write_OLCP_First(gatts_if, param, handle_table);
            break;

        case OLCP_OP_CODE_LAST:
            ObjectTransfer_write_OLCP_Last(gatts_if, param, handle_table);
            break;

        case OLCP_OP_CODE_PREVIOUS:
            ObjectTransfer_write_OLCP_Previous(gatts_if, param, handle_table);
            break;

        case OLCP_OP_CODE_NEXT:
            ObjectTransfer_write_OLCP_Next(gatts_if, param, handle_table);
            break;

        case OLCP_OP_CODE_GOTO:
            ObjectTransfer_write_OLCP_Goto(gatts_if, param, handle_table);
            break;

        case OLCP_OP_CODE_ORDER:
            ObjectTransfer_write_OLCP_Order(gatts_if, param, handle_table);
            break;

        case OLCP_OP_CODE_REQ_NUM_OF_OBJ:
            ObjectTransfer_write_OLCP_Request_Num(gatts_if, param, handle_table);
            break;

        case OLCP_OP_CODE_CLEAR_MARING:
            ObjectTransfer_write_OLCP_Clear_Marking(gatts_if, param, handle_table);
            break;

        default:
            ObjectTransfer_write_OLCP_OP_NS(gatts_if, param, handle_table);
            break;
    }

    return ESP_OK;
}

static esp_err_t ObjectTransfer_write_OLCP_First(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table)
{
    esp_gatt_rsp_t rsp;
    rsp.handle = handle_table[OPT_IDX_CHAR_OBJECT_OLCP_VAL];

    uint8_t status = STATUS_OK;
    uint8_t indicate_data[2];
    uint8_t indicate_data_len = 0;
    indicate_data[0] = OLCP_OP_CODE_RESPONSE;

    if(param->write.len != 1)
    {
        ESP_LOGE(TAG, "INVALID ATTR VAL LENGTH");
        ESP_LOGE(TAG, "LEN: %d", param->write.len);

        status = INVALID_ATTR_VAL_LENGTH;
    }

    if(param->write.need_rsp)
    {
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, &rsp);
    }
    if(status != STATUS_OK) return ESP_OK;

    olcp_op_code_result_t result;
    ObjectManager_first_object(&result);
    indicate_data_len = 2;
    indicate_data[1] = result;
    esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, handle_table[OPT_IDX_CHAR_OBJECT_OLCP_VAL], indicate_data_len, indicate_data, true);

    return ESP_OK;
}

static esp_err_t ObjectTransfer_write_OLCP_Last(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table)
{
    esp_gatt_rsp_t rsp;
    rsp.handle = handle_table[OPT_IDX_CHAR_OBJECT_OLCP_VAL];

    uint8_t status = STATUS_OK;
    uint8_t indicate_data[2];
    uint8_t indicate_data_len = 0;
    indicate_data[0] = OLCP_OP_CODE_RESPONSE;

    if(param->write.len != 1)
    {
        ESP_LOGE(TAG, "INVALID ATTR VAL LENGTH");
        ESP_LOGE(TAG, "LEN: %d", param->write.len);

        status = INVALID_ATTR_VAL_LENGTH;
    }

    if(param->write.need_rsp)
    {
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, &rsp);
    }
    if(status != STATUS_OK) return ESP_OK;

    olcp_op_code_result_t result;
    ObjectManager_last_object(&result);
    indicate_data_len = 2;
    indicate_data[1] = result;
    esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, handle_table[OPT_IDX_CHAR_OBJECT_OLCP_VAL], indicate_data_len, indicate_data, true);

    return ESP_OK;
}

static esp_err_t ObjectTransfer_write_OLCP_Previous(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table)
{
    esp_gatt_rsp_t rsp;
    rsp.handle = handle_table[OPT_IDX_CHAR_OBJECT_OLCP_VAL];

    uint8_t status = STATUS_OK;
    uint8_t indicate_data[2];
    uint8_t indicate_data_len = 0;
    indicate_data[0] = OLCP_OP_CODE_RESPONSE;

    if(param->write.len != 1)
    {
        ESP_LOGE(TAG, "INVALID ATTR VAL LENGTH");
        ESP_LOGE(TAG, "LEN: %d", param->write.len);

        status = INVALID_ATTR_VAL_LENGTH;
    }

    if(param->write.need_rsp)
    {
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, &rsp);
    }
    if(status != STATUS_OK) return ESP_OK;

    olcp_op_code_result_t result;
    ObjectManager_previous_object(&result);
    indicate_data_len = 2;
    indicate_data[1] = result;
    esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, handle_table[OPT_IDX_CHAR_OBJECT_OLCP_VAL], indicate_data_len, indicate_data, true);

    return ESP_OK;
}

static esp_err_t ObjectTransfer_write_OLCP_Next(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table)
{
    esp_gatt_rsp_t rsp;
    rsp.handle = handle_table[OPT_IDX_CHAR_OBJECT_OLCP_VAL];

    uint8_t status = STATUS_OK;
    uint8_t indicate_data[2];
    uint8_t indicate_data_len = 0;
    indicate_data[0] = OLCP_OP_CODE_RESPONSE;

    if(param->write.len != 1)
    {
        ESP_LOGE(TAG, "INVALID ATTR VAL LENGTH");
        ESP_LOGE(TAG, "LEN: %d", param->write.len);

        status = INVALID_ATTR_VAL_LENGTH;
    }

    if(param->write.need_rsp)
    {
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, &rsp);
    }
    if(status != STATUS_OK) return ESP_OK;

    olcp_op_code_result_t result;
    ObjectManager_next_object(&result);
    indicate_data_len = 2;
    indicate_data[1] = result;
    esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, handle_table[OPT_IDX_CHAR_OBJECT_OLCP_VAL], indicate_data_len, indicate_data, true);

    return ESP_OK;
}

static esp_err_t ObjectTransfer_write_OLCP_Goto(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table)
{
    esp_gatt_rsp_t rsp;
    rsp.handle = handle_table[OPT_IDX_CHAR_OBJECT_OLCP_VAL];

    uint8_t status = STATUS_OK;
    uint8_t indicate_data[2];
    uint8_t indicate_data_len = 0;
    indicate_data[0] = OLCP_OP_CODE_RESPONSE;

    if(param->write.len != 7)
    {
        ESP_LOGE(TAG, "INVALID ATTR VAL LENGTH");
        ESP_LOGE(TAG, "LEN: %d", param->write.len);

        status = INVALID_ATTR_VAL_LENGTH;
    }

    if(param->write.need_rsp)
    {
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, &rsp);
    }
    if(status != STATUS_OK) return ESP_OK;

    olcp_op_code_result_t result;
    uint64_t id = 0;
    // uint8_t *ptr = (uint8_t*)&id;

    // for(int iter=0; iter<6; iter++)
    // {
    //     ptr[iter] = param->write.value[iter+1];
    // }

    memcpy(&id, &param->write.value[1], 6);

    ESP_LOGI(TAG, "Searching for ID: %llx", id);

    ObjectManager_goto_object(id, &result);
    indicate_data_len = 2;
    indicate_data[1] = result;
    esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, handle_table[OPT_IDX_CHAR_OBJECT_OLCP_VAL], indicate_data_len, indicate_data, true);

    return ESP_OK;
}

static esp_err_t ObjectTransfer_write_OLCP_Order(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table)
{
    esp_gatt_rsp_t rsp;
    rsp.handle = handle_table[OPT_IDX_CHAR_OBJECT_OLCP_VAL];

    uint8_t status = STATUS_OK;
    uint8_t indicate_data[2];
    uint8_t indicate_data_len = 0;
    indicate_data[0] = OLCP_OP_CODE_RESPONSE;

    if(param->write.len != 2)
    {
        ESP_LOGE(TAG, "INVALID ATTR VAL LENGTH");
        ESP_LOGE(TAG, "LEN: %d", param->write.len);

        status = INVALID_ATTR_VAL_LENGTH;
    }

    if(param->write.need_rsp)
    {
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, &rsp);
    }
    if(status != STATUS_OK) return ESP_OK;

    olcp_op_code_result_t result = OLCP_RES_SUCCESS;

    uint8_t type = param->write.value[1];
    object_id_list_t *object = ObjectManager_list_first_elem();

    if(type == 0 || (type >= 0x04 && type <= 0x10) || type >= 0x14)
    {
        result = OLCP_RES_INVALID_PAR;
    }
    else if(object == NULL)
    {
        ESP_LOGI(TAG, "No objects on the server");
        result = OLCP_RES_NO_OBJECT;
    }

    if(result == OLCP_RES_SUCCESS)
    {
        uint8_t *order = FilterOrder_get_order();
        *order = type;
        ESP_LOGI(TAG, "Order type: %x", *order);
        FilterOrder_make_list();
    }

    indicate_data_len = 2;
    indicate_data[1] = result;
    esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, handle_table[OPT_IDX_CHAR_OBJECT_OLCP_VAL], indicate_data_len, indicate_data, true);

    return ESP_OK;
}

static esp_err_t ObjectTransfer_write_OLCP_Request_Num(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table)
{
    esp_gatt_rsp_t rsp;
    rsp.handle = handle_table[OPT_IDX_CHAR_OBJECT_OLCP_VAL];

    uint8_t status = STATUS_OK;
    uint8_t indicate_data[7];
    uint8_t indicate_data_len = 0;
    indicate_data[0] = OLCP_OP_CODE_RESPONSE;
    indicate_data[1] = param->write.value[0];


    if(param->write.len != 1)
    {
        ESP_LOGE(TAG, "INVALID ATTR VAL LENGTH");
        ESP_LOGE(TAG, "LEN: %d", param->write.len);

        status = INVALID_ATTR_VAL_LENGTH;
    }

    if(param->write.need_rsp)
    {
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, &rsp);
    }
    if(status != STATUS_OK) return ESP_OK;

    olcp_op_code_result_t result;
    uint32_t number_of_objects = 0;
    ObjectManager_request_number(&number_of_objects, &result);
    indicate_data[2] = result;

    if(result == OLCP_RES_SUCCESS)
    {
        indicate_data_len = 7;
        memcpy(&indicate_data[3], &number_of_objects, sizeof(number_of_objects));
    }
    else
    {
        indicate_data_len = 3;
    }

    esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, handle_table[OPT_IDX_CHAR_OBJECT_OLCP_VAL], indicate_data_len, indicate_data, true);

    return ESP_OK;
}

static esp_err_t ObjectTransfer_write_OLCP_Clear_Marking(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table)
{
    esp_gatt_rsp_t rsp;
    rsp.handle = handle_table[OPT_IDX_CHAR_OBJECT_OLCP_VAL];

    uint8_t status = STATUS_OK;
    uint8_t indicate_data[2];
    uint8_t indicate_data_len = 0;
    indicate_data[0] = OLCP_OP_CODE_RESPONSE;

    if(param->write.len != 1)
    {
        ESP_LOGE(TAG, "INVALID ATTR VAL LENGTH");
        ESP_LOGE(TAG, "LEN: %d", param->write.len);

        status = INVALID_ATTR_VAL_LENGTH;
    }

    if(param->write.need_rsp)
    {
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, &rsp);
    }
    if(status != STATUS_OK) return ESP_OK;

    olcp_op_code_result_t result;
    ObjectManager_clear_marking(&result);
    indicate_data_len = 2;
    indicate_data[1] = result;
    esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, handle_table[OPT_IDX_CHAR_OBJECT_OLCP_VAL], indicate_data_len, indicate_data, true);

    return ESP_OK;
}

static esp_err_t ObjectTransfer_write_OLCP_OP_NS(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table)
{
    esp_gatt_rsp_t rsp;
    rsp.handle = handle_table[OPT_IDX_CHAR_OBJECT_OLCP_VAL];

    uint8_t indicate_data[2];
    uint8_t indicate_data_len = 0;
    indicate_data[0] = OLCP_OP_CODE_RESPONSE;

    esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, STATUS_OK, &rsp);
    ESP_LOGE(TAG, "OP CODE NOT SUPPORTED");
    indicate_data_len = 2;
    indicate_data[1] = OACP_RES_OP_CODE_NOT_SUPPORTED;
    esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, handle_table[OPT_IDX_CHAR_OBJECT_OLCP_VAL], indicate_data_len, indicate_data, true);

    return ESP_OK;
}

static esp_err_t ObjectTransfer_write_OLCP_CCC(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table)
{
    if(param->write.len == 2){
        uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
        if (descr_value == 0x0002){
            ESP_LOGI(TAG, "OLCP indicate enable");
            uint8_t indicate_data[15];
            for (int i = 0; i < sizeof(indicate_data); ++i)
            {
                indicate_data[i] = i % 0xff;
            }
        }
        else if (descr_value == 0x0000){
            ESP_LOGI(TAG, "OLCP indicate disable");
        }else{
            ESP_LOGE(TAG, "unknown descr value");;
        }
    }
    return ESP_OK;
}

static esp_err_t ObjectTransfer_write_Alarm_Action(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table)
{
    ESP_LOGI(TAG, "Object Alarm Action WRITE EVENT, payload length: %u", param->write.len);

    esp_gatt_rsp_t rsp;
    rsp.handle = handle_table[OPT_IDX_CHAR_OBJECT_ALARM_ACTION_VAL];
    esp_err_t ret;

    object_t *object;
    object = ObjectManager_get_object();
    if(object == NULL && param->write.need_rsp)
    {
        ESP_LOGE(TAG, "Object not selected");
        ret = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ERROR_OBJECT_NOT_SELECTED, &rsp);
        return ESP_OK;
    }

    if(ObjectManager_check_type(object->type.uuid.uuid128) != ALARM_TYPE)
    {
        ESP_LOGI(TAG, "Wrong type - required: Alarm");
        ret = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, WRITE_REQUEST_REJECTED, &rsp);
        return ESP_OK;
    }

    uint8_t result = set_alarm_values(param->write.value, param->write.len);
    alarm_mode_args_t alarm = get_alarm_values();

    if(result && param->write.need_rsp)
    {
        ESP_LOGI(TAG, "Wrong Alarm payload");
        ret = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, result, &rsp);
        return ESP_OK;
    }

    object->set_custom_object = true;

    ObjectManager_change_alarm_data_in_file(alarm);

    ESP_LOGI("WRITE", "Object alarm data changed");

    if(param->write.need_rsp)
    {
        ret = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, STATUS_OK, &rsp);
        if(ret) return ret;
    }

    return ESP_OK;
}

static esp_err_t ObjectTransfer_write_Ringtone_Action(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table)
{
    ESP_LOGD(TAG, "Object Ringtone Action WRITE EVENT");

    esp_gatt_rsp_t rsp;
    rsp.handle = handle_table[OPT_IDX_CHAR_OBJECT_RINGTONE_ACTION_VAL];
    esp_err_t ret;

    if(param->write.need_rsp)
    {
        ret = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, STATUS_OK, &rsp);
        if(ret) return ret;
    }

    return ESP_OK;
}

static esp_err_t ObjectTransfer_write_wifi_action(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table)
{
    ESP_LOGD(TAG, "Object Ringtone Action WRITE EVENT");

    esp_gatt_rsp_t rsp;
    rsp.handle = handle_table[OPT_IDX_CHAR_OBJECT_WIFI_ACTION_VAL];
    esp_err_t ret;

    if(param->write.need_rsp)
    {
        ret = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, STATUS_OK, &rsp);
        if(ret) return ret;
    }

    return ESP_OK;
}