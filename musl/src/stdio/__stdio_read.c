#include "stdio_impl.h"
#include <sys/uio.h>
#include <carbon/public/status.h>

size_t __stdio_read(FILE *f, unsigned char *buf, size_t len)
{
	struct iovec iov[2] = {
		{ .iov_base = buf, .iov_len = len - !!f->buf_size },
		{ .iov_base = f->buf, .iov_len = f->buf_size }
	};
	ssize_t cnt;

	cbn_status_t st;
	if(iov[0].iov_len)
	{
		st = syscall(SYS_cbn_readv, f->fd, iov, 2, &cnt);
	}
	else
		st = syscall(SYS_cbn_read, f->fd, iov[1].iov_base, iov[1].iov_len, &cnt);

	if (st != CBN_STATUS_OK)
		f->flags |= F_ERR;
	else if(cnt == 0) {
		f->flags |= F_EOF;
		return 0;
	}
	if (cnt <= iov[0].iov_len) return cnt;
	cnt -= iov[0].iov_len;
	f->rpos = f->buf;
	f->rend = f->buf + cnt;
	if (f->buf_size) buf[len-1] = *f->rpos++;
	return len;
}
