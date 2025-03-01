#ifndef __FILTER_ORDER_H__
#define __FILTER_ORDER_H__
#include "pp_object_manager.h"

typedef struct ListFilter
{
    uint8_t type;
    uint8_t parameter[NAME_LEN_MAX+1];
    uint8_t par_length;
}ListFilter_t;


void pp_filter_order_init();
ListFilter_t* pp_filter_order_get_filter(void);
uint8_t* pp_filter_order_get_order(void);
void pp_filter_order_make_list(void);

#endif