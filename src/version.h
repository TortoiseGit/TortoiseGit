#define FILEVER        0,7,2,0
#define PRODUCTVER     0,7,2,0
#define STRFILEVER     "0, 7, 2, 0\0"
#define STRPRODUCTVER  "0, 7, 2, 0\0"

#define TSVN_VERMAJOR             0
#define TSVN_VERMINOR             7
#define TSVN_VERMICRO             2
#define TSVN_VERBUILD			  0
#define TSVN_VERDATE			  "date unknown\n"

#ifdef _WIN64
#define TSVN_PLATFORM		"64 Bit $DEVBUILD$"
#else
#define TSVN_PLATFORM		"32 Bit $DEVBUILD$"
#endif


