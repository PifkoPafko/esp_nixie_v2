#include "ObjectTransfer_metadata_read.h"
#include "ObjectTransfer_attr_ids.h"
#include "ObjectManager.h"
#include "ObjectTransfer_defs.h"
#include "FilterOrder.h"
#include "esp_gatts_api.h"
#include "esp_err.h"
#include "esp_log.h"

#define TAG "READ_EVENT"

static esp_err_t ObjectTransfer_read_name(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table);
static esp_err_t ObjectTransfer_read_type(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table);
static esp_err_t ObjectTransfer_read_size(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table);
static esp_err_t ObjectTransfer_read_id(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table);
static esp_err_t ObjectTransfer_read_properties(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table);
static esp_err_t ObjectTransfer_read_list_filter(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table);
static esp_err_t ObjectTransfer_read_alarm_action(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table);


esp_err_t ObjectTranfer_metadata_read_event(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table)
{
    if(param->read.handle == handle_table[OPT_IDX_CHAR_OBJECT_NAME_VAL]) ObjectTransfer_read_name(gatts_if, param, handle_table);
    else if(param->read.handle == handle_table[OPT_IDX_CHAR_OBJECT_TYPE_VAL]) ObjectTransfer_read_type(gatts_if, param, handle_table);
    else if(param->read.handle == handle_table[OPT_IDX_CHAR_OBJECT_SIZE_VAL]) ObjectTransfer_read_size(gatts_if, param, handle_table);
    else if(param->read.handle == handle_table[OPT_IDX_CHAR_OBJECT_ID_VAL]) ObjectTransfer_read_id(gatts_if, param, handle_table);
    else if(param->read.handle == handle_table[OPT_IDX_CHAR_OBJECT_PROPERTIES_VAL]) ObjectTransfer_read_properties(gatts_if, param, handle_table);
    else if(param->read.handle == handle_table[OPT_IDX_CHAR_OBJECT_LIST_FILTER_VAL]) ObjectTransfer_read_list_filter(gatts_if, param, handle_table);
    else if(param->read.handle == handle_table[OPT_IDX_CHAR_OBJECT_ALARM_ACTION_VAL]) ObjectTransfer_read_alarm_action(gatts_if, param, handle_table);

    return ESP_OK;
}                           /*!< Gatt server callback param of ESP_GATTS_READ_EVT */

static esp_err_t ObjectTransfer_read_name(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table)
{
    ESP_LOGI(TAG, "Object Name READ EVENT");

    if(param->read.need_rsp)
    {
        object_t* object = ObjectManager_get_object();
        if(object == NULL)
        {
            ESP_LOGI(TAG, "Object not selected");
            esp_gatt_rsp_t rsp_error;
            rsp_error.handle = handle_table[OPT_IDX_CHAR_OBJECT_NAME_VAL];
            esp_err_t err = esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ERROR_OBJECT_NOT_SELECTED, &rsp_error);
            return ESP_OK;
        }

        esp_gatt_rsp_t rsp;
        memcpy(rsp.attr_value.value, object->name, object->name_len);
        rsp.attr_value.handle = handle_table[OPT_IDX_CHAR_OBJECT_NAME_VAL];
        rsp.attr_value.offset = 0;
        rsp.attr_value.len = object->name_len;
        rsp.attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
        esp_err_t err = esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, STATUS_OK, &rsp);

        if(err) return err;
    }

    return ESP_OK;
}

static esp_err_t ObjectTransfer_read_type(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table)
{
    ESP_LOGI(TAG, "Object Type READ EVENT");

    if(param->read.need_rsp)
    {
        object_t *object = ObjectManager_get_object();
        if(object == NULL)
        {
            ESP_LOGI(TAG, "Object not selected");
            esp_gatt_rsp_t rsp_error;
            rsp_error.handle = handle_table[OPT_IDX_CHAR_OBJECT_TYPE_VAL];
            esp_err_t err = esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ERROR_OBJECT_NOT_SELECTED, &rsp_error);
            return ESP_OK;
        }

        esp_gatt_rsp_t rsp;

        memcpy(rsp.attr_value.value, &object->type.uuid.uuid16, object->type.len);
        rsp.attr_value.handle = handle_table[OPT_IDX_CHAR_OBJECT_TYPE_VAL];
        rsp.attr_value.offset = 0;
        rsp.attr_value.len = object->type.len;
        rsp.attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
        esp_err_t err = esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, STATUS_OK, &rsp);

        if(err) return err;
    }
    return ESP_OK;
}

static esp_err_t ObjectTransfer_read_size(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table)
{
    ESP_LOGI(TAG, "Object Size READ EVENT");

    if(param->read.need_rsp)
    {
        object_t *object = ObjectManager_get_object();
        if(object == NULL)
        {
            ESP_LOGI(TAG, "Object not selected");
            esp_gatt_rsp_t rsp_error;
            rsp_error.handle = handle_table[OPT_IDX_CHAR_OBJECT_SIZE_VAL];
            esp_err_t err = esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ERROR_OBJECT_NOT_SELECTED, &rsp_error);
            return ESP_OK;
        }

        esp_gatt_rsp_t rsp;
        uint8_t object_size[8];
        memcpy(object_size, &object->size, 4);
        memcpy(&object_size[4], &object->alloc_size, 4);

        memcpy(rsp.attr_value.value, object_size, 8);
        rsp.attr_value.handle = handle_table[OPT_IDX_CHAR_OBJECT_SIZE_VAL];
        rsp.attr_value.offset = 0;
        rsp.attr_value.len = 8;
        rsp.attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
        esp_err_t err = esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, STATUS_OK, &rsp);

        if(err) return err;
    }
    return ESP_OK;
}

static esp_err_t ObjectTransfer_read_id(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table)
{
    ESP_LOGD(TAG, "Object ID READ EVENT");

    if(param->read.need_rsp)
    {
        object_t* object = ObjectManager_get_object();
        if(object == NULL)
        {
            ESP_LOGI(TAG, "Object not selected");
            esp_gatt_rsp_t rsp_error;
            rsp_error.handle = handle_table[OPT_IDX_CHAR_OBJECT_ID_VAL];
            esp_err_t err = esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ERROR_OBJECT_NOT_SELECTED, &rsp_error);
            return ESP_OK;
        }

        ESP_LOGI(TAG, "ID: %llx", object->id);

        esp_gatt_rsp_t rsp;
        memcpy(rsp.attr_value.value, &object->id, 6);
        rsp.attr_value.handle = handle_table[OPT_IDX_CHAR_OBJECT_ID_VAL];
        rsp.attr_value.offset = 0;
        rsp.attr_value.len = 6;
        rsp.attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
        esp_err_t err = esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, STATUS_OK, &rsp);

        if(err) return err;
    }

    return ESP_OK;
}

static esp_err_t ObjectTransfer_read_list_filter(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table)
{
    ESP_LOGD(TAG, "Object List Filter READ EVENT");

    if(param->read.need_rsp)
    {
        esp_gatt_rsp_t rsp;
        ListFilter_t *filter = FilterOrder_get_filter();
        rsp.attr_value.value[0] = filter->type;
        memcpy(&rsp.attr_value.value[1], filter->parameter, filter->par_length);
        rsp.attr_value.handle = handle_table[OPT_IDX_CHAR_OBJECT_ID_VAL];
        rsp.attr_value.offset = 0;
        rsp.attr_value.len = filter->par_length + 1;
        rsp.attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
        esp_err_t err = esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, STATUS_OK, &rsp);

        if(err) return err;
    }

    return ESP_OK;
}

static esp_err_t ObjectTransfer_read_properties(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table)
{
    ESP_LOGI(TAG, "Object Properties READ EVENT");

    if(param->read.need_rsp)
    {
        object_t* object = ObjectManager_get_object();

        if(object == NULL)
        {
            ESP_LOGI(TAG, "Object not selected");
            esp_gatt_rsp_t rsp_error;
            rsp_error.handle = handle_table[OPT_IDX_CHAR_OBJECT_PROPERTIES_VAL];
            esp_err_t err = esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ERROR_OBJECT_NOT_SELECTED, &rsp_error);
            return ESP_OK;
        }

        esp_gatt_rsp_t rsp;
        memcpy(rsp.attr_value.value, &object->properties, 4);
        rsp.attr_value.handle = handle_table[OPT_IDX_CHAR_OBJECT_PROPERTIES_VAL];
        rsp.attr_value.offset = 0;
        rsp.attr_value.len = 4;
        rsp.attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
        esp_err_t err = esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, STATUS_OK, &rsp);

        if(err) return err;
    }
    return ESP_OK;
}

static esp_err_t ObjectTransfer_read_alarm_action(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table)
{
    ESP_LOGI(TAG, "Object alarm data READ EVENT");

    if(param->read.need_rsp)
    {
        object_t* object = ObjectManager_get_object();
        if(object == NULL)
        {
            ESP_LOGI(TAG, "Object not selected");
            esp_gatt_rsp_t rsp_error;
            rsp_error.handle = handle_table[OPT_IDX_CHAR_OBJECT_ALARM_ACTION_VAL];
            esp_err_t err = esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ERROR_OBJECT_NOT_SELECTED, &rsp_error);
            return ESP_OK;
        }

        if(ObjectManager_check_type(object->type.uuid.uuid128) != ALARM_TYPE)
        {
            ESP_LOGI(TAG, "Wrong type - required: Alarm");
            esp_gatt_rsp_t rsp_error;
            rsp_error.handle = handle_table[OPT_IDX_CHAR_OBJECT_ALARM_ACTION_VAL];
            esp_err_t err = esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, WRITE_REQUEST_REJECTED, &rsp_error);
            return ESP_OK;
        }

        if(object->params.alarm.set == 0)
        {
            ESP_LOGI(TAG, "Alarm is not configured");
            esp_gatt_rsp_t rsp_error;
            rsp_error.handle = handle_table[OPT_IDX_CHAR_OBJECT_ALARM_ACTION_VAL];
            esp_err_t err = esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ALARM_NOT_CONFIGURED, &rsp_error);
            return ESP_OK;
        }

        ObjectManager_printf_alarm_info();

        esp_gatt_rsp_t rsp;
        uint8_t mode_len = object->params.alarm.mode==1?1:0;

        memcpy(rsp.attr_value.value, &object->params.alarm.enable, 1);
        memcpy(&rsp.attr_value.value[1], &object->params.alarm.mode, 1);
        memcpy(&rsp.attr_value.value[2], &object->params.alarm.timestamp, 4);
        if(object->params.alarm.mode == 1) memcpy(&rsp.attr_value.value[6], &object->params.alarm.days, 1);
        memcpy(&rsp.attr_value.value[6+mode_len], &object->params.alarm.description_len, 1);
        memcpy(&rsp.attr_value.value[7+mode_len], (uint8_t*)object->params.alarm.description, object->params.alarm.description_len);
        memcpy(&rsp.attr_value.value[7+mode_len+object->params.alarm.description_len], &object->params.alarm.volume, 1);
        memcpy(&rsp.attr_value.value[8+mode_len+object->params.alarm.description_len], &object->params.alarm.risingSoundEnable, 1);
        memcpy(&rsp.attr_value.value[9+mode_len+object->params.alarm.description_len], &object->params.alarm.napEnable, 1);
        memcpy(&rsp.attr_value.value[10+mode_len+object->params.alarm.description_len], &object->params.alarm.napDuration, 1);
        memcpy(&rsp.attr_value.value[11+mode_len+object->params.alarm.description_len], &object->params.alarm.napAmount, 1);

        rsp.attr_value.handle = handle_table[OPT_IDX_CHAR_OBJECT_ALARM_ACTION_VAL];
        rsp.attr_value.offset = 0;
        rsp.attr_value.len = 12+mode_len+object->params.alarm.description_len;
        rsp.attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
        esp_err_t err = esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, STATUS_OK, &rsp);

        if(err) return err;
    }
    return ESP_OK;
}