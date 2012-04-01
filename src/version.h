#define FILEVER        1,7,8,0
#define PRODUCTVER     1,7,8,0
#define STRFILEVER     "1, 7, 8, 0\0"
#define STRPRODUCTVER  "1, 7, 8, 0\0"

#define TGIT_VERMAJOR             1
#define TGIT_VERMINOR             7
#define TGIT_VERMICRO             8
#define TGIT_VERBUILD			  0
#define TGIT_VERDATE			  __DATE__

#ifdef _WIN64
#define TGIT_PLATFORM		"64 Bit"
#else
#define TGIT_PLATFORM		"32 Bit"
#endif


