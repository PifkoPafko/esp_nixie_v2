#ifndef __FILTER_ORDER_H__
#define __FILTER_ORDER_H__
#include "ObjectManager.h"

typedef struct ListFilter
{
    uint8_t type;
    uint8_t parameter[NAME_LEN_MAX+1];
    uint8_t par_length;
}ListFilter_t;


void FilterOrder_init();
ListFilter_t* FilterOrder_get_filter(void);
uint8_t* FilterOrder_get_order(void);
void FilterOrder_make_list(void);

#endif