#ifndef LV_DRV_CONF_H
#define LV_DRV_CONF_H
#include "lv_conf.h"

#ifndef USE_SDL
# define USE_SDL 1
#endif

#if USE_SDL
    #define SDL_HOR_RES     800
    #define SDL_VER_RES     480
    #define SDL_ZOOM        1
    #define SDL_DOUBLE_BUFFER 0
    #define SDL_INCLUDE_PATH <SDL2/SDL.h>
#endif

#ifndef USE_MOUSE
# define USE_MOUSE 1
#endif
#ifndef USE_MOUSEWHEEL
# define USE_MOUSEWHEEL 1
#endif
#ifndef USE_KEYBOARD
# define USE_KEYBOARD 1
#endif
#endif
