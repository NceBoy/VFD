#include <assert.h>
#include "data.h"
#include "protocol.h"
#include "vfd_param.h"

typedef void (*protocol_cb)(Packet* in , Packet* out);

#define CMD_PARAM_SET           0x00
#define CMD_PARAM_GET           0x01

typedef struct
{
    uint32_t            cmd;
    protocol_cb         cb;
}data_item_t;

static void vfd_param_set(Packet* in , Packet* out);
static void vfd_param_get(Packet* in , Packet* out);

static const data_item_t data_tab[] = {
    {CMD_PARAM_SET , vfd_param_set},
    {CMD_PARAM_GET , vfd_param_get},
};

int data_process(Packet* in , Packet* out)
{
    for(int i = 0 ; i < sizeof(data_tab) / sizeof(data_tab[0]) ; i++)
    {
        if((data_tab[i].cmd == in->subtype) && (data_tab[i].cb != NULL))
            data_tab[i].cb(in , out);
    }
    return 0;
}


static void vfd_param_set(Packet* in ,Packet* out)
{
    assert(in->body_length == 48);
    pushAllParams(in->body);
    flushAllParams();
    create_packet(out, ACTION_REPLY, TYPE_VFD, in->target_id, in->source_id, in->subtype, NULL, 0);
}
static void vfd_param_get(Packet* in , Packet* out)
{
    assert(in->body_length == 0);
    create_packet(out, ACTION_REPLY, TYPE_VFD, in->target_id, in->source_id, in->subtype, (uint8_t*)g_vfdParam, sizeof(g_vfdParam));
}