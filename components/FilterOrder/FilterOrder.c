#include "FilterOrder.h"
#include "ObjectManager.h"
#include "ObjectManagerIdList.h"
#include "ObjectTransfer_defs.h"
#include "esp_log.h"
#include "stdlib.h"
#include "string.h"
#include "ctype.h"

#define TAG "FILTERORDER"

typedef int (*compare_function)(uint64_t, uint64_t, bool);       //par bool: 1-ascending, 0-descending
typedef bool (*compare_function_filter)(uint64_t);

static ListFilter_t filter;
static uint8_t order;

static void FilterOrder_sort(compare_function fun, bool asc);
static void FilterOrder_filter(compare_function_filter fun);

static int name_compare(uint64_t rID, uint64_t lID, bool asc);
static int type_compare(uint64_t rID, uint64_t lID, bool asc);
static int size_compare(uint64_t rID, uint64_t lID, bool asc);

static char* read_name_from_file(char* dest, uint64_t id);
static uint8_t* read_type_from_file(uint8_t* dest, uint64_t id);
static uint32_t read_current_size_from_file(uint64_t id);

static bool name_starts_with(uint64_t id);
static bool name_ends_with(uint64_t id);
static bool name_containts(uint64_t id);
static bool name_is_exactly(uint64_t id);
static bool object_type(uint64_t id);
static bool current_size_between(uint64_t id);
static bool alloc_size_between(uint64_t id);
static bool marked_objects(uint64_t id);


void FilterOrder_init()
{
    order = 0;
    filter.type = 0;
    filter.par_length = 0;
}

ListFilter_t* FilterOrder_get_filter(void)
{
    return &filter;
}

uint8_t* FilterOrder_get_order(void)
{
    return &order;
}

void FilterOrder_make_list(void)
{
    ObjectManager_sort_list_reinit();
    ESP_LOGI(TAG, "ORDER OP Code: %x", order);
    ESP_LOGI(TAG, "Filter OP Code: %x", filter.type);
    switch(order)
    {
        case NAME_ASC:
            ESP_LOGI(TAG, "Sorting by name, ascending");
            FilterOrder_sort(name_compare, true);
            break;

        case TYPE_ASC:
            ESP_LOGI(TAG, "Sorting by type, ascending");
            FilterOrder_sort(type_compare, true);
            break;

        case CURRENT_SIZE_ASC:
            ESP_LOGI(TAG, "Sorting by current size, ascending");
            FilterOrder_sort(size_compare, true);
            break;

        case NAME_DESC:
            ESP_LOGI(TAG, "Sorting by name, descending");
            FilterOrder_sort(name_compare, false);
            break;

        case TYPE_DESC:
            ESP_LOGI(TAG, "Sorting by type, descending");
            FilterOrder_sort(type_compare, false);
            break;

        case CURRENT_SIZE_DESC:
            ESP_LOGI(TAG, "Sorting by current size, descending");
            FilterOrder_sort(size_compare, false);
            break;

        default:
            break;
    }

    switch(filter.type)
    {
        case NO_FILTER:
            ESP_LOGI(TAG, "No filter");
            break;

        case NAME_STARTS_WITH:
            ESP_LOGI(TAG, "'Name starts with' filter");
            FilterOrder_filter(name_starts_with);
            break;

        case NAME_ENDS_WITH:
            ESP_LOGI(TAG, "Name ends with filter");
            FilterOrder_filter(name_ends_with);
            break;

        case NAME_CONTAINS:
            ESP_LOGI(TAG, "Name contains filter");
            FilterOrder_filter(name_containts);
            break;

        case NAME_IS_EXACTLY:
            ESP_LOGI(TAG, "Name is exactly filter");
            FilterOrder_filter(name_is_exactly);
            break;

        case OBJECT_TYPE:
            ESP_LOGI(TAG, "Object type filter");
            FilterOrder_filter(object_type);
            break;

        case CURRENT_SIZE_BETWEEN:
            ESP_LOGI(TAG, "Current size between filter");
            FilterOrder_filter(current_size_between);
            break;

        case ALLOC_SIZE_BETWEEN:
            ESP_LOGI(TAG, "Allocated size between filter");
            FilterOrder_filter(alloc_size_between);
            break;

        case MARKED_OBJECTS:
            ESP_LOGI(TAG, "Marked objects filter");
            FilterOrder_filter(marked_objects);
            break;
    }
    ESP_LOGI(TAG, "Sorting and filtering done");

    object_t* object = ObjectManager_get_object();

    if(object)
    {
        if(ObjectManager_list_search(true, object->id) == NULL)
        {
            ObjectManager_null_current_object();
        }
    }
}

static void FilterOrder_sort(compare_function fun, bool asc)
{
    int swapped;
    object_id_list_t *rptr = ObjectManager_sort_list_first_elem();
    object_id_list_t *lptr = NULL;

    /* Checking for empty list */
    if (rptr == NULL)
        return;

    do
    {
        swapped = 0;
        rptr = ObjectManager_sort_list_first_elem();

        while (rptr->next != lptr)
        {
            if (fun(rptr->id, rptr->next->id, asc) > 0)
            {
                uint64_t temp = rptr->id;
                rptr->id = rptr->next->id;
                rptr->next->id = temp;

                swapped = 1;
            }
            rptr = rptr->next;
        }
        lptr = rptr;
    }
    while (swapped);

}

static int name_compare(uint64_t rID, uint64_t lID, bool asc)
{
    char rName[NAME_LEN_MAX];
    char lName[NAME_LEN_MAX];

    read_name_from_file(rName, rID);
    read_name_from_file(lName, lID);

    uint8_t rName_len = strlen(rName);
    uint8_t lName_len = strlen(lName);
    uint8_t len = (rName_len-lName_len)?lName_len:rName_len;
    int16_t cmp;
    bool right_is_first;
    bool compared = false;

    for(int i=0; i<len; i++)
    {
        cmp = tolower(rName[i]) - tolower(lName[i]);
        if(cmp < 0)
        {
            right_is_first = true;
            compared = true;
            break;
        }
        else if(cmp > 0)
        {
            right_is_first = false;
            compared = true;
            break;
        }
        else
        {

        }
    }

    if(compared == false)
    {
        for(int i=0; i<len; i++)
        {
            if(rName[i] > lName[i])
            {
                right_is_first = true;
                compared = true;
                break;
            }
            else if(rName[i] < lName[i])
            {
                right_is_first = false;
                compared = true;
                break;
            }
        }
    }

    if(compared == false)
    {
        if(rName_len < lName_len)
        {
            right_is_first = true;
        }
        else
        {
            right_is_first = false;
        }
    }

    if(right_is_first) cmp = -1;
    else cmp = 1;
    if(asc == false) cmp = -cmp;

    return cmp;
}

static int type_compare(uint64_t rID, uint64_t lID, bool asc)
{
    uint8_t rType[16];
    uint8_t lType[16];

    read_type_from_file(rType, rID);
    read_type_from_file(lType, lID);

    int cmp = memcmp(rType, lType, 16);
    if(asc == false) cmp = -cmp;

    return cmp;
}

static int size_compare(uint64_t rID, uint64_t lID, bool asc)
{
    uint32_t rSize = read_current_size_from_file(rID);
    uint32_t lSize = read_current_size_from_file(lID);

    int cmp;
    if(rSize>lSize) cmp = 1;
    else if(rSize<lSize) cmp = -1;
    else cmp = 0;

    if(asc == false) cmp = -cmp;

    return cmp;
}

static char* read_name_from_file(char* dest, uint64_t id)
{
    char file[30];
    strcpy(file, "/spiffs/");
    itoa(id, &file[8], 16);
    strcat(file, ".txt");

    FILE* f = fopen(file, "r");
    char line[50];

    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);

    char name_len_str[20];
    fgets(name_len_str, sizeof(name_len_str), f);
    uint8_t name_len = atoi(&name_len_str[13]);

    strncpy(dest, &line[6], name_len);
    dest[name_len] = '\0';
    fclose(f);

    return dest;
}

static uint8_t* read_type_from_file(uint8_t* dest, uint64_t id)
{
    char file[30];
    strcpy(file, "/spiffs/");
    itoa(id, &file[8], 16);
    strcat(file, ".txt");

    FILE* f = fopen(file, "r");
    char line[50];

    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);

    char *ptr;
    char uuid_byte_str[3];
    uint8_t uuid_byte;
    for(int i=15; i>=0; i--)
    {
        strncpy(uuid_byte_str, &line[6+(15-i)*2], 2);
        uuid_byte_str[2] = '\0';
        uuid_byte = strtol(uuid_byte_str, &ptr, 16);
        dest[i] = uuid_byte;
    }

    fclose(f);

    return dest;
}

static uint32_t read_current_size_from_file(uint64_t id)
{
    char file[30];
    strcpy(file, "/spiffs/");
    itoa(id, &file[8], 16);
    strcat(file, ".txt");

    FILE* f = fopen(file, "r");
    char line[50];

    fgets(line, sizeof(line), f);

    char *ptr;
    uint32_t size = strtol(&line[6], &ptr, 16);

    fclose(f);

    return size;
}

static void FilterOrder_filter(compare_function_filter fun)
{
    object_id_list_t *object = ObjectManager_sort_list_first_elem();

    while(object)
    {
        if(fun(object->id))
        {
            ObjectManager_list_delete_from_sort_list(object);
        }
        object = object->next;
    }
}

static bool name_starts_with(uint64_t id)
{
    char name[NAME_LEN_MAX];
    read_name_from_file(name, id);
    uint8_t name_len = strlen(name);

    if(filter.par_length > name_len)
    {
        return true;
    }

    if(strncmp(name, (char*)filter.parameter, filter.par_length))
    {
        return true;
    }

    return false;
}

static bool name_ends_with(uint64_t id)
{
    char name[NAME_LEN_MAX];
    read_name_from_file(name, id);
    uint8_t name_len = strlen(name);

    if(filter.par_length > name_len)
    {
        return true;
    }

    if(strncmp(&name[name_len-filter.par_length], (char*)filter.parameter, filter.par_length))
    {
        return true;
    }

    return false;
}

static bool name_containts(uint64_t id)
{
    char name[NAME_LEN_MAX];
    read_name_from_file(name, id);
    uint8_t name_len = strlen(name);

    if(filter.par_length > name_len)
    {
        return true;
    }

    char name_search[NAME_LEN_MAX];
    strncpy(name_search, (char*)filter.parameter, filter.par_length);
    name_search[filter.par_length] = '\0';

    if(strstr(name, name_search) == NULL)
    {
        return true;
    }

    return false;
}

static bool name_is_exactly(uint64_t id)
{
    char name[NAME_LEN_MAX];
    read_name_from_file(name, id);
    uint8_t name_len = strlen(name);

    if(filter.par_length != name_len)
    {
        return true;
    }

    char name_search[NAME_LEN_MAX];
    strncpy(name_search, (char*)filter.parameter, filter.par_length);
    name_search[filter.par_length] = '\0';

    if(strcmp(name, name_search))
    {
        return true;
    }

    return false;
}

static bool object_type(uint64_t id)
{
    uint8_t uuid[16];
    read_type_from_file(uuid, id);

    if(memcmp(uuid, filter.parameter, 16))
    {
        return true;
    }

    return false;
}

static bool current_size_between(uint64_t id)
{
    uint32_t current_size = read_current_size_from_file(id);
    uint32_t size_left, size_right;

    memcpy(&size_left, filter.parameter, 4);
    memcpy(&size_right, &filter.parameter[4], 4);

    if(current_size < size_left || current_size > size_right)
    {
        return true;
    }

    return false;
}

static bool alloc_size_between(uint64_t id)
{
    char file[30];
    strcpy(file, "/spiffs/");
    itoa(id, &file[8], 16);
    strcat(file, ".txt");

    FILE* f = fopen(file, "r");
    char line[50];

    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);

    char *ptr;
    uint32_t alloc_size = strtol(&line[16], &ptr, 16);
    fclose(f);

    uint32_t size_left, size_right;

    memcpy(&size_left, filter.parameter, 4);
    memcpy(&size_right, &filter.parameter[4], 4);

    if(alloc_size < size_left || alloc_size > size_right)
    {
        return true;
    }

    return false;
}

static bool marked_objects(uint64_t id)
{
    char file[30];
    strcpy(file, "/spiffs/");
    itoa(id, &file[8], 16);
    strcat(file, ".txt");

    FILE* f = fopen(file, "r");
    char line[50];

    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);

    char *ptr;
    uint32_t properties = strtol(&line[12], &ptr, 16);

    fclose(f);

    if(properties & PROPERTY_MARK)
    {
        return false;
    }

    return true;
}