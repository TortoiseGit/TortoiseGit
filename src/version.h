#define FILEVER        1,7,3,0
#define PRODUCTVER     1,7,3,0
#define STRFILEVER     "1, 7, 3, 0\0"
#define STRPRODUCTVER  "1, 7, 3, 0\0"

#define TSVN_VERMAJOR             1
#define TSVN_VERMINOR             7
#define TSVN_VERMICRO             3
#define TSVN_VERBUILD			  0
#define TSVN_VERDATE			  __DATE__

#ifdef _WIN64
#define TSVN_PLATFORM		"64 Bit"
#else
#define TSVN_PLATFORM		"32 Bit"
#endif


