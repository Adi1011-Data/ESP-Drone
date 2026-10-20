#ifndef PID_STORE_H_
#define PID_STORE_H_

#include <stdbool.h>

/* Load NVS overlay after attitudeControllerInit(). */
void pidStoreInit(void);

bool pidStoreHasSaved(void);
bool pidStoreIsDirty(void);
void pidStoreMarkLiveChanged(void);
bool pidStoreSave(void);
bool pidStoreRestoreDefaults(void);

#endif /* PID_STORE_H_ */
