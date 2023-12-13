// to complete............
#include "ObjectManagerIdList.h"
#include "ObjectManager.h"

#define MAX_ID_VAL 0xFFFFFFFFFFFF

static object_id_list_t *first_elem = NULL;
static object_id_list_t *last_elem = NULL;

static object_id_list_t *first_elem_sort = NULL;
static object_id_list_t *last_elem_sort = NULL;

void ObjectManager_sort_list_reinit()
{
    object_id_list_t *object = first_elem_sort;
    object_id_list_t *object_next = NULL;
    while(object)
    {
        object_next = object->next;
        free(object);
        object = object_next;
    }
    first_elem_sort = NULL;
    last_elem_sort = NULL;

    object = first_elem;
    object_id_list_t *sorted_list_object = NULL;

    if(object)
    {
        first_elem_sort = (object_id_list_t *)malloc(sizeof(object_id_list_t));
        first_elem_sort->id = object->id;
        first_elem_sort->next = object->next;
        first_elem_sort->prev = object->prev;
        last_elem_sort = first_elem_sort;
        object = object->next;
        sorted_list_object = first_elem_sort;
    }

    object_id_list_t *new_elem = NULL;

    while(object)
    {
        new_elem = (object_id_list_t *)malloc(sizeof(object_id_list_t));
        new_elem->id = object->id;
        new_elem->next = NULL;
        new_elem->prev = sorted_list_object;
        last_elem_sort = new_elem;
        sorted_list_object->next = new_elem;
        sorted_list_object = sorted_list_object->next;
        object = object->next;
    }

    // sorted_list_object = first_elem;

    // while(sorted_list_object)
    // {
    //     ESP_LOGI(TAG, "ID: %llx", sorted_list_object->id);
    //     sorted_list_object = sorted_list_object->next;
    // }

    // sorted_list_object = last_elem;

    // while(sorted_list_object)
    // {
    //     ESP_LOGI(TAG, "ID: %llx", sorted_list_object->id);
    //     sorted_list_object = sorted_list_object->prev;
    // }
}

esp_err_t ObjectManager_list_delete_from_sort_list(object_id_list_t *object)
{
    if(object->prev)
    {
        object->prev->next = object->next;
    }
    else
    {
        first_elem_sort = object->next;
    }

    if(object->next)
    {
        object->next->prev = object->prev;
    }
    else
    {
        last_elem_sort = object->prev;
    }

    free(object);
    return ESP_OK;
}

object_id_list_t* ObjectManager_list_add(void)
{
    object_id_list_t *elem = first_elem;
    object_id_list_t *elem_prev = NULL;
    uint64_t new_id = 0x100;
    while(elem)
    {
        if(new_id != elem->id)
        {
            break;
        }
        new_id++;
        elem_prev = elem;
        elem = elem->next;
    }

    object_id_list_t * new_elem = (object_id_list_t *)malloc(sizeof(object_id_list_t));
    new_elem->id = new_id;
    new_elem->next = NULL;
    new_elem->prev = NULL;

    if(first_elem == NULL)
    {
        first_elem = new_elem;
        last_elem = new_elem;
        return first_elem;
    }

    if(elem_prev)
    {
        elem_prev->next = new_elem;
        new_elem->prev = elem_prev;
    }
    else
    {
        new_elem->prev = NULL;
        first_elem = new_elem;
    }

    if(elem)
    {
        new_elem->next = elem;
        elem->prev = new_elem;
    }
    else
    {
        new_elem->next = NULL;
        last_elem = new_elem;
    }

    return new_elem;
}

object_id_list_t* ObjectManager_list_add_by_id(uint64_t id)
{
    object_id_list_t *elem = first_elem;
    object_id_list_t *elem_prev = NULL;
    while(elem)
    {
        if(id < elem->id)
        {
            break;
        }

        elem_prev = elem;
        elem = elem->next;
    }

    object_id_list_t * new_elem = (object_id_list_t *)malloc(sizeof(object_id_list_t));
    new_elem->id = id;
    new_elem->next = NULL;
    new_elem->prev = NULL;

    if(first_elem == NULL)
    {
        first_elem = new_elem;
        last_elem = new_elem;
        return first_elem;
    }

    if(elem_prev)
    {
        elem_prev->next = new_elem;
        new_elem->prev = elem_prev;
    }
    else
    {
        first_elem = new_elem;
    }

    if(elem)
    {
        new_elem->next = elem;
        elem->prev = new_elem;
    }
    else
    {
        new_elem->next = NULL;
        last_elem = new_elem;
    }

    return new_elem;
}

esp_err_t ObjectManager_list_delete_by_id(uint64_t id)
{
    object_id_list_t *elem = first_elem;
    object_id_list_t *elem_prev = NULL;
    while(elem)
    {
        if(id == elem->id)
        {
            if(elem_prev)
            {
                elem_prev->next = elem->next;
            }

            if(elem == first_elem)
            {
                first_elem = elem->next;
                if (first_elem) 
                {
                    first_elem->prev = NULL;
                }
            }

            if(elem->next)
            {
                elem->next->prev = elem->prev;
            }
            else
            {
                last_elem = elem->prev;
                if (last_elem) 
                {
                    last_elem->next = NULL;
                }
            }

            free(elem);
            ObjectManager_null_current_object();
            break;
        }
        elem_prev = elem;
        elem = elem->next;
    }

    return ESP_OK;
}

object_id_list_t* ObjectManager_list_search(bool sorted, uint64_t id)
{
    object_id_list_t *elem;
    if(sorted) elem = first_elem_sort;
    else elem = first_elem;

    while(elem)
    {
        if(id == elem->id)
        {
            break;
        }
        elem = elem->next;
    }

    return elem;
}

object_id_list_t* ObjectManager_list_first_elem()
{
    return first_elem;
}

object_id_list_t* ObjectManager_list_last_elem()
{
    return last_elem;
}

object_id_list_t* ObjectManager_sort_list_first_elem()
{
    return first_elem_sort;
}

object_id_list_t* ObjectManager_sort_list_last_elem()
{
    return last_elem_sort;
}