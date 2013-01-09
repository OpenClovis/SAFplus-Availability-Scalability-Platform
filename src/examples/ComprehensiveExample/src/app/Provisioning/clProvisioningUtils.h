#include <stdio.h>

#define min(x,y) ((x<y)? x: y)

struct ClProvisioningTime {
    ClUint32T hour;
    ClUint32T min;
    ClUint32T sec;
};

typedef struct ClProvisioningTime ClProvisioningTimeT;
