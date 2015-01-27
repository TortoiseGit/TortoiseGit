#define FILEVER				1,8,0,0
#define PRODUCTVER			FILEVER
#define STRFILEVER			"1.8.0.0"
#define STRPRODUCTVER		STRFILEVER

#define TGIT_VERMAJOR		1
#define TGIT_VERMINOR		8
#define TGIT_VERMICRO		12
#define TGIT_VERBUILD		0
#define TGIT_VERDATE		__DATE__

#ifdef _WIN64
#define TGIT_PLATFORM		"64 Bit"
#else
#define TGIT_PLATFORM		"32 Bit"
#endif
