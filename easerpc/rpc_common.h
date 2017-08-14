#define DEALER_ADDR	"inproc://easerpc_dealer_%d"

#define REQ_BUF_MAXSIZE 1024
#define REP_BUF_MAXSIZE 1024

#ifdef _DEBUG
#	define PRINT(...) fprintf(stdout, __VA_ARGS__);
#else
#	define PRINT(fmt) do {} while (0);
#endif