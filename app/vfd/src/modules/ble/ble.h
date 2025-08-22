#ifndef __BLE_H__
#define __BLE_H__

#ifdef __cplusplus
extern "C" {
#endif 

int ble_init(void);

int ble_connect(void);

int ble_get_state(void);

void ble_set_state(int state);

#ifdef __cplusplus
}
#endif

#endif