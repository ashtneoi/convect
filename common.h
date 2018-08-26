#pragma once


#define _POSIX_C_SOURCE 200112L


#define E_COMMON 1
#define E_RARE 2
#define E_USAGE 3


#define print(x) fputs((x), stdout)
#define lengthof(x) (sizeof(x) / sizeof((x)[0]))
