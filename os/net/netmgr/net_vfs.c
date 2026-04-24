/****************************************************************************
 *
 * Copyright 2019 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/

#include <tinyara/config.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/sockio.h>
#include <errno.h>
#include <debug.h>
#include <net/if.h>
#include <tinyara/net/net.h>
#include "netstack.h"
#include <tinyara/net/netlog.h>

#define TAG "[NETMGR]"
/****************************************************************************
 * Name: net_checksd
 *
 * Description:
 *   Dispatch socket-descriptor validation to the active network stack
 *   selected by the descriptor range.  The wrapper returns backend
 *   results directly and falls back to -1 when no stack or check hook is
 *   available.
 *
 ****************************************************************************/
int net_checksd(int sd, int oflags)
{
	NETSTACK_CALL_BYFD(sd, checksd, (sd, oflags));
}

/****************************************************************************
 * Function: net_clone
 *
 * Description:
 *   Clone one socket structure into another. The current bridge does not
 *   dispatch this operation and simply reports that it is unsupported.
 *
 ****************************************************************************/

int net_clone(FAR struct socket *sock1, FAR struct socket *sock2)
{
	NET_LOGKV(TAG, "Not supported yet\n");
	return -1;
}

/****************************************************************************
 * Function: net_dupsd
 *
 * Description:
 *   Dispatch socket descriptor duplication to the active network stack
 *   selected by get_netstack_byfd(sd).
 *
 ****************************************************************************/
int net_dupsd(int sd)
{
	NETSTACK_CALL_BYFD(sd, dup, (sd));
}

/****************************************************************************
 * Function: net_dupsd2
 *
 * Description:
 *   Dispatch socket descriptor replacement to the active network stack
 *   selected by get_netstack_byfd(sd1).
 *
 ****************************************************************************/

int net_dupsd2(int sd1, int sd2)
{
	NETSTACK_CALL_BYFD(sd1, dup2, (sd1, sd2));
}

/****************************************************************************
 * Function: net_close
 *
 * Description:
 *   Dispatch socket close to the active network stack selected by
 *   get_netstack_byfd(sd).
 *
 * Parameters:
 *   sd   Socket descriptor of socket
 *
 * Returned Value:
 *   Returns the backend close result directly. If no stack or close hook is
 *   available for the descriptor, the bridge falls back to -1.
 *
 * Assumptions:
 *
 ****************************************************************************/

int net_close(int sd)
{
	NETSTACK_CALL_BYFD(sd, close, (sd));
}

/****************************************************************************
 * Function: net_poll
 *
 * Description:
 *   Dispatch poll setup and teardown to the active network stack selected by
 *   get_netstack_byfd(sd). The backend owns the actual wait and returns the
 *   poll result directly.
 *
 * Returned Value:
 *   Returns the backend poll result directly, including backend-specific
 *   failure conventions such as lwIP's negated errno values. If no stack or
 *   poll hook is available for the descriptor, the bridge falls back to -1.
 *
 * Assumptions:
 *
 ****************************************************************************/
int net_poll(int sd, struct pollfd *fds, bool setup)
{
	NETSTACK_CALL_BYFD(sd, poll, (sd, fds, setup));
}

/****************************************************************************
 * Name: net_ioctl
 *
 * Description:
 *   Bridge socket ioctl requests through the active network stack and then
 *   through netdev ioctl fallback handlers when the stack reports `-ENOTTY`.
 *
 * Parameters:
 *   sd   Socket descriptor of device
 *   cmd      The ioctl command
 *   arg      The argument of the ioctl cmd
 *
 * Return:
 *   >=0 on success (positive non-zero values are command-specific). On
 *   failure, -1 is returned with errno set from the final negative result.
 *
 *   EBADF
 *     'sd' is not a valid descriptor.
 *   ENOTTY
 *     'cmd' is outside the accepted `_FIOCVALID()` / `_SIOCVALID()` range or
 *     no handler accepts it.
 *   ENOTTY
 *     'sd' is not associated with a network device.
 *   ENOTTY
 *      The specified request does not apply to the kind of object that the
 *      descriptor 'sd' references.
 *
 ****************************************************************************/
extern int netdev_imsfioctl(FAR struct socket *sock, int cmd, FAR struct ip_msfilter *imsf);
extern int netdev_ifrioctl(FAR struct socket *sock, int cmd, FAR struct ifreq *req);

int net_ioctl(int sd, int cmd, unsigned long arg)
{
	FAR struct lwip_sock *sock = NULL;
	int ret = -ENOTTY;

	/* Validate only the command family here. The current wrapper does not
	 * reject NULL arg values up front; backend handlers decide whether the
	 * argument layout is acceptable for the command.
	 */
	if (!((_FIOCVALID(cmd)) ||  (_SIOCVALID(cmd)))) {
		ret = -ENOTTY;
		NET_LOGKE(TAG, "command is not valid\n");
		goto errout;
	}

	/* Verify that the descriptor corresponds to a valid allocated socket. */
	sock = get_socket_by_pid(sd, getpid());
	if (sock == NULL) {
		NET_LOGKE(TAG, "get socket fail\n");
		ret = -EBADF;
		goto errout;
	}

	/* Execute the stack-specific handler first, then try the generic netdev
	 * ioctl fallbacks if the stack reports -ENOTTY.
	 */
	struct netstack *st = get_netstack_byfd(sd);
	if (st) {
		NETSTACK_CALL_RET(st, ioctl, (sd, cmd, arg), ret);
	}
	if (ret == -ENOTTY) {
		ret = netdev_ifrioctl(NULL, cmd, (FAR struct ifreq *)((uintptr_t)arg));
	}
#ifdef CONFIG_NET_IGMP
	if (ret == -ENOTTY) {
		ret = netdev_imsfioctl(NULL, cmd, (FAR struct ip_msfilter *)((uintptr_t)arg));
	}
#endif							/* CONFIG_NET_IGMP */
#ifdef CONFIG_NET_ROUTE
	if (ret == -ENOTTY) {
		ret = netdev_rtioctl(NULL, cmd, (FAR struct rtentry *)((uintptr_t)arg));
	}
#endif							/* CONFIG_NET_ROUTE */
	/* Check for success or failure */
	if (ret >= 0) {
		return ret;
	}

	/* On failure, set the errno and return -1 */
errout:
	errno = -ret;
	return ERROR;
}


/****************************************************************************
 * Name: net_vfcntl
 *
 * Description:
 *   Perform a limited fcntl bridge on a socket descriptor. Only a subset of
 *   commands is handled here, and the supported ones either dispatch to the
 *   active netstack or fail locally.
 *
 * Input Parameters:
 *   sd - Socket descriptor of the socket to operate on
 *   cmd    - The fcntl command.
 *   ap     - Command-specific arguments
 *
 * Returned Value:
 *   Returns backend results directly for dispatched commands. Unsupported
 *   commands fail with errno set by the wrapper. The current implementation
 *   also has command-specific fallback quirks when no stack or hook is
 *   present.
 *
 ****************************************************************************/

int net_vfcntl(int sd, int cmd, va_list ap)
{

	FAR struct socket *sock = (struct socket *)get_socket_by_pid(sd, getpid());
	int err = 0;
	int ret = 0;

	NET_LOGKV(TAG, "sd=%d cmd=%d\n", sd, cmd);

	/* Verify that the sd corresponds to valid, allocated socket */

	if (!sock) {
		err = EBADF;
		NET_LOGKE(TAG, "invalid socket\n");
		goto errout;
	}

	/* Interrupts must be disabled in order to perform operations on socket structures */

	struct netstack *st = get_netstack_byfd(sd);
	// net_lock();
	switch (cmd) {
	case F_DUPFD:
		/* Return a new file descriptor which shall be the lowest numbered
		 * available (that is, not already open) file descriptor greater than
		 * or equal to the third argument, arg, taken as an integer of type
		 * int. The new file descriptor shall refer to the same open file
		 * description as the original file descriptor, and shall share any
		 * locks.  The FD_CLOEXEC flag associated  with the new file descriptor
		 * shall be cleared to keep the file open across calls to one of the
		 * exec functions.
		 */

	{
		/* The current bridge ignores the third argument and forwards only
		 * the source descriptor to the backend dup hook.
		 */
		NETSTACK_CALL_RET(st, dup, (sd), ret);
	}
	break;

	case F_GETFD:
		/* Get the file descriptor flags defined in <fcntl.h> that are associated
		 * with the file descriptor fd.  File descriptor flags are associated
		 * with a single file descriptor and do not affect other file descriptors
		 * that refer to the same file.
		 */

	case F_SETFD:
		/* Set the file descriptor flags defined in <fcntl.h>, that are associated
		 * with fd, to the third argument, arg, taken as type int. If the
		 * FD_CLOEXEC flag in the third argument is 0, the file shall remain open
		 * across the exec functions; otherwise, the file shall be closed upon
		 * successful execution of one  of  the  exec  functions.
		 */

		err = ENOSYS;			/* F_GETFD and F_SETFD not implemented */
		break;

	case F_GETFL:
		/* Get the file status flags and file access modes, defined in
		 * <fcntl.h>, for the file description associated with fd. The file
		 * access modes can be extracted from the return value using the
		 * mask O_ACCMODE, which is defined  in <fcntl.h>. File status flags
		 * and file access modes are associated with the file description
		 * and do not affect other file descriptors that refer to the same
		 * file with different open file descriptions.
		 */

	{
		NETSTACK_CALL_RET(st, fcntl, (sd, cmd, ap), ret);
	}
	break;

	case F_SETFL:
		/* Set the file status flags, defined in <fcntl.h>, for the file description
		 * associated with fd from the corresponding  bits in the third argument,
		 * arg, taken as type int. Bits corresponding to the file access mode and
		 * the file creation flags, as defined in <fcntl.h>, that are set in arg shall
		 * be ignored. If any bits in arg other than those mentioned here are changed
		 * by the application, the result is unspecified.
		 */

	{
		NETSTACK_CALL_RET(st, fcntl, (sd, cmd, ap), ret);
	}

	break;

	case F_GETOWN:
		/* If fd refers to a socket, get the process or process group ID specified
		 * to receive SIGURG signals when out-of-band data is available. Positive values
		 * indicate a process ID; negative values, other than -1, indicate a process group
		 * ID. If fd does not refer to a socket, the results are unspecified.
		 */

	case F_SETOWN:
		/* If fd refers to a socket, set the process or process group ID specified
		 * to receive SIGURG signals when out-of-band data is available, using the value
		 * of the third argument, arg, taken as type int. Positive values indicate a
		 * process ID; negative values, other than -1, indicate a process group ID. If
		 * fd does not refer to a socket, the results are unspecified.
		 */

	case F_GETLK:
		/* Get the first lock which blocks the lock description pointed to by the third
		 * argument, arg, taken as a pointer to type struct flock, defined in <fcntl.h>.
		 * The information retrieved shall overwrite the information passed to fcntl() in
		 * the structure flock. If no lock is found that would prevent this lock from being
		 * created, then the structure shall be left unchanged except for the lock type
		 * which shall be set to F_UNLCK.
		 */

	case F_SETLK:
		/* Set or clear a file segment lock according to the lock description pointed to
		 * by the third argument, arg, taken as a pointer to type struct flock, defined in
		 * <fcntl.h>. F_SETLK can establish shared (or read) locks (F_RDLCK) or exclusive
		 * (or write) locks (F_WRLCK), as well  as  to  remove  either  type  of  lock  (F_UNLCK).
		 * F_RDLCK, F_WRLCK, and F_UNLCK are defined in <fcntl.h>. If a shared or exclusive
		 * lock cannot be set, fcntl() shall return immediately with a return value of -1.
		 */

	case F_SETLKW:
		/* This command shall be equivalent to F_SETLK except that if a shared or exclusive
		 * lock is blocked by other locks, the thread shall wait until the request can be
		 * satisfied. If a signal that is to be caught is received while fcntl() is waiting
		 * for a region, fcntl() shall be interrupted. Upon return from the signal handler,
		 * fcntl() shall return -1 with errno set to [EINTR], and the lock operation shall
		 * not be done.
		 */

		err = ENOSYS;			/* F_GETOWN, F_SETOWN, F_GETLK, F_SETLK, F_SETLKW */
		break;

	default:
		err = EINVAL;
		break;
	}

	// net_unlock();

errout:
	if (err != 0) {
		errno = err;
		return ERROR;
	}
	return ret;
}


/****************************************************************************
 * Name: net_initlist
 *
 * Description:
 *   Initialize a list of sockets for a new task.  The wrapper forwards the
 *   request to the default socket netstack when that backend exposes an
 *   initlist hook.
 *
 * Input Parameters:
 *   list -- A reference to the pre-allocated socket list to be initialized.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void net_initlist(FAR struct socketlist *list)
{
	struct netstack *stk = get_netstack(TR_SOCKET);
	if (stk) {
		stk->ops->initlist(list);
	}
}

/****************************************************************************
 * Name: net_releaselist
 *
 * Description:
 *   Release resources held by the socket list.  The wrapper first lets the
 *   default socket netstack release backend-owned socket state, then
 *   destroys the list semaphore.
 *
 * Input Parameters:
 *   list -- A reference to the pre-allocated socket list to be un-initialized.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void net_releaselist(FAR struct socketlist *list)
{
	DEBUGASSERT(list);
	struct netstack *stk = get_netstack(TR_SOCKET);
	if (stk) {
		stk->ops->releaselist(list);
	}

	/* Destroy the semaphore */
	sem_destroy(&list->sl_sem);
}
