/*
 * Copyright 2019 Jacques de Laval

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdarg.h>
#include <stdio.h>

int _dbg_print(const char *subject,
               const char *filename,
               int line,
               const char *format,
               ...) {
    va_list argptr;
    int n_bytes;

    va_start(argptr, format);
    n_bytes = fprintf(stderr, "%s(%s:%d): ", subject, filename, line);
    n_bytes += vfprintf(stderr, format, argptr);
    va_end(argptr);

    return n_bytes;
}
