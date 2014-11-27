/******************************************************************************

  Copyright (c) 2012, Intel Corporation 
  All rights reserved.
  
  Redistribution and use in source and binary forms, with or without 
  modification, are permitted provided that the following conditions are met:
  
   1. Redistributions of source code must retain the above copyright notice, 
      this list of conditions and the following disclaimer.
  
   2. Redistributions in binary form must reproduce the above copyright 
      notice, this list of conditions and the following disclaimer in the 
      documentation and/or other materials provided with the distribution.
  
   3. Neither the name of the Intel Corporation nor the names of its 
      contributors may be used to endorse or promote products derived from 
      this software without specific prior written permission.
  
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.

******************************************************************************/

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "mrpd.h"

/* global variables */
int control_socket = -1;

#define VERSION_STR	"0.0"

static const char *version_str =
	"mrpctl v" VERSION_STR "\n"
	"Copyright (c) 2012, Intel Corporation\n";

int init_local_ctl(void)
{
	int sock_fd = -1;
	int sock_flags;
	struct sockaddr_in addr;

	sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock_fd < 0)
		goto err;

	sock_flags = fcntl(sock_fd, F_GETFL, 0);
	fcntl(sock_fd, F_SETFL, sock_flags | O_NONBLOCK);

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(MRPD_PORT_DEFAULT);
	inet_aton("127.0.0.1", &addr.sin_addr);

	/* rc = bind(sock_fd, (struct sockaddr *)&addr, addr_len); */
	/* if (rc < 0) goto out; */

	memset(&addr, 0, sizeof(addr));
	/* 
	 * use an abstract socket address with a leading null character 
	 * note this is non-portable
	 */

	printf("connected!\n");
	control_socket = sock_fd;

	return 0;
err:
	if (sock_fd != -1)
		close(sock_fd);
	sock_fd = -1;
	return -1;
}

int process_ctl_msg(char *buf, int buflen, struct sockaddr_in *client)
{
	(void)buflen; /* unused */
	(void)client; /* unused */

	/*
	 * M?? - query MMRP Registrar MAC Address database
	 * M+? - JOIN_MT a MAC address
	 * -- note M++ doesn't exist apparently- MMRP doesn't use 'New' --
	 * M-- - LV a MAC address
	 * V?? - query MVRP Registrar VID database
	 * V++ - JOIN_IN a VID (VLAN ID)
	 * V+? - JOIN_MT a VID (VLAN ID)
	 * V-- - LV a VID (VLAN ID)
	 */

	if (buf[1] == ':')
		printf("?? RESP:\n%s", buf);
	else
		printf("MRPD ---> %s", buf);
	fflush(stdout);

	return 0;
}

int recv_ctl_msg(void)
{
	char *msgbuf;
	struct sockaddr_in client_addr;
	struct msghdr msg;
	struct iovec iov;
	int bytes = 0;

	msgbuf = (char *)malloc (MAX_MRPD_CMDSZ);
	if (NULL == msgbuf) 
		goto err;
	memset(&msg, 0, sizeof(msg));
	memset(&client_addr, 0, sizeof(client_addr));
	memset(msgbuf, 0, MAX_MRPD_CMDSZ);

	iov.iov_len = MAX_MRPD_CMDSZ;
	iov.iov_base = msgbuf;
	msg.msg_name = &client_addr;
	msg.msg_namelen = sizeof(client_addr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	bytes = recvmsg(control_socket, &msg, 0);
	if (bytes < 0)
		goto err;

	return process_ctl_msg(msgbuf, bytes, &client_addr);
err:
	free(msgbuf);
	
	return -1;
}

int send_control_msg(char *notify_data, int notify_len)
{
	struct sockaddr_in	addr;
	socklen_t addr_len;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(MRPD_PORT_DEFAULT);
	inet_aton("127.0.0.1", &addr.sin_addr);
	addr_len = sizeof(addr);

	if (control_socket != -1)
		return sendto(control_socket, notify_data, notify_len, 0,
			(struct sockaddr *)&addr, addr_len);
	else
		return 0;
}

void usage(void)
{
	fprintf(stderr, 
		"\n"
		"usage: mrpctl [-h]"
		"\n"
		"options:\n"
		"    -h  show this message\n"
		"\n"
		"%s"
		"\n", version_str);
	exit(1);
}


int main(int argc, char *argv[])
{
	int	c;
	int	rc = 0;
	char	*msgbuf;
	int	status;

	for (;;) {
		c = getopt(argc, argv, "h");
		if (c < 0)
			break;
		switch (c) {
		case 'h':
			usage();
			break;
		}
	}
	if (optind < argc)
		usage();

	rc = init_local_ctl();
	if (rc) {
		printf("init_local_ctl failed\n");
		goto out;
	}

	msgbuf = malloc(1500);
	if (NULL == msgbuf) {
		printf("malloc failed\n");
		return -1;
	}

#ifdef XXX
	memset(msgbuf,0,1500);
	sprintf(msgbuf,"M++:M=010203040506");
	rc = send_control_msg(msgbuf, 1500 );
	
	memset(msgbuf,0,1500);
	sprintf(msgbuf,"M++:M=ffffffffffff");
	rc = send_control_msg(msgbuf, 1500 );

	memset(msgbuf,0,1500);
	sprintf(msgbuf,"V++:I=0002");
	rc = send_control_msg(msgbuf, 1500 );

	memset(msgbuf,0,1500);
	sprintf(msgbuf,"M++:M=060504030201");
	rc = send_control_msg(msgbuf, 1500 );

	memset(msgbuf,0,1500);
	sprintf(msgbuf,"M++:S=1");
	rc = send_control_msg(msgbuf, 1500 );

	memset(msgbuf,0,1500);
	sprintf(msgbuf,"M--:M=060504030201");
	rc = send_control_msg(msgbuf, 1500 );

	memset(msgbuf,0,1500);
	sprintf(msgbuf,"M--:S=1");
	rc = send_control_msg(msgbuf, 1500 );

	sprintf(msgbuf,"M+?:M=060504030201");
	rc = send_control_msg(msgbuf, 1500 );

	memset(msgbuf,0,1500);
	sprintf(msgbuf,"M+?:S=1");
	rc = send_control_msg(msgbuf, 1500 );

	memset(msgbuf,0,1500);
	sprintf(msgbuf,"M--:M=060504030201");
	rc = send_control_msg(msgbuf, 1500 );

	memset(msgbuf,0,1500);
	sprintf(msgbuf,"M--:S=1");
	rc = send_control_msg(msgbuf, 1500 );

	sprintf(msgbuf,"V--:I=0002");
	rc = send_control_msg(msgbuf, 1500 );

	sprintf(msgbuf,"V+?:I=0002");
	rc = send_control_msg(msgbuf, 1500 );

	sprintf(msgbuf,"V--:I=0002");
	rc = send_control_msg(msgbuf, 1500 );

	memset(msgbuf,0,1500);
	sprintf(msgbuf,"S+?:S=DEADBEEFBADFCA11,A=112233445566,V=0002,Z=576,I=8000,P=96,L=1000");
	rc = send_control_msg(msgbuf, 1500);

	memset(msgbuf,0,1500);
	sprintf(msgbuf,"S--:S=DEADBEEFBADFCA11");
	rc = send_control_msg(msgbuf, 1500);

	memset(msgbuf,0,1500);
	sprintf(msgbuf,"S++:S=FFEEDDCCBBAA9988,A=112233445567,V=0002,Z=576,I=8000,P=96,L=1000");
	rc = send_control_msg(msgbuf, 1500);

	memset(msgbuf,0,1500);
	sprintf(msgbuf,"S+L:L=DEADBEEFBADFCA11,D=2");
	rc = send_control_msg(msgbuf, 1500);

	memset(msgbuf,0,1500);
	sprintf(msgbuf,"S+L:L=F00F00F00F00F000,D=2");
	rc = send_control_msg(msgbuf, 1500);

	memset(msgbuf,0,1500);
	sprintf(msgbuf, "S+D:C=6,P=3,V=0002");
	rc = send_control_msg(msgbuf, 1500);

	memset(msgbuf,0,1500);
	sprintf(msgbuf, "S-D:C=6,P=3,V=0002");
	rc = send_control_msg(msgbuf, 1500);

	memset(msgbuf,0,1500);
	sprintf(msgbuf,"S-L:L=F00F00F00F00F000");
	rc = send_control_msg(msgbuf, 1500);
#endif /* XXX */

	memset(msgbuf,0,1500);
	sprintf(msgbuf,"V++:I=0002"); /* JOIN_IN VID 2 (SR_PVID, SRP traffic */
	rc = send_control_msg(msgbuf, 1500 );

#ifdef YYYY	
	memset(msgbuf,0,1500);
	sprintf(msgbuf,"S+L:L=0050c24edb0a0001,D=2");
	rc = send_control_msg(msgbuf, 1500);
#endif /* YYYY */

	do {
		memset(msgbuf,0,1500);
		sprintf(msgbuf,"M??"); /* query MMRP Registrar MAC Address database */
		rc = send_control_msg(msgbuf, 1500);
		memset(msgbuf,0,1500);
		sprintf(msgbuf,"V??"); /* query MVRP Registrar VID database */
		rc = send_control_msg(msgbuf, 1500 );
		memset(msgbuf,0,1500);
		sprintf(msgbuf,"S??"); /* query MSRP Registrar database */
		rc = send_control_msg(msgbuf, 1500 );
		status = 0;
		while (status >= 0) {
			status = recv_ctl_msg();
		}
		sleep(1);
	} while (1);

out:
	printf("exiting (rc=%d)\n", rc);

	return rc;
}
