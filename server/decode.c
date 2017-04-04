#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "decode.h"
#include "exit.h"
#include "trinity.h"
#include "types.h"
#include "udp.h"
#include "utils.h"

static void decode_main_started(void)
{
	struct msg_mainstarted *mainmsg;

	mainmsg = (struct msg_mainstarted *) &buf;
	printf("Main started. pid:%d number of children: %d. shm:%p-%p\n",
		mainmsg->hdr.pid, mainmsg->num_children, mainmsg->shm_begin, mainmsg->shm_end);
}

static void decode_main_exiting(void)
{
	struct msg_mainexiting *mainmsg;

	mainmsg = (struct msg_mainexiting *) &buf;
	printf("Main exiting. pid:%d Reason: %s\n", mainmsg->hdr.pid, decode_exit(mainmsg->reason));
}

static void decode_child_spawned(void)
{
	struct msg_childspawned *childmsg;

	childmsg = (struct msg_childspawned *) &buf;
	printf("Child spawned. id:%d pid:%d\n", childmsg->childno, childmsg->hdr.pid);
}

static void decode_child_exited(void)
{
	struct msg_childexited *childmsg;

	childmsg = (struct msg_childexited *) &buf;
	printf("Child exited. id:%d pid:%d\n", childmsg->childno, childmsg->hdr.pid);
}

static void decode_child_signalled(void)
{
	struct msg_childsignalled *childmsg;

	childmsg = (struct msg_childsignalled *) &buf;
	printf("Child signal. id:%d pid:%d signal: %s\n",
		childmsg->childno, childmsg->hdr.pid, strsignal(childmsg->sig));
}

static void decode_obj_created_file(void)
{
	struct msg_objcreatedfile *objmsg;

	objmsg = (struct msg_objcreatedfile *) &buf;

	if (objmsg->fopened) {
		printf("%s object created at %p by pid %d: fd %d = fopen(\"%s\") ; fcntl(fd, 0x%x)\n",
			objmsg->hdr.global ? "local" : "global",
			objmsg->hdr.address, objmsg->hdr.pid,
			objmsg->fd, objmsg->filename,
			objmsg->fcntl_flags);
	} else {
		printf("%s object created at %p by pid %d: fd %d = open(\"%s\", 0x%x)\n",
			objmsg->hdr.global ? "local" : "global",
			objmsg->hdr.address, objmsg->hdr.pid,
			objmsg->fd, objmsg->filename, objmsg->flags);
	}
}

static void decode_obj_created_map(void)
{
	struct msg_objcreatedmap *objmsg;
	const char *maptypes[] = {
		"initial anon mmap",
		"child created anon mmap",
		"mmap'd file",
	};
	objmsg = (struct msg_objcreatedmap *) &buf;

	printf("%s map object created at %p by pid %d: start:%p size:%ld name:%s prot:%x type:%s\n",
		objmsg->hdr.global ? "local" : "global",
		objmsg->hdr.address, objmsg->hdr.pid,
		objmsg->start, objmsg->size, objmsg->name, objmsg->prot, maptypes[objmsg->type - 1]);
}

static void decode_obj_created_pipe(void)
{
	struct msg_objcreatedpipe *objmsg;
	objmsg = (struct msg_objcreatedpipe *) &buf;

	printf("%s pipe object created at %p by pid %d: fd:%d flags:%x [%s]\n",
		objmsg->hdr.global ? "local" : "global",
		objmsg->hdr.address, objmsg->hdr.pid,
		objmsg->fd, objmsg->flags,
		objmsg->reader ? "reader" : "writer");
}

const struct msgfunc decodefuncs[MAX_LOGMSGTYPE] = {
	[MAIN_STARTED] = { decode_main_started },
	[MAIN_EXITING] = { decode_main_exiting },
	[CHILD_SPAWNED] = { decode_child_spawned },
	[CHILD_EXITED] = { decode_child_exited },
	[CHILD_SIGNALLED] = { decode_child_signalled },
	[OBJ_CREATED_FILE] = { decode_obj_created_file },
	[OBJ_CREATED_MAP] = { decode_obj_created_map },
	[OBJ_CREATED_PIPE] = {decode_obj_created_pipe },
};