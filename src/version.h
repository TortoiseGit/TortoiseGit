#define FILEVER        0,3,0,0
#define PRODUCTVER     0,3,0,0
#define STRFILEVER     "0, 3, 0, 0\0"
#define STRPRODUCTVER  "0, 3, 0, 0\0"

#define TSVN_VERMAJOR             0
#define TSVN_VERMINOR             3
#define TSVN_VERMICRO             0
#define TSVN_VERBUILD			  0
#define TSVN_VERDATE			  "date unknown\n"

#ifdef _WIN64
#define TSVN_PLATFORM		"64 Bit $DEVBUILD$"
#else
#define TSVN_PLATFORM		"32 Bit $DEVBUILD$"
#endif


