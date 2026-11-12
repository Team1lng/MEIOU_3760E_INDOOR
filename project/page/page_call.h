#ifndef _PAGE_CALL_H
#define _PAGE_CALL_H

#include "page_common.h"
#include "device_types.h"
#include "../intercom.h"

typedef enum
{
  INTERCOM_INCOMING,
  INTERCOM_OUTGOING,
  INTERCOM_TALKING,
  INTERCOM_UNKNOW,
} intercom_status;

extern int g_call_selected_dev;

void call_set_selected_dev(int dev);
void intercom_status_set(intercom_status status);
intercom_status intercom_status_get(void);

#endif // _PAGE_CALL_H
