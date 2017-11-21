/*
 * File-related system call implementations.
 */

#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <kern/limits.h>
#include <kern/seek.h>
#include <kern/stat.h>
#include <lib.h>
#include <uio.h>
#include <proc.h>
#include <current.h>
#include <synch.h>
#include <copyinout.h>
#include <vfs.h>
#include <vnode.h>
#include <openfile.h>
#include <filetable.h>
#include <syscall.h>

/*
 * open() - get the path with copyinstr, then use openfile_open and
 * filetable_place to do the real work.
 */
int
sys_open(const_userptr_t upath, int flags, mode_t mode, int *retval)
{

	const int allflags = O_ACCMODE | O_CREAT | O_EXCL | O_TRUNC | O_APPEND | O_NOCTTY; 
	char *kpath;
	int result = 0;

	/* 
	 * your implementation of system call open starts here.  
	 *
	 * check the design document design/filesyscall.txt for the steps
	 */

	struct openfile * of;
	size_t * actual;
	size_t len;

   	/* check for invalid flags */
	if((allflags | flags) != allflags){
		*retval = -1;
		return EINVAL; // return invalid argument code
	}

	/* get path length for copyinstr */
	len = strlen((char*)upath) + 1;
	
	/* prepare parameters for copyinstr in order to copy a string
	 * from user-level address usersrc to kernel address * dest,
	 * as per copystr. */
	actual = (size_t *)kmalloc(sizeof(size_t));
	kpath = (char *)kmalloc(sizeof(char)*len);

	/* call copyinstr and check for results */
	result = copyinstr(upath, kpath, len, actual);
	if(result != 0){
		//kfree(kpath);
		*retval = -1;
		return result;
	}

	/* allocate space for the file */
	of = (struct openfile *)kmalloc(sizeof(struct openfile));
	
	
	/* open file and check for errors */
	result = openfile_open(kpath, flags, mode, &of);
	if(result != 0){
		//kfree(kpath);
		*retval = -1;
		return result;
	}

	/* increment the reference count on an open file */
	openfile_incref(of);
  	

	/* place the file in a file table and get the file descriptor
	 * using filetable_place and then check for errors. */
	result = filetable_place(curproc->p_filetable, of, retval);
	if(result != 0){
		//kfree(kpath);
		*retval = -1;
		return result;
	}

	kfree(kpath);

	return result;
}

/*
 * read() - read data from a file
 */
int
sys_read(int fd, userptr_t buf, size_t size, int *retval)
{

       /* 
        * your implementation of system call read starts here.  
        *
        * check the design document design/filesyscall.txt for the steps
        */

	int result = 0;   
	int is_seekable;
	struct openfile * of;
	
   	/* lookup fd in filetable and get the file object. */
	result = filetable_get(curproc->p_filetable, fd, &of);
	/* check for errors */
	if(result != 0){
		*retval = -1;
		return result;
	}
	
	
	/* check if this file is seekable. all regular files
 	 * and directories are seekable, but some devices are not. */
	is_seekable = VOP_ISSEEKABLE(of->of_vnode); 

	/* lock the seek position if object is seekable */
	if(is_seekable){
		lock_acquire(of->of_offsetlock);
	}

  	/* check for files opened write-only */
	if(of->of_accmode == O_WRONLY){
		*retval = -1;
		return EFTYPE;
	}

   	/* construct a uio and iovec */
	struct uio myuio;
	struct iovec iov;

	/* initialize a uio suitable for i/o from a kernel buffer. */
	uio_kinit(&iov, &myuio, buf, size, of->of_offset, UIO_READ);
	
	//uio_kinit(&iov, &myuio, buf, size, of->of_offset, UIO_USERSPACE);

   	/* call vop_read and check for results */
	result = VOP_READ(of->of_vnode, &myuio);
	if(result != 0){
		*retval = -1;
		return result;
	}
   
	/* set the return value correctly */
	*retval = myuio.uio_offset - of->of_offset;
	
	/* update the seek position afterwards */
	of->of_offset = myuio.uio_offset;
	
	/* unlock and filetable_put() */
	if(is_seekable){
		lock_release(of->of_offsetlock);
	}
	filetable_put(curproc->p_filetable, fd, of);	
   
	/* note: for simplicify, you do not need to consider
	 * contention from multiple processes.
	 */

       return result;
}

/*
 * write() - write data to a file
 */
int
sys_write(int fd, userptr_t buf, size_t size, int *retval)
{
	int result;   

	struct openfile * of;
	
   	/* lookup fd in filetable and get the file object. */
	result = filetable_get(curproc->p_filetable, fd, &of);
	/* check for errors */
	if(result != 0){
		*retval = -1;
		return result;
	}
	
	/* check if this file is seekable. all regular files
 	 * and directories are seekable, but some devices are not. */
	int is_seekable = VOP_ISSEEKABLE(of->of_vnode); 

	/* lock the seek position if object is seekable */
	if(is_seekable){
		lock_acquire(of->of_offsetlock);
	}

  	/* check for files opened read-only */
	if(of->of_accmode == O_RDONLY){
		*retval = -1;
		return EACCES;
	}

   	/* construct a uio and iovec */
	struct uio myuio;
	struct iovec iov;

	/* initialize a uio suitable for i/o from a kernel buffer. */
	uio_kinit(&iov, &myuio, buf, size, of->of_offset, UIO_WRITE);
	
	//uio_kinit(&iov, &myuio, buf, size, of->of_offset, UIO_USERSPACE);

   	/* call vop_write and check for results */
	result = VOP_WRITE(of->of_vnode, &myuio);
	if(result != 0){
		*retval = -1;
		return result;
	}
   
	/* update the seek position afterwards */
	*retval = myuio.uio_offset - of->of_offset;
	of->of_offset = myuio.uio_offset;
	
	/* unlock and filetable_put() */
	if(is_seekable){
		lock_release(of->of_offsetlock);
	}
	filetable_put(curproc->p_filetable, fd, of);	
   

	/* note: for simplicify, you do not need to consider
	 * contention from multiple processes. */
	
	/* set the return value correctly */
	return 0;

}

/*
 * close() - remove from the file table.
 */
int
sys_close(int fd, int *retval)
{
	struct openfile * of;
	
	//kprintf("fd: %d\n", fd);

	/* validate the fd number (use filetable_okfd) */
	if(!filetable_okfd(curproc->p_filetable, fd)){
		*retval = -1;
		return EBADF;
	}
   	
	/* use filetable_placeat to replace curproc's file
	 * table entry with NULL */
	filetable_placeat(curproc->p_filetable, NULL, fd, &of);

   	/* check if the previous entry in the file table
	 * was also NULL (this means no such file was open) */
	if(of == NULL){
		*retval = -1;
		return ENOENT;
	}
   
	/* decref the open file returned by filetable_placeat */
	openfile_decref(of);

	*retval = 0;
	return 0;
}


/* 
* meld () - combine the content of two files word by word into a new file
*/
int
sys_meld(userptr_t pn1, userptr_t pn2, userptr_t pn3)
{
	const int FILNUM = 3;
	const int READBUFLEN = 4;

	int retval[1];
	*retval = -1;

	int fd[FILNUM];
	int result[FILNUM];
	//int retval[FILNUM];
	struct openfile * pn[FILNUM];
	size_t * actual[FILNUM];
	size_t len[FILNUM];
	char * kpath[FILNUM];

	char readbuf[READBUFLEN];
	
	/* get path length for pn1, pn2, pn3 */
	len[0] = strlen((char*)pn1) + 1;
	len[1] = strlen((char*)pn2) + 1;
	len[2] = strlen((char*)pn3) + 1;
	
	for(int i=0; i<FILNUM; i++) {
		/* malloc space for actual file path */
		actual[i] = (size_t *)kmalloc(sizeof(size_t));
		/* allocate kpaths for pn1, pn2, pn3 */
		kpath[i] = (char *)kmalloc(sizeof(char)*len[i]);
	}

	/* call copyinstr for each path */
	result[0] = copyinstr(pn1, kpath[0], len[0], actual[0]);
	result[1] = copyinstr(pn2, kpath[1], len[1], actual[1]);
	result[2] = copyinstr(pn3, kpath[2], len[2], actual[2]);
	
	/* check for errors from copyinstr */
	if(result[0] != 0 || result[1] != 0 || result[2] != 0){
		//kfree(kpath);
		//kfree(actual);
		return -1;
	}
	
	/* allocate space to open the files */
	for(int i=0; i<FILNUM; i++) {
		pn[i] = (struct openfile *)kmalloc(sizeof(struct openfile));
	}

	/* open the first two files (use openfile_open) for reading */
	result[0] = openfile_open(kpath[0], 0, O_RDONLY, &pn[0]);
	result[1] = openfile_open(kpath[1], 0, O_RDONLY, &pn[1]);

	/* if pn1 or pn2 do not exist, return an error */	
	if(result[0] != 0 || result[1] != 0){
		//kfree(kpath);
		//kfree(actual);
		return ENOENT;
	}

	/* increase the number of references for open files */
	openfile_incref(pn[0]);
	openfile_incref(pn[1]);

	/* open the third file (use openfile_open) for writing */
	result[2] = openfile_open(kpath[2], O_CREAT|O_EXCL, \
		    O_WRONLY, &pn[2]);
	
	/* O_CREATE|O_EXCL are used to throw errors if pn3 already
	 * exists. Otherwise, the file will be created */
	
	/* if pn3 exists, return an error */
	if(result[2] != 0){
		return EEXIST;
	}

	/* increase the number of references for open files */
	openfile_incref(pn[2]);

	/* return if any file is not open'ed correctly */
	if((pn[0]->of_accmode != O_RDONLY) || \
	  (pn[1]->of_accmode != O_RDONLY) || \
	  (pn[2]->of_accmode == O_WRONLY)) {
		return EFTYPE;
	}	

	/* place the files in a file table and get the file
	 * descriptors using filetable_place and then check
	 * for results. */
	for(int i=0; i<FILNUM; i++) {
		result[i] = filetable_place(curproc->p_filetable, pn[i], \
			    &fd[i]);
	}

	/* check for errors from filetable_place */
	if(result[0] != 0) return result[0];
	else if(result[1] != 0) return result[1];
	else if(result[2] != 0) return result[2];

	/* init read buffer to zeros */
	for(int i=0; i<READBUFLEN; i++) {
		readbuf[i] = 0;
	}

	/* AND NOW, FOR THE MELDING PROCESS! */

	/* read until files are empty */
	while(1) {
		/* read 4 bytes from file 1 */
		result[0] = sys_read(fd[0], (userptr_t)readbuf, \
			    READBUFLEN, retval);
		/* check for reading errors */
		if(result[0] != 0) {
			return result[0];
		}
		/* check if EOF */
		if(*retval == 0) break;
		/* write to file 3 */
		result[2] = sys_write(fd[2], (userptr_t)readbuf, \
			    READBUFLEN, NULL);
		/* check for writing errors */
		if(result[2] != 0) {
			return result[2];
		}
		/* read 4 bytes from file 2 */
		result[1] = sys_read(fd[1], (userptr_t)readbuf, \
			    READBUFLEN, retval);
		/* check if EOF */
		if(*retval == 0) break;
		/* check for reading errors */
		if(result[1] != 0) {
			return result[1];
		}
		/* write to file 3 */
		result[2] = sys_write(fd[2], (userptr_t)readbuf, \
			    READBUFLEN, NULL);
		/* check for writing errors */
		if(result[2] != 0) {
			return result[2];
		}		
	}

	/* use sys_close() to complete the use of the three files */
	result[0] = sys_close(fd[0], NULL);
	if(result[0] != 0) {
		return result[0];
	}
	result[1] = sys_close(fd[1], NULL);
	if(result[1] != 0) {
		return result[1];
	}
	result[2] = sys_close(fd[2], NULL);
	if(result[2] != 0) {
		return result[2];
	}
   
	/* deallocate our resources */
	for(int i=0; i<FILNUM; i++) {
		kfree(kpath[i]);
		kfree(actual[i]);
		openfile_decref(pn[i]);
	}

	/* set the return value correctly for success */
	return 0;
}
