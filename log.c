#include <stdarg.h>
#include "log.h"

void lookup_log(int level, const char *fmt,...) {
	if (logLevel <= level) {
		va_list ap;
		va_start(ap, fmt);
		vprintf(fmt, ap);
		va_end(ap);
	}
}