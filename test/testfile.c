/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2001 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Apache" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "apr_file_io.h"
#include "apr_file_info.h"
#include "apr_network_io.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "test_apr.h"

struct view_fileinfo
{
    apr_int32_t bits;
    char *description;
} vfi[] = {
    {APR_FINFO_MTIME,  "MTIME"},
    {APR_FINFO_CTIME,  "CTIME"},
    {APR_FINFO_ATIME,  "ATIME"},
    {APR_FINFO_SIZE,   "SIZE"},
    {APR_FINFO_DEV,    "DEV"},
    {APR_FINFO_INODE,  "INODE"},
    {APR_FINFO_NLINK,  "NLINK"},
    {APR_FINFO_TYPE,   "TYPE"},
    {APR_FINFO_USER,   "USER"}, 
    {APR_FINFO_GROUP,  "GROUP"}, 
    {APR_FINFO_UPROT,  "UPROT"}, 
    {APR_FINFO_GPROT,  "GPROT"},
    {APR_FINFO_WPROT,  "WPROT"},
    {0,                NULL}
}; 

int test_filedel(apr_pool_t *);
int testdirs(apr_pool_t *);
static void test_read(apr_pool_t *);

static void closeapr(void)
{
    apr_terminate();
}

int main(void)
{
    apr_pool_t *pool;
    apr_file_t *thefile = NULL;
    apr_finfo_t finfo;
    apr_socket_t *testsock = NULL;
    apr_pollfd_t *sdset = NULL;
    apr_status_t status;
    apr_int32_t flag = APR_READ | APR_WRITE | APR_CREATE;
    apr_size_t nbytes = 0;
    apr_off_t zer = 0;
    char *buf;
    const char *str;
    char *filename = "test.fil";
    char *teststr;
    apr_uid_t uid;
    apr_gid_t gid;
#if APR_FILES_AS_SOCKETS
    apr_int32_t num;
#endif

    printf("APR File Functions Test\n=======================\n\n");
    
    STD_TEST_NEQ("Initializing APR", apr_initialize())
    atexit(closeapr);
    STD_TEST_NEQ("Creating the main pool we'll use", 
                 apr_pool_create(&pool, NULL))
    STD_TEST_NEQ("Creating the second pool we'll use", 
                 apr_pool_create(&pool, NULL))
    

    fprintf(stdout, "Testing file functions.\n");

    STD_TEST_NEQ("    Opening file", 
                 apr_file_open(&thefile, filename, flag, 
                               APR_UREAD | APR_UWRITE | APR_GREAD, pool))
    
    printf("%-60s", "    Checking filename");
    if (thefile == NULL){
        MSG_AND_EXIT("\nBad file descriptor")
    }
    apr_file_name_get(&str, thefile);
    printf("%s\n", str);
    if (strcmp(str, filename) != 0){
        MSG_AND_EXIT("Wrong filename\n")
    }
    
    printf("%-60s", "    Writing to file");
    
    nbytes = strlen("this is a test");
    status = apr_file_write(thefile, "this is a test", &nbytes);
    if (status != APR_SUCCESS) {
        printf("Failed\n");
        PRINT_ERROR(status);
    }
    if (nbytes != strlen("this is a test")) {
        printf("Failed\n");
        MSG_AND_EXIT("Failed to write correctly.");
    }
    else {
        printf("OK\n");
    }

    zer = 0;
    STD_TEST_NEQ("    Moving to the start of file", 
                 apr_file_seek(thefile, SEEK_SET, &zer))
                 
#if APR_FILES_AS_SOCKETS
    printf("    This platform supports files_like_sockets, testing...\n");
    STD_TEST_NEQ("        Making file look like a socket",
                 apr_socket_from_file(&testsock, thefile))

    apr_poll_setup(&sdset, 1, pool);
    apr_poll_socket_add(sdset, testsock, APR_POLLIN);
    num = 1;
    STD_TEST_NEQ("        Checking for incoming data",
                 apr_poll(sdset, &num, -1))
    if (num == 0) {
        MSG_AND_EXIT("I should not return until num == 1\n")
    }
    printf("    End of files as sockets test.\n");
#endif

    nbytes = strlen("this is a test");
    buf = (char *)apr_palloc(pool, nbytes + 1);
    STD_TEST_NEQ("    Reading from the file",
                 apr_file_read(thefile, buf, &nbytes))
    if (nbytes != strlen("this is a test")) {
        MSG_AND_EXIT("We didn't read properly.\n");
    }
    
    STD_TEST_NEQ("    Adding user data to the file",
                 apr_file_data_set(thefile, "This is a test",
                                   "test", apr_pool_cleanup_null))

    STD_TEST_NEQ("    Getting user data from the file",
                 apr_file_data_get((void **)&teststr, "test", thefile))
                 
    if (strcmp(teststr, "This is a test")) {
        MSG_AND_EXIT("Got the data, but it was wrong");
    }

    printf("%-60s", "    Getting fileinfo");
    status = apr_file_info_get(&finfo, APR_FINFO_NORM, thefile);
    if (status  == APR_INCOMPLETE) {
	int i;
        printf("INCOMPLETE\n");
        for (i = 0; vfi[i].bits; ++i)
            if (vfi[i].bits & ~finfo.valid)
                fprintf(stderr, "\t    Missing %s\n", vfi[i].description);
    }
    else if (status != APR_SUCCESS) {
        printf("OK\n");
        MSG_AND_EXIT("Couldn't get the fileinfo")
    }
    else {
        printf("OK\n");
    }
    gid = finfo.group;
    uid = finfo.user;

    STD_TEST_NEQ("    Closing the file", apr_file_close(thefile))

    printf("%-60s", "    Stat'ing file");
    status = apr_stat(&finfo, filename, APR_FINFO_NORM, pool);
    if (status  == APR_INCOMPLETE) {
	int i;
        printf("INCOMPLETE\n");
        for (i = 0; vfi[i].bits; ++i)
            if (vfi[i].bits & ~finfo.valid)
                fprintf(stderr, "\t    Missing %s\n", vfi[i].description);
    }
    else if (status  != APR_SUCCESS) {
        printf("Failed\n");
        MSG_AND_EXIT("Couldn't stat the file")
    }
    else {
        printf("OK\n");
    }    

    if (finfo.valid & APR_FINFO_GROUP) {
        STD_TEST_NEQ("    Getting groupname", 
                     apr_get_groupname(&buf, finfo.group, pool))
        STD_TEST_NEQ("    Comparing group ID's",
                     apr_compare_groups(finfo.group, gid))
        printf("     (gid's for %s match)\n", buf);
    }

    if (finfo.valid & APR_FINFO_USER) {
        STD_TEST_NEQ("    Getting username", 
                     apr_get_username(&buf, finfo.user, pool))
        STD_TEST_NEQ("    Comparing users",
                     apr_compare_users(finfo.user, uid))
        printf("     (uid's for %s match)\n", buf);
    }

    STD_TEST_NEQ("    Deleting the file", apr_file_remove(filename, pool))
    TEST_EQ("    Making sure it's gone",
           apr_file_open(&thefile, filename, APR_READ, 
                         APR_UREAD | APR_UWRITE | APR_GREAD, pool),
           APR_SUCCESS, "OK", "Failed")

    testdirs(pool); 
    test_filedel(pool);
    test_read(pool);

    apr_pool_destroy(pool);
    return 1;
}

int test_filedel(apr_pool_t *pool)
{
    apr_file_t *thefile = NULL;
    apr_int32_t flag = APR_READ | APR_WRITE | APR_CREATE;
    apr_status_t stat;
  
    stat = apr_file_open(&thefile, "testdel", flag, APR_UREAD | APR_UWRITE | APR_GREAD, pool);
    if (stat != APR_SUCCESS) {
         return stat;
    }

    if ((stat = apr_file_close(thefile))  != APR_SUCCESS) {
        return stat;
    }

    if ((stat = apr_file_remove("testdel", pool))  != APR_SUCCESS) {
        return stat;
    }

    stat = apr_file_open(&thefile, "testdel", APR_READ, APR_UREAD | APR_UWRITE | APR_GREAD, pool);
    if (stat == APR_SUCCESS) {
        return stat;
    }
  
    return APR_SUCCESS;
}

int testdirs(apr_pool_t *pool)
{
    apr_dir_t *temp;  
    apr_file_t *file = NULL;
    apr_size_t bytes;
    apr_finfo_t dirent;

    fprintf(stdout, "Testing Directory functions.\n");

    fprintf(stdout, "\tMakeing Directory.......");
    if (apr_dir_make("testdir", APR_UREAD | APR_UWRITE | APR_UEXECUTE | APR_GREAD | APR_GWRITE | APR_GEXECUTE | APR_WREAD | APR_WWRITE | APR_WEXECUTE, pool)  != APR_SUCCESS) {
        fprintf(stderr, "Could not create directory\n");
        return -1;
    }
    else {
        fprintf(stdout, "OK\n");
    }

    if (apr_file_open(&file, "testdir/testfile", APR_READ | APR_WRITE | APR_CREATE, APR_UREAD | APR_UWRITE | APR_UEXECUTE, pool) != APR_SUCCESS) {;
        return -1;
    }

    bytes = strlen("Another test!!!");
    apr_file_write(file, "Another test!!", &bytes); 
	apr_file_close(file);

    fprintf(stdout, "\tOpening Directory.......");
    if (apr_dir_open(&temp, "testdir", pool) != APR_SUCCESS) {
        fprintf(stderr, "Could not open directory\n");
        return -1;
    }
    else {
        fprintf(stdout, "OK\n");
    }

    fprintf(stdout, "\tReading Directory.......");
    if ((apr_dir_read(&dirent, APR_FINFO_DIRENT, temp))  != APR_SUCCESS) {
        fprintf(stderr, "Could not read directory\n");
        return -1;
    }
    else {
        fprintf(stdout, "OK\n");
    }
   
    fprintf(stdout, "\tGetting Information about the file.......\n");
    fprintf(stdout, "\t\tFile name.......");
    do {
        /* Because I want the file I created, I am skipping the "." and ".."
         * files that are here. 
         */
        if (apr_dir_read(&dirent, APR_FINFO_DIRENT | APR_FINFO_TYPE
                                | APR_FINFO_SIZE | APR_FINFO_MTIME, temp) 
                != APR_SUCCESS) {
            fprintf(stderr, "Error reading directory testdir"); 
            return -1;
        }
    } while (dirent.name[0] == '.');
    if (strcmp(dirent.name, "testfile")) {
        fprintf(stderr, "Got wrong file name %s\n", dirent.name);
        return -1;
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "\t\tFile type.......");
    if (dirent.filetype != APR_REG) {
        fprintf(stderr, "Got wrong file type\n");
        return -1;
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "\t\tFile size.......");
    if (dirent.size != bytes) {
        fprintf(stderr, "Got wrong file size %" APR_OFF_T_FMT "\n", dirent.size);
        return -1;
    }
    fprintf(stdout, "OK\n");
     
    fprintf(stdout, "\tRewinding directory.......");
    apr_dir_rewind(temp); 
    fprintf(stdout, "OK\n");
    
    fprintf(stdout, "\tClosing Directory.......");
    if (apr_dir_close(temp)  != APR_SUCCESS) {
        fprintf(stderr, "Could not close directory\n");
        return -1;
    }
    else {
        fprintf(stdout, "OK\n");
    }

    fprintf(stdout, "\tRemoving file from directory.......");
    if (apr_file_remove("testdir/testfile", pool)  != APR_SUCCESS) {
        fprintf(stderr, "Could not remove file\n");
        return -1;
    }
    else {
        fprintf(stdout, "OK\n");
    }

    fprintf(stdout, "\tRemoving Directory.......");
    if (apr_dir_remove("testdir", pool)  != APR_SUCCESS) {
        fprintf(stderr, "Could not remove directory\n");
        return -1;
    }
    else {
        fprintf(stdout, "OK\n");
    }
    
    return 1; 
}    

#define TESTREAD_BLKSIZE 1024
#define APR_BUFFERSIZE   4096 /* This should match APR's buffer size. */

static void create_testread(apr_pool_t *p, const char *fname)
{
    apr_file_t *f = NULL;
    apr_status_t rv;
    char buf[TESTREAD_BLKSIZE];
    apr_size_t nbytes;

    /* Create a test file with known content.
     */
    rv = apr_file_open(&f, fname, APR_CREATE | APR_WRITE | APR_TRUNCATE, APR_UREAD | APR_UWRITE, p);
    if (rv) {
        fprintf(stderr, "apr_file_open()->%d/%s\n",
                rv, apr_strerror(rv, buf, sizeof buf));
        exit(1);
    }
    nbytes = 4;
    rv = apr_file_write(f, "abc\n", &nbytes);
    assert(!rv && nbytes == 4);
    memset(buf, 'a', sizeof buf);
    nbytes = sizeof buf;
    rv = apr_file_write(f, buf, &nbytes);
    assert(!rv && nbytes == sizeof buf);
    nbytes = 2;
    rv = apr_file_write(f, "\n\n", &nbytes);
    assert(!rv && nbytes == 2);
    rv = apr_file_close(f);
    assert(!rv);
}

static char read_one(apr_file_t *f, int expected)
{
  char bytes[3];
  apr_status_t rv;
  static int counter = 0;
  apr_size_t nbytes;

  counter += 1;

  bytes[0] = bytes[2] = 0x01;
  if (counter % 2) {
      rv = apr_file_getc(bytes + 1, f);
  }
  else {
      nbytes = 1;
      rv = apr_file_read(f, bytes + 1, &nbytes);
      assert(nbytes == 1);
  }
  assert(!rv);
  assert(bytes[0] == 0x01 && bytes[2] == 0x01);
  if (expected != -1) {
      assert(bytes[1] == expected);
  }
  return bytes[1];
}

static void test_read_guts(apr_pool_t *p, const char *fname, apr_int32_t extra_flags)
{
    apr_file_t *f = NULL;
    apr_status_t rv;
    apr_size_t nbytes;
    char buf[1024];
    int i;

    rv = apr_file_open(&f, fname, APR_READ | extra_flags, 0, p);
    assert(!rv);
    read_one(f, 'a');
    read_one(f, 'b');
    if (extra_flags & APR_BUFFERED) {
        fprintf(stdout, 
                "\n\t\tskipping apr_file_ungetc() for APR_BUFFERED... "
                "doesn't work yet...\n\t\t\t\t ");
    }
    else {
        rv = apr_file_ungetc('b', f);
        assert(!rv);
        /* Note: some implementations move the file ptr back;
         *       others just save up to one char; it isn't 
         *       portable to unget more than once.
         */
        /* Don't do this: rv = apr_file_ungetc('a', f); */
        read_one(f, 'b');
    }
    read_one(f, 'c');
    read_one(f, '\n');
    for (i = 0; i < TESTREAD_BLKSIZE; i++) {
        read_one(f, 'a');
    }
    read_one(f, '\n');
    read_one(f, '\n');
    rv = apr_file_getc(buf, f);
    assert(rv == APR_EOF);
    rv = apr_file_close(f);
    assert(!rv);

    f = NULL;
    rv = apr_file_open(&f, fname, APR_READ | extra_flags, 0, p);
    assert(!rv);
    rv = apr_file_gets(buf, 10, f);
    assert(!rv);
    assert(!strcmp(buf, "abc\n"));
    /* read first 800 of TESTREAD_BLKSIZE 'a's 
     */
    rv = apr_file_gets(buf, 801, f);
    assert(!rv);
    assert(strlen(buf) == 800);
    for (i = 0; i < 800; i++) {
        assert(buf[i] == 'a');
    }
    /* read rest of the 'a's and the first newline
     */
    rv = apr_file_gets(buf, sizeof buf, f);
    assert(!rv);
    assert(strlen(buf) == TESTREAD_BLKSIZE - 800 + 1);
    for (i = 0; i < TESTREAD_BLKSIZE - 800; i++) {
        assert(buf[i] == 'a');
    }
    assert(buf[TESTREAD_BLKSIZE - 800] == '\n');
    /* read the last newline
     */
    rv = apr_file_gets(buf, sizeof buf, f);
    assert(!rv);
    assert(!strcmp(buf, "\n"));
    /* get APR_EOF
     */
    rv = apr_file_gets(buf, sizeof buf, f);
    assert(rv == APR_EOF);
    /* get APR_EOF with apr_file_getc
     */
    rv = apr_file_getc(buf, f);
    assert(rv == APR_EOF);
    /* get APR_EOF with apr_file_read
     */
    nbytes = sizeof buf;
    rv = apr_file_read(f, buf, &nbytes);
    assert(rv == APR_EOF);
    rv = apr_file_close(f);
    assert(!rv);
}

static void test_bigread(apr_pool_t *p, const char *fname, apr_int32_t extra_flags)
{
    apr_file_t *f = NULL;
    apr_status_t rv;
    char buf[APR_BUFFERSIZE * 2];
    apr_size_t nbytes;

    /* Create a test file with known content.
     */
    rv = apr_file_open(&f, fname, APR_CREATE | APR_WRITE | APR_TRUNCATE, APR_UREAD | APR_UWRITE, p);
    if (rv) {
        fprintf(stderr, "apr_file_open()->%d/%s\n",
                rv, apr_strerror(rv, buf, sizeof buf));
        exit(1);
    }
    nbytes = APR_BUFFERSIZE;
    memset(buf, 0xFE, nbytes);
    rv = apr_file_write(f, buf, &nbytes);
    assert(!rv && nbytes == APR_BUFFERSIZE);
    rv = apr_file_close(f);
    assert(!rv);

    f = NULL;
    rv = apr_file_open(&f, fname, APR_READ | extra_flags, 0, p);
    assert(!rv);
    nbytes = sizeof buf;
    rv = apr_file_read(f, buf, &nbytes);
    assert(!rv);
    assert(nbytes == APR_BUFFERSIZE);
    rv = apr_file_close(f);
    assert(!rv);
}

static void test_read(apr_pool_t *p)
{
    const char *fname = "testread.dat";
    apr_status_t rv;

    fprintf(stdout, "Testing file read functions.\n");

    create_testread(p, fname);
    fprintf(stdout, "\tBuffered file tests......");
    test_read_guts(p, fname, APR_BUFFERED);
    fprintf(stdout, "OK\n");
    fprintf(stdout, "\tUnbuffered file tests....");
    test_read_guts(p, fname, 0);
    fprintf(stdout, "OK\n");
    fprintf(stdout, "\tMore buffered file tests......");
    test_bigread(p, fname, APR_BUFFERED);
    fprintf(stdout, "OK\n");
    fprintf(stdout, "\tMore unbuffered file tests......");
    test_bigread(p, fname, 0);
    fprintf(stdout, "OK\n");
    rv = apr_file_remove(fname, p);
    assert(!rv);
    fprintf(stdout, "\tAll read tests...........OK\n");
}

