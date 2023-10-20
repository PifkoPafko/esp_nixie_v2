#ifndef __OBJECT_TRANSFER_DEFS_H__
#define __OBJECT_TRANSFER_DEFS_H__

//ATTRIBUTE GENERAL ERROR CODES
#define STATUS_OK                       ((uint8_t)0x00)
#define INVALID_ATTR_VAL_LENGTH         ((uint8_t)0x0D)
#define ERROR_INSUFFICIENT_RESOURCES    ((uint8_t)0x11)
#define WRITE_REQUEST_REJECTED          ((uint8_t)0x80)
#define ERROR_OBJECT_NOT_SELECTED       ((uint8_t)0x81)
#define ALARM_NOT_CONFIGURED            ((uint8_t)0x82)

//OACP RESULT CODES
typedef enum{
    OACP_RES_SUCCESS = 0x01,
    OACP_RES_OP_CODE_NOT_SUPPORTED,
    OACP_RES_INVALID_PAR,
    OACP_RES_INSUF_RSR,
    OACP_RES_INVALID_OBJECT,
    OACP_RES_CHANNEL_UNAVBL,
    OACP_RES_UNSUPPORTED_TYPE,
    OACP_RES_PROCEDURE_NOT_PERMIT,
    OACP_RES_OBJECT_LOCKED,
    OACP_RES_OPERATION_FAILED
}oacp_op_code_result_t;

typedef enum{
    OLCP_RES_SUCCESS = 0x01,
    OLCP_RES_OP_CODE_NOT_SUPPORTED,
    OLCP_RES_INVALID_PAR,
    OLCP_RES_OPERATION_FAILED,
    OLCP_RES_OUT_OF_THE_BONDS,
    OLCP_RES_TOO_MANY_OJECTS,
    OLCP_RES_NO_OBJECT,
    OLCP_RES_OBJECT_NOT_FOUND
}olcp_op_code_result_t;


//OACP OP CODES
#define OACP_OP_CODE_CREATE                  ((uint8_t)0x01)
#define OACP_OP_CODE_DELETE                  ((uint8_t)0x02)
#define OACP_OP_CODE_CALC_SUM                ((uint8_t)0x03)
#define OACP_OP_CODE_EXECUTE                 ((uint8_t)0x04)
#define OACP_OP_CODE_READ                    ((uint8_t)0x05)
#define OACP_OP_CODE_WRITE                   ((uint8_t)0x06)
#define OACP_OP_CODE_ABORT                   ((uint8_t)0x07)
#define OACP_OP_CODE_RESPONSE                ((uint8_t)0x60)

//OLCP OP CODES
#define OLCP_OP_CODE_FIRST                   ((uint8_t)0x01)
#define OLCP_OP_CODE_LAST                    ((uint8_t)0x02)
#define OLCP_OP_CODE_PREVIOUS                ((uint8_t)0x03)
#define OLCP_OP_CODE_NEXT                    ((uint8_t)0x04)
#define OLCP_OP_CODE_GOTO                    ((uint8_t)0x05)
#define OLCP_OP_CODE_ORDER                   ((uint8_t)0x06)
#define OLCP_OP_CODE_REQ_NUM_OF_OBJ          ((uint8_t)0x07)
#define OLCP_OP_CODE_CLEAR_MARING            ((uint8_t)0x08)
#define OLCP_OP_CODE_RESPONSE                ((uint8_t)0x70)

//Object Type Create data length
#define DATA_LEN_UUID16                 7
#define DATA_LEN_UUID32                 9
#define DATA_LEN_UUID128                21

//Filter OP CODES
#define NO_FILTER                       0x00
#define NAME_STARTS_WITH                0x01
#define NAME_ENDS_WITH                  0x02
#define NAME_CONTAINS                   0x03
#define NAME_IS_EXACTLY                 0x04
#define OBJECT_TYPE                     0x05
#define CURRENT_SIZE_BETWEEN            0x08
#define ALLOC_SIZE_BETWEEN              0x09
#define MARKED_OBJECTS                  0x0A

//Order op codes
#define NAME_ASC                        0x01
#define TYPE_ASC                        0x02
#define CURRENT_SIZE_ASC                0x03
#define NAME_DESC                       0x11
#define TYPE_DESC                       0x12
#define CURRENT_SIZE_DESC               0x13

#endif