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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "mt-internal.h"
#include <mt/filesystem.h>
#include <mt/time.h>
#include <dmem/file.h>
#include <dmem/wchar.h>

#ifdef _WIN32
#include <dmem/wchar.h>
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#endif


/* -------------------------------------------------------------------------- */

MT_Directory* MT_OpenDirectory(d_Slice(char) folder)
{ return MT_OpenDirectory2(NULL, folder); }

/* -------------------------------------------------------------------------- */

#ifdef _WIN32

struct MT_Directory {
    d_Vector(wchar) wname;
    d_Vector(char) utf8ent;
    HANDLE finder;
    WIN32_FIND_DATAW data;
    bool used_first;
};

MT_Directory* MT_OpenDirectory2(MT_Directory* dir, d_Slice(char) relative)
{
    MT_Directory* newdir = NEW(MT_Directory);

    if (dir) {
        dv_append(&newdir->wname, dir->wname);
        dv_append(&newdir->wname, W("\\"));
    } else {
        dv_append(&newdir->wname, W("\\\\?\\"));
    }

    dv_append_from_utf8(&newdir->wname, relative);

    dv_append(&newdir->wname, W("\\*"));
    newdir->finder = FindFirstFileW(newdir->wname.data, &newdir->data);

    if (newdir->finder == INVALID_HANDLE_VALUE) {
        MT_CloseDirectory(newdir);
        return NULL;
    }

    dv_erase_end(&newdir->wname, W("\\*").size);

    return newdir;
}

void MT_CloseDirectory(MT_Directory* dir)
{
    if (dir) {
        dv_free(dir->wname);
        dv_free(dir->utf8ent);
        FindClose(dir->finder);
        free(dir);
    }
}

bool MT_HaveNextDirEntry(MT_Directory* dir, MT_DirEntry* dirent)
{
    if (dir->used_first) {
        if (!FindNextFileW(dir->finder, &dir->data)) {
            return false;
        }
    }

    dir->used_first = true;
    dv_clear(&dir->utf8ent);
    dv_append_from_utf16(&dir->utf8ent, dv_wchar(dir->data.cFileName));
    dirent->name = dir->utf8ent;
    dirent->size = ((uint64_t) dir->data.nFileSizeHigh << 32) | (uint64_t) dir->data.nFileSizeLow;
    dirent->modify_time = MT_FromFileTime(&dir->data.ftLastWriteTime);
    dirent->is_folder = (dir->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    dirent->is_hidden = dir->data.cFileName[0] == L'.' 
      || (dir->data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0
      || (dir->data.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) != 0;

    return true;
}

int MT_AppendFile(d_Vector(char)* out, MT_Directory* dir, d_Slice(char) name)
{ return dv_append_file_win32(out, dir ? dir->wname : W(""), name); }

bool MT_IsDirectory(d_Slice(char) name)
{
    d_Vector(wchar) wname = DV_INIT;
    DWORD attr;

    dv_append(&wname, W("\\\\?\\"));
    dv_append_from_utf8(&wname, name);
    attr = GetFileAttributes(wname.data);
    dv_free(wname);

    return attr != INVALID_FILE_ATTRIBUTES
        && (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

#elif defined(DMEM_HAVE_OPENAT)
/* -------------------------------------------------------------------------- */

MT_Directory* MT_OpenDirectory2(MT_Directory* dir, d_Slice(char) relative)
{
    /* Need to take a copy of relative to ensure its null terminated */
    d_Vector(char) relativecopy = DV_INIT;
    DIR* newdir = NULL;

    dv_set(&relativecopy, relative);

    if (dir) {
        int newfd = openat(dirfd((DIR*) dir), relativecopy.data, O_DIRECTORY);
        if (newfd >= 0) {
            newdir = fdopendir(newfd);
        }
    } else {
        newdir = opendir(relativecopy.data);
    }

    dv_free(relativecopy);
    return (MT_Directory*) newdir;
}

void MT_CloseDirectory(MT_Directory* dir)
{ if (dir) closedir((DIR*) dir); }

bool MT_HaveNextDirEntry(MT_Directory* dir, MT_DirEntry* dirent)
{
    struct stat st;
    struct dirent* e;
    int dfd;

    if (dir == NULL) {
        return false;
    }

    dfd = dirfd((DIR*) dir);

    for (;;) {
        e = readdir((DIR*) dir);
        if (e == NULL) {
            return false;
        }

        dirent->name = dv_char(e->d_name);

        if (!fstatat(dfd, e->d_name, &st, 0)) {
            break;
        }
    }

    dirent->is_folder = S_ISDIR(st.st_mode);
    dirent->size = st.st_size;
    dirent->modify_time = st.st_mtime;
    dirent->is_hidden = (e->d_name[0] == '.');
    return true;
}

int MT_AppendFile(d_Vector(char)* out, MT_Directory* dir, d_Slice(char) name)
{ return dv_append_file_openat(out, dir ? dirfd((DIR*) dir) : -1, name); }

bool MT_IsDirectory(d_Slice(char) name)
{
    struct stat st;
    d_Vector(char) namecopy = DV_INIT;
    int ret;

    dv_set(&namecopy, name);
    ret = stat(namecopy.data, &st);
    dv_free(namecopy);

    return !ret && S_ISDIR(st.st_mode);
}

#else
/* -------------------------------------------------------------------------- */

struct MT_Directory {
    d_Vector(char) folder;
    DIR* dir;
};

MT_Directory* MT_OpenDirectory2(MT_Directory* dir, d_Slice(char) relative)
{
    MT_Directory* ret = NEW(MT_Directory);

    if (dir && dir->folder.size && (relative.size == 0 || relative.data[0] != '/')) {
        dv_append(&ret->folder, dir->folder);
        dv_append(&ret->folder, C("/"));
        dv_append(&ret->folder, relative);
    } else {
        dv_set(&ret->folder, relative);
    }

    ret->dir = opendir(ret->folder.data);

    if (ret->dir) {
        return ret;
    } else {
        MT_CloseDirectory(ret);
        return NULL;
    }
}

void MT_CloseDirectory(MT_Directory* dir)
{ 
    if (dir) {
        if (dir->dir) {
            closedir(dir->dir);
        }
        dv_free(dir->folder);
        free(dir);
    }
}

bool MT_HaveNextDirEntry(MT_Directory* dir, MT_DirEntry* dirent)
{
    struct stat st;
    struct dirent* e;
    int ret = -1;

    if (dir == NULL) {
        return false;
    }

    while (ret != 0) {
        e = readdir(dir->dir);
        if (e == NULL) {
            return false;
        }

        dirent->name = dv_char(e->d_name);
        dv_append(&dir->folder, dirent->name);
        ret = stat(dir->folder.data, &st);
        dv_erase_end(&dir->folder, dirent->name.size);
    }

    dirent->is_folder = S_ISDIR(st.st_mode);
    dirent->size = st.st_size;
    dirent->modify_time = st.st_mtime;
    dirent->is_hidden = (e->d_name[0] == '.');
    return true;
}

int MT_AppendFile(d_Vector(char)* out, MT_Directory* dir, d_Slice(char) name)
{ return dv_append_file_posix(out, dir ? dir->folder : C(""), name); }

bool MT_IsDirectory(d_Slice(char) name)
{
    struct stat st;
    d_Vector(char) namecopy = DV_INIT;
    int ret;

    dv_set(&namecopy, name);
    ret = stat(namecopy.data, &st);
    dv_free(namecopy);

    return !ret && S_ISDIR(st.st_mode);
}

#endif
