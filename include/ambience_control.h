#ifndef CONTROL_H
#define CONTROL_H

typedef enum {
    BRIGHTNESS_MODE = 0,
    VOLUME_MODE     = 1
} knobMode;


void control_update(void);

#endif
