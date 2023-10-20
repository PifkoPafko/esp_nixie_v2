#ifndef __OBJECT_MANAGER_ID_LIST_H__
#define __OBJECT_MANAGER_ID_LIST_H__

#include "esp_err.h"
#include "string.h"
#include  <stdbool.h>

typedef struct object_id_list{
    uint64_t id;
    struct object_id_list *next;
    struct object_id_list *prev;
}object_id_list_t;

void ObjectManager_sort_list_reinit();
object_id_list_t* ObjectManager_list_add(void);
object_id_list_t* ObjectManager_list_add_by_id(uint64_t id);
esp_err_t ObjectManager_list_delete_by_id(uint64_t id);
esp_err_t ObjectManager_list_delete(object_id_list_t *object);
esp_err_t ObjectManager_list_delete_from_sort_list(object_id_list_t *object);
object_id_list_t* ObjectManager_list_search(bool sorted, uint64_t id);
object_id_list_t* ObjectManager_list_first_elem();
object_id_list_t* ObjectManager_list_last_elem();
object_id_list_t* ObjectManager_sort_list_first_elem();
object_id_list_t* ObjectManager_sort_list_last_elem();

#endif