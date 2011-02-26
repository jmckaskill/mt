/* vim: ts=4 sw=4 sts=4 et
 *
 * Copyright (c) 2009 James R. McKaskill
 *
 * This software is licensed under the stock MIT license:
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * ----------------------------------------------------------------------------
 */

/* To get off64_t */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#define DMEM_LIBRARY
#include <dmem/file.h>
#include <limits.h>

#ifdef _WIN32
#include <dmem/wchar.h>
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#endif

/* -------------------------------------------------------------------------- */

#ifdef _WIN32
int dv_append_file(d_Vector(char)* out, d_Slice(char) filename)
{ return dv_append_file_win32(out, W(""), filename); }

#else
int dv_append_file(d_Vector(char)* out, d_Slice(char) filename)
{ return dv_append_file_posix(out, C(""), filename); }

#endif

/* -------------------------------------------------------------------------- */

#ifdef _WIN32

#ifdef _WIN32_WCE
#define ROOT W("\\")
#else
#define ROOT W("\\\\?\\")
#endif

int dv_append_file_win32(d_Vector(char)* out, d_Slice(wchar) folder, d_Slice(char) relative)
{
    d_Vector(wchar) wfilename = DV_INIT;
    DWORD read;
    DWORD dwsize[2];
    uint64_t size;
    int toread;
    HANDLE file;
    char* buf;
    bool is_absolute_path = dv_begins_with(relative, C("\\")) || (relative.size > 1 && relative.data[1] == ':');

    if (is_absolute_path) {
        /* absolute path */
        dv_append(&wfilename, ROOT);

    } else if (folder.size == 0) {
        /* relative path from current working directory
         * we fake the root as CE's relative directory
         */
#ifndef _WIN32_WCE
        DWORD dirsz;
        wchar_t* buf;
        dirsz = GetCurrentDirectoryW(0, NULL);
        buf = dv_append_buffer(&wfilename, dirsz);
        GetCurrentDirectoryW(dirsz + 1, buf);
        dv_resize(&wfilename, (int) wcslen(wfilename.data));
        dv_append(&wfilename, W("\\"));
#endif

    } else {
        /* relative path from supplied folder */
        dv_append(&wfilename, folder);
        dv_append(&wfilename, W("\\"));
    }

    if (!dv_begins_with(wfilename, ROOT)) {
        dv_insert(&wfilename, 0, ROOT);
    }

    dv_append_from_utf8(&wfilename, relative);
    file = CreateFileW(wfilename.data, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    dv_free(wfilename);

    if (file == INVALID_HANDLE_VALUE) {
        return -1;
    }

    dwsize[0] = GetFileSize(file, &dwsize[1]);
    size = ((uint64_t) dwsize[1] << 32) | (uint64_t) dwsize[0];
    toread = size > INT_MAX ? INT_MAX : (int) size;
    buf = dv_append_buffer(out, toread);
    
    if ((toread > 0 && buf == NULL) || !ReadFile(file, buf, (DWORD) toread, &read, NULL) || read != (DWORD) toread) {
        CloseHandle(file);
        dv_erase_end(out, toread);
        return -1;
    }

    CloseHandle(file);
    return toread;
}
#endif

/* -------------------------------------------------------------------------- */

#ifdef _POSIX_C_SOURCE
int dv_append_file_posix(d_Vector(char)* out, d_Slice(char) folder, d_Slice(char) relative)
{
    struct stat st;
    d_Vector(char) filename = DV_INIT;
    char* buf;
    int toread = 0;
    int fd;

    if (folder.size == 0 || (relative.size && relative.data[0] == '/')) {
        dv_set(&filename, relative);
    } else {
        dv_set(&filename, folder);
        dv_append(&filename, C("/"));
        dv_append(&filename, relative);
    }

    fd = open(filename.data, O_RDONLY);
    dv_free(filename);

    if (fd < 0) {
        goto err;
    }

    if (fstat(fd, &st)) {
        goto err;
    }

    toread = st.st_size > INT_MAX ? INT_MAX : (int) st.st_size;
    buf = dv_append_buffer(out, toread);

    if ((toread > 0 && buf == NULL) || read(fd, buf, toread) != toread) {
        goto err;
    }

    close(fd);
    return toread;

err:
    dv_erase_end(out, toread);
    close(fd);
    return -1;
}
#endif

/* -------------------------------------------------------------------------- */

#ifdef DMEM_HAVE_OPENAT
int dv_append_file_openat(d_Vector(char)* out, int dfd, d_Slice(char) relative)
{
    struct stat st;
    char* buf;
    d_Vector(char) filecopy = DV_INIT;
    int toread = 0;
    int fd;

    /* need to copy the filename to ensure its null terminated */
    dv_set(&filecopy, relative);
    if (dfd >= 0) {
        fd = openat(dfd, filecopy.data, O_RDONLY);
    } else {
        fd = open(filecopy.data, O_RDONLY);
    }
    dv_free(filecopy);

    if (fd < 0) {
        goto err;
    }

    if (fstat(fd, &st)) {
        goto err;
    }

    toread = st.st_size > INT_MAX ? INT_MAX : (int) st.st_size;
    buf = dv_append_buffer(out, toread);

    if ((toread > 0 && buf == NULL) || read(fd, buf, toread) != toread) {
        goto err;
    }

    close(fd);
    return toread;    

err:
    dv_erase_end(out, toread);
    close(fd);
    return -1;
}
#endif

/* -------------------------------------------------------------------------- */


