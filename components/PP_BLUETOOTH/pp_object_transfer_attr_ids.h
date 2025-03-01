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

#ifndef __OBJECT_TRANSFER_ATTR_IDS_H__
#define __OBJECT_TRANSFER_ATTR_IDS_H__

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

    // OPT_IDX_CHAR_OBJECT_RINGTONE_ACTION,
    // OPT_IDX_CHAR_OBJECT_RINGTONE_ACTION_VAL,
    // OPT_IDX_CHAR_OBJECT_RINGTONE_ACTION_CFG,

    OPT_IDX_CHAR_OBJECT_WIFI_ACTION,
    OPT_IDX_CHAR_OBJECT_WIFI_ACTION_VAL,
    OPT_IDX_CHAR_OBJECT_WIFI_ACTION_CFG,

    OPT_IDX_NB,
};

#endif