#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define GPIO_OUTPUT_OE    GPIO_NUM_3
#define GPIO_OUTPUT_PIN_SEL  (1ULL<<GPIO_OUTPUT_OE)

extern QueueHandle_t mess_queue_hdl;

#define MESSAGE_QUEUE_LEGNTH 10

typedef struct nixie_tube_state
{
    bool digit_enable;
    uint8_t digit;
    uint8_t left_comma_enable;
    uint8_t right_comma_enable;
} nixie_tube_state_t;

typedef struct display_mess
{
    nixie_tube_state_t nixie[16];
}display_mess_t;

esp_err_t pp_nixie_diplay_init();
bool pp_nixie_display_generate_i2c_msg(uint8_t expander_id, uint8_t *msg);

#define NIXIE_FIRST_ID      0
#define NIXIE_SECOND_ID     1
#define NIXIE_THIRD_ID      2

#define NIXIE_0_ID          NIXIE_FIRST_ID
#define NIXIE_1_ID          NIXIE_SECOND_ID
#define NIXIE_2_ID          NIXIE_THIRD_ID

#define NIXIE_0_0_BIT       (1<<3)
#define NIXIE_0_1_BIT       (1<<6)
#define NIXIE_0_2_BIT       (1<<7)
#define NIXIE_0_3_BIT       (1<<0)
#define NIXIE_0_4_BIT       (1<<1)
#define NIXIE_0_5_BIT       (1<<2)
#define NIXIE_0_6_BIT       (1<<3)
#define NIXIE_0_7_BIT       (1<<4)
#define NIXIE_0_8_BIT       (1<<5)
#define NIXIE_0_9_BIT       (1<<2)
#define NIXIE_0_LC_BIT      (1<<5)
#define NIXIE_0_RC_BIT      (1<<4)

#define NIXIE_1_0_BIT       (1<<5)
#define NIXIE_1_1_BIT       (1<<0)
#define NIXIE_1_2_BIT       (1<<1)
#define NIXIE_1_3_BIT       (1<<6)
#define NIXIE_1_4_BIT       (1<<7)
#define NIXIE_1_5_BIT       (1<<0)
#define NIXIE_1_6_BIT       (1<<1)
#define NIXIE_1_7_BIT       (1<<2)
#define NIXIE_1_8_BIT       (1<<3)
#define NIXIE_1_9_BIT       (1<<4)
#define NIXIE_1_LC_BIT      (1<<7)
#define NIXIE_1_RC_BIT      (1<<6)

#define NIXIE_2_0_BIT       (1<<7)
#define NIXIE_2_1_BIT       (1<<2)
#define NIXIE_2_2_BIT       (1<<3)
#define NIXIE_2_3_BIT       (1<<4)
#define NIXIE_2_4_BIT       (1<<5)
#define NIXIE_2_5_BIT       (1<<6)
#define NIXIE_2_6_BIT       (1<<7)
#define NIXIE_2_7_BIT       (1<<0)
#define NIXIE_2_8_BIT       (1<<1)
#define NIXIE_2_9_BIT       (1<<6)
#define NIXIE_2_LC_BIT      (1<<1)
#define NIXIE_2_RC_BIT      (1<<0)

#define NIXIE_0_0_REG_ID       4
#define NIXIE_0_1_REG_ID       4
#define NIXIE_0_2_REG_ID       4
#define NIXIE_0_3_REG_ID       0
#define NIXIE_0_4_REG_ID       0
#define NIXIE_0_5_REG_ID       0
#define NIXIE_0_6_REG_ID       0
#define NIXIE_0_7_REG_ID       0
#define NIXIE_0_8_REG_ID       0
#define NIXIE_0_9_REG_ID       4
#define NIXIE_0_LC_REG_ID      4
#define NIXIE_0_RC_REG_ID      4

#define NIXIE_1_0_REG_ID       3
#define NIXIE_1_1_REG_ID       4
#define NIXIE_1_2_REG_ID       4
#define NIXIE_1_3_REG_ID       0
#define NIXIE_1_4_REG_ID       0
#define NIXIE_1_5_REG_ID       1
#define NIXIE_1_6_REG_ID       1
#define NIXIE_1_7_REG_ID       1
#define NIXIE_1_8_REG_ID       1
#define NIXIE_1_9_REG_ID       3
#define NIXIE_1_LC_REG_ID      3
#define NIXIE_1_RC_REG_ID      3

#define NIXIE_2_0_REG_ID       2
#define NIXIE_2_1_REG_ID       3
#define NIXIE_2_2_REG_ID       3
#define NIXIE_2_3_REG_ID       1
#define NIXIE_2_4_REG_ID       1
#define NIXIE_2_5_REG_ID       1
#define NIXIE_2_6_REG_ID       1
#define NIXIE_2_7_REG_ID       2
#define NIXIE_2_8_REG_ID       2
#define NIXIE_2_9_REG_ID       2
#define NIXIE_2_LC_REG_ID      3
#define NIXIE_2_RC_REG_ID      3