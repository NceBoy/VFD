#include <assert.h>
#include "data.h"
#include "protocol.h"
#include "vfd_param.h"
#include "inout.h"

typedef int (*protocol_cb)(Packet* in , Packet* out);

#define CMD_PARAM_SET           0x000300FF
#define CMD_MOTOR_SET           0x00030104



#define CMD_PARAM_GET           0x000200FF




typedef struct
{
    uint32_t            cmd;
    protocol_cb         cb;
}data_item_t;

static int vfd_param_set(Packet* in , Packet* out);
static int vfd_param_get(Packet* in , Packet* out);
static int vfd_motor_set(Packet* in , Packet* out);


static const data_item_t data_tab[] = {
    {CMD_PARAM_SET , vfd_param_set},
    {CMD_PARAM_GET , vfd_param_get},
    {CMD_MOTOR_SET , vfd_motor_set},
};

int data_process(Packet* in , Packet* out)
{
    if(in->type != TYPE_VFD)
        return -1;
    uint32_t cmd = (in->action << 16) | in->subtype ;
    for(int i = 0 ; i < sizeof(data_tab) / sizeof(data_tab[0]) ; i++)
    {
        if((data_tab[i].cmd == cmd) && (data_tab[i].cb != NULL))
        {
            return data_tab[i].cb(in , out);
        }
            
    }
    return -1;
}


static int vfd_param_set(Packet* in ,Packet* out)
{
    assert(in->body_length == 48);
    pushAllParams(in->body);
    flushAllParams();
    uint8_t result = 1;
    create_packet(out, ACTION_REPLY, TYPE_VFD, in->target_id, in->source_id, in->subtype, &result, 1);
    return 1;
}
static int vfd_param_get(Packet* in , Packet* out)
{
    assert(in->body_length == 0);
    create_packet(out, ACTION_REPLY, TYPE_VFD, in->target_id, in->source_id, in->subtype, (uint8_t*)g_vfdParam, sizeof(g_vfdParam));
    return sizeof(g_vfdParam);
}

static int vfd_motor_set(Packet* in , Packet* out)
{
    assert(in->body_length == 1);
    if(in->body[0])
        motor_stop_ctl();
    else
        motor_start_ctl();
    uint8_t result = 1;
    create_packet(out, ACTION_REPLY, TYPE_VFD, in->target_id, in->source_id, in->subtype, &result, 1);
    return 1;    
    
}