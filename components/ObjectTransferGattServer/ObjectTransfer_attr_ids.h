/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef __OBJECT_TRANSFER_ATTR_IDS_H__
#define __OBJECT_TRANSFER_ATTR_IDS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Attributes State Machine */
enum
{
    OPT_IDX_SVC,

    OPT_IDX_CHAR_OTS_FEATURE,
    OPT_IDX_CHAR_OTS_FEATURE_VAL,

    OPT_IDX_CHAR_OBJECT_NAME,
    OPT_IDX_CHAR_OBJECT_NAME_VAL,

    OPT_IDX_CHAR_OBJECT_TYPE,
    OPT_IDX_CHAR_OBJECT_TYPE_VAL,

    OPT_IDX_CHAR_OBJECT_SIZE,
    OPT_IDX_CHAR_OBJECT_SIZE_VAL,


    OPT_IDX_CHAR_OBJECT_ID,
    OPT_IDX_CHAR_OBJECT_ID_VAL,

    OPT_IDX_CHAR_OBJECT_PROPERTIES,
    OPT_IDX_CHAR_OBJECT_PROPERTIES_VAL,

    OPT_IDX_CHAR_OBJECT_OACP,
    OPT_IDX_CHAR_OBJECT_OACP_VAL,
    OPT_IDX_CHAR_OBJECT_OACP_IND_CFG,

    OPT_IDX_CHAR_OBJECT_OLCP,
    OPT_IDX_CHAR_OBJECT_OLCP_VAL,
    OPT_IDX_CHAR_OBJECT_OLCP_IND_CFG,

    OPT_IDX_CHAR_OBJECT_LIST_FILTER,
    OPT_IDX_CHAR_OBJECT_LIST_FILTER_VAL,

    OPT_IDX_CHAR_OBJECT_ALARM_ACTION,
    OPT_IDX_CHAR_OBJECT_ALARM_ACTION_VAL,

    OPT_IDX_NB,
};


#endif