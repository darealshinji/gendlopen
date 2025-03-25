/**
 Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 SPDX-License-Identifier: MIT
 Copyright (c) 2024-2025 Carsten Janssen

 Permission is hereby  granted, free of charge, to any  person obtaining a copy
 of this software and associated  documentation files (the "Software"), to deal
 in the Software  without restriction, including without  limitation the rights
 to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
 copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
 IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
 FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
 AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
 LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
**/

#include <rpc.h>
#include <stdio.h>

#include "example_uuid_win.h"


int print_uuid()
{
    UUID uu;
    RPC_CSTR *str = NULL;

    if (UuidCreate(&uu) != RPC_S_OK) {
        fprintf(stderr, "Failed to create UUID\n");
        return 1;
    }

    if (UuidToStringA(&uu, (RPC_CSTR *)&str) != RPC_S_OK) {
        fprintf(stderr, "Failed to create string from UUID\n");
        return 1;
    }

    printf("%s\n", (char *)str);

    RpcStringFreeA((RPC_CSTR *)&str);

    return 0;
}

int main()
{
    if (!gdo_load_lib_and_symbols()) {
        fprintf(stderr, "an error has occurred trying to dynamically load `%s':\n", GDO_DEFAULT_LIBA);
        fprintf(stderr, "%s\n", gdo_last_error());
        return 1;
    }

    int ret = print_uuid();

    gdo_free_lib();

    return ret;
}
