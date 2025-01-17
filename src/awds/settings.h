#ifndef _SETTINGS_H__
#define _SETTINGS_H__

#include <stdint.h>

static const int BEACON_INTERVAL = 1400;
static const int TOPO_INTERVAL = 1000;

/**
 * Number of beacon periods that trigger a link failure.
 *
 * If this amount of beacon periods has passed without reception of
 * a new beacon, the link becomes inactive.
 */
static const int NR_BEACON_TRIGGER_FAIL = 8;

/**
 * Length of a TX packet queue
 *
 * This limits the number of packets enqueued for transmission on the
 * rawbasic device.
 */
static const size_t QUEUE_LENGTH = 10;

#endif //SETTINGS_H__
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
