/*
 * Local stub for GenTilingDebug when the linked AutoTiler library
 * does not provide an implementation. This keeps existing models
 * working while disabling tiling debug prints.
 */

#include "AutoTilerLib.h"
#include <stdarg.h>

void GenTilingDebug(const char *Message, ...)
{
    (void)Message;
    /* Intentionally no-op: debug output is optional for codegen. */
}

