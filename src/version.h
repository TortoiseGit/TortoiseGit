#define FILEVER        1,3,5,0
#define PRODUCTVER     1,3,5,0
#define STRFILEVER     "1, 3, 5, 0\0"
#define STRPRODUCTVER  "1, 3, 5, 0\0"

#define TSVN_VERMAJOR             1
#define TSVN_VERMINOR             3
#define TSVN_VERMICRO             5
#define TSVN_VERBUILD			  0
#define TSVN_VERDATE			  "date unknown\n"

#ifdef _WIN64
#define TSVN_PLATFORM		"64 Bit $DEVBUILD$"
#else
#define TSVN_PLATFORM		"32 Bit $DEVBUILD$"
#endif


