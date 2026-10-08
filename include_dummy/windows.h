#define WIN32_LEAN_AND_MEAN
#include <stdint.h>
typedef unsigned long DWORD;
typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef int BOOL;
typedef long LONG;
typedef void* HANDLE;
typedef void* HWND;
typedef void* HINSTANCE;
typedef void* HDC;
typedef void* HGLRC;
typedef void* HFONT;
typedef void* HBITMAP;
typedef void* HGDIOBJ;
typedef void* HICON;
typedef void* HMENU;
typedef void* HCURSOR;
typedef void* HBRUSH;
typedef void* HPEN;
typedef struct tagRECT {
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
} RECT, *PRECT, *NPRECT, *LPRECT;
typedef struct tagPOINT {
    LONG x;
    LONG y;
} POINT, *PPOINT, *NPPOINT, *LPPOINT;
typedef struct tagSIZE {
    LONG cx;
    LONG cy;
} SIZE, *PSIZE, *LPSIZE;
#define TRUE 1
#define FALSE 0
