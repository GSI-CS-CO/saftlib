#ifndef _ECA_AC_WBM_H_
#define _ECA_AC_WBM_H_

//| Address Map ------------------------ slave ----------------------------------------------|
#define SLAVE_STATUS_GET         0x00  // r     _0x000000ff , Shows if the device is rdy/busy
#define SLAVE_MAX_MACROS_GET     0x04  // r     _0xffffffff , Shows maximum number of macros
#define SLAVE_MAX_SPACE_GET      0x08  // r     _0xffffffff , Shows maximum memory space
#define SLAVE_ENABLE_GET         0x0c  // rw    _0x00000001 , Turns device on/off
#define SLAVE_ENABLE_SET         0x10  // rw    _0x00000001 , ""
#define SLAVE_ENABLE_CLR         0x14  // rw    _0x00000001 , ""
#define SLAVE_EXEC_OWR           0x18  // wfs   _0x000000ff , Executes macro at idx
#define SLAVE_LAST_EXEC_GET      0x1c  // r     _0x000000ff , Shows idx of last executed macro
#define SLAVE_REC_OWR            0x20  // wfs   _0x000000ff , Records macro at idx
#define SLAVE_LAST_REC_GET       0x24  // r     _0x000000ff , Shows idx of last recorded macro
#define SLAVE_MACRO_QTY_GET      0x28  // r     _0x000000ff , Shows the number of macros in the ram
#define SLAVE_SPACE_LEFT_GET     0x2c  // r     _0x0000ffff , Shows number of free spaces in the RAM
#define SLAVE_CLEAR_ALL_OWR      0x30  // wsp   _0x00000001 , Clears all macros
#define SLAVE_CLEAR_IDX_OWR      0x34  // wfs   _0x000000ff , Clears macro at idx
#define SLAVE_REC_FIFO_OWR       0x38  // wf    _0xffffffff , Recording fifo. 3 word sequences: #ADR# #VAL# #META#

#endif
