#define FGSTAT          0x6088
#define SWI_STATUS  0x1c

// FG lm32 shared memory layout
#define STARTSHARED     0x500
#define BOARD_ID        0x500
#define BOARD_TEMP      0x508
#define EXT_ID          0x510
#define EXT_TEMP        0x518
#define BACKPLANE_ID    0x520
#define BACKPLANE_TEMP  0x528
#define FG_VERSION      0x530
#define FG_MAGIC_NUMBER 0x52c
#define FG_BUFFER       0x534
#define NUM_FGS_FOUND   0x6084
#define FGSTAT          0x6088

//device ID
#define LM32_RAM_USER_VENDOR      0x651       //vendor ID
#define LM32_RAM_USER_PRODUCT     0x54111351  //product ID
#define LM32_RAM_USER_VMAJOR      1           //major revision
#define LM32_RAM_USER_VMINOR      1           //minor revision
