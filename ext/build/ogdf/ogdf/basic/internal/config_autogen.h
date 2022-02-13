#define OGDF_MEMORY_POOL_TS

#if !defined(NDEBUG)
	#define OGDF_DEBUG
#endif

#ifdef OGDF_DEBUG
/* #undef OGDF_HEAVY_DEBUG */
/* #undef OGDF_USE_ASSERT_EXCEPTIONS */
	#ifdef OGDF_USE_ASSERT_EXCEPTIONS
		#define OGDF_FUNCTION_NAME 
	#endif
#endif

/* #undef OGDF_DLL */

#define OGDF_SIZEOF_POINTER sizeof(std::intptr_t)
#define COIN_OSI_CLP
