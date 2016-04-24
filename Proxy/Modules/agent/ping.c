/*
 * ping.c
 *
 *  Created on: Mar 31, 2016
 *      Author: martin
 */

#include <sys/param.h>		/* NB: we rely on this for <sys/types.h> */
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/uio.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
//#include <netinet/ip_var.h>
#include <arpa/inet.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <math.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include "agent.h"

#define	INADDR_LEN	((int)sizeof(in_addr_t))
#define	TIMEVAL_LEN	((int)sizeof(struct timeval))
#define	MASK_LEN	(ICMP_MASKLEN - ICMP_MINLEN)
#define	TS_LEN		(ICMP_TSLEN - ICMP_MINLEN)
#define	DEFDATALEN	56		/* default data length */
#define	FLOOD_BACKOFF	20000		/* usecs to back off if F_FLOOD mode */
/* runs out of buffer space */
#define	MAXIPLEN	(sizeof(struct ip) + MAX_IPOPTLEN)
#define	MAXICMPLEN	(ICMP_ADVLENMIN + MAX_IPOPTLEN)
#define	MAXWAIT		10		/* max seconds to wait for response */
#define	MAXALARM	(60 * 60)	/* max seconds for alarm timeout */
#define	MAXTOS		255

#define	A(bit)		rcvd_tbl[(bit)>>3]	/* identify byte in array */
#define	B(bit)		(1 << ((bit) & 0x07))	/* identify bit in byte */
#define	SET(bit)	(A(bit) |= B(bit))
#define	CLR(bit)	(A(bit) &= (~B(bit)))
#define	TST(bit)	(A(bit) & B(bit))

#define	MAX_DUP_CHK	(8 * 128)
int mx_dup_ck = MAX_DUP_CHK;
char rcvd_tbl[MAX_DUP_CHK / 8];

struct sockaddr_in whereto;	/* who to ping */
int datalen = DEFDATALEN;
int maxpayload;
int s;				/* socket file descriptor */
u_char outpackhdr[IP_MAXPACKET], *outpack;
char BBELL = '\a';		/* characters written for MISSED and AUDIBLE */
char BSPACE = '\b';		/* characters written for flood */
char DOT = '.';
char *hostname;
char *shostname;
int ident;			/* process id to identify our packets */
int uid;			/* cached uid for micro-optimization */
u_char icmp_type = ICMP_ECHO;
u_char icmp_type_rsp = ICMP_ECHOREPLY;
int phdr_len = 0;
int send_len;

/* counters */
long nmissedmax;		/* max value of ntransmitted - nreceived - 1 */
long npackets;			/* max packets to transmit */
long nreceived;			/* # of packets we got back */
long nrepeats;			/* number of duplicates */
long ntransmitted;		/* sequence # for outbound packets = #sent */
int interval = 1000;		/* interval between packets, ms */

/* timing */
int timing;			/* flag to do timing */
double tmin = 999999999.0;	/* minimum round trip time */
double tmax = 0.0;		/* maximum round trip time */
double tsum = 0.0;		/* sum of all times, for doing average */
double tsumsq = 0.0;		/* sum of all times squared, for std. dev. */

struct timeval tv1;		//time sent


static u_short in_cksum(u_short *, int);
static void pinger(void);
static char *pr_addr(struct in_addr);
static int pr_pack(char *, int, struct sockaddr_in *, struct timeval *);
static void tvsub(struct timeval *, struct timeval *);

static struct timeval last;
/* returns  0 - not an echo
* returns  1 - our echo
* returns -1 - not ours
*/
int pingServer(char *target)
{
	struct sockaddr_in from;
	struct timeval intvl;
	struct iovec iov;

	struct msghdr msg;
	u_char packet[IP_MAXPACKET];
	char *source;

	struct sockaddr_in *to;

	int  hold,  icmp_len, sockerrno;


	source = NULL;

	s = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	sockerrno = errno;

	outpack = outpackhdr + sizeof(struct ip);

	icmp_len = sizeof(struct ip) + ICMP_MINLEN + phdr_len;

	maxpayload = IP_MAXPACKET - icmp_len;
	if (datalen > maxpayload)
		errx(EX_USAGE, "packet size too large: %d > %d", datalen,
				maxpayload);
	send_len = icmp_len + datalen;

	bzero(&whereto, sizeof(whereto));
	to = &whereto;
	to->sin_family = AF_INET;
	//	to->sin_len = sizeof *to;
	if (inet_aton(target, &to->sin_addr) != 0) {
		hostname = target;
	}

	if (datalen >= TIMEVAL_LEN)	/* can we time transfer */
		timing = 1;

	ident = getpid() & 0xFFFF;

	if (s < 0) {
		errno = sockerrno;
		err(EX_OSERR, "socket");
	}
	hold = 1;

#ifdef SO_TIMESTAMP
	{ int on = 1;
	if (setsockopt(s, SOL_SOCKET, SO_TIMESTAMP, &on, sizeof(on)) < 0)
		err(EX_OSERR, "setsockopt SO_TIMESTAMP");
	}
#endif

	/*
	 * XXX receive buffer needs undetermined space for mbuf overhead
	 * as well.
	 */
	hold = IP_MAXPACKET + 128;
	(void)setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *)&hold,
			sizeof(hold));
	if (uid == 0)
		(void)setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *)&hold,
				sizeof(hold));

	if (to->sin_family == AF_INET) {
		(void)printf("PING %s (%s)", hostname,
				inet_ntoa(to->sin_addr));
		if (source)
			(void)printf(" from %s", shostname);
		(void)printf(": %d data bytes\n", datalen);
	} else
		(void)printf("PING %s: %d data bytes\n", hostname, datalen);

	bzero(&msg, sizeof(msg));
	msg.msg_name = (caddr_t)&from;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	iov.iov_base = packet;
	iov.iov_len = IP_MAXPACKET;

	pinger();		/* send the ping */

	(void)gettimeofday(&last, NULL);

	intvl.tv_sec = interval / 1000;
	intvl.tv_usec = interval % 1000 * 1000;

	while(1)
	{
		struct timeval now, timeout;
		fd_set rfds;
		int cc, n;

		if ((unsigned)s >= FD_SETSIZE)
			errx(EX_OSERR, "descriptor too large");
		FD_ZERO(&rfds);
		FD_SET(s, &rfds);
		(void)gettimeofday(&now, NULL);
		timeout.tv_sec = last.tv_sec + intvl.tv_sec - now.tv_sec;
		timeout.tv_usec = last.tv_usec + intvl.tv_usec - now.tv_usec;
		while (timeout.tv_usec < 0) {
			timeout.tv_usec += 1000000;
			timeout.tv_sec--;
		}
		while (timeout.tv_usec >= 1000000) {
			timeout.tv_usec -= 1000000;
			timeout.tv_sec++;
		}
		if (timeout.tv_sec < 0)
			timeout.tv_sec = timeout.tv_usec = 0;
		n = select(s + 1, &rfds, NULL, NULL, &timeout);
		if (n < 0)
			continue;	/* Must be EINTR. */
		else if (n == 1) {
			struct timeval *tv = NULL;
			msg.msg_namelen = sizeof(from);
			if ((cc = recvmsg(s, &msg, 0)) < 0) {
				if (errno == EINTR)
					continue;
				warn("recvmsg");
				continue;
			}

			if (tv == NULL) {
				(void)gettimeofday(&now, NULL);
				tv = &now;
			}

			int reply = pr_pack((char *)packet, cc, &from, tv);
			if (reply >= 0) return reply;
		}
		else return 0;
	}
	return 0;
}


/*
 * pinger --
 *	Compose and transmit an ICMP ECHO REQUEST packet.  The IP packet
 * will be added on by the kernel.  The ID field is our UNIX process ID,
 * and the sequence number is an ascending integer.  The first TIMEVAL_LEN
 * bytes of the data portion are used to hold a UNIX "timeval" struct in
 * host byte-order, to compute the round-trip time.
 */
static void
pinger(void)
{

	struct icmp *icp;
	int cc;
	u_char *packet;

	packet = outpack;
	icp = (struct icmp *)outpack;
	icp->icmp_type = icmp_type;
	icp->icmp_code = 0;
	icp->icmp_cksum = 0;
	icp->icmp_seq = htons(ntransmitted);
	icp->icmp_id = ident;			/* ID */

	CLR(ntransmitted % mx_dup_ck);

	cc = ICMP_MINLEN + phdr_len + datalen;

	/* compute ICMP checksum here */
	icp->icmp_cksum = in_cksum((u_short *)icp, cc);

	sendto(s, (char *)packet, cc, 0, (struct sockaddr *)&whereto,
			sizeof(whereto));

	ntransmitted++;

}

/*
 * pr_pack --
 *	Print out the packet, if it came from us.  This logic is necessary
 * because ALL readers of the ICMP socket get a copy of ALL ICMP packets
 * which arrive ('tis only fair).  This permits multiple copies of this
 * program to be run without having intermingled output (or statistics!).
 * returns  0 - not an echo
 * returns  1 - our echo
 * returns -1 - not ours
 */
static int
pr_pack(buf, cc, from, tv)
char *buf;
int cc;
struct sockaddr_in *from;
struct timeval *tv;
{
	struct in_addr ina;
	struct icmp *icp;
	struct ip *ip;
	const void *tp;
	u_char *cp;
	double triptime;
	int dupflag, hlen, i, j, recv_len, seq;
	static int old_rrlen;
	static char old_rr[MAX_IPOPTLEN];
	int reply = 0;

	/* Check the IP header */
	ip = (struct ip *)buf;
	hlen = ip->ip_hl << 2;
	recv_len = cc;
	if (cc < hlen + ICMP_MINLEN) {
		return -1;
	}

	/* Now the ICMP part */
	cc -= hlen;
	icp = (struct icmp *)(buf + hlen);
	if (icp->icmp_type == icmp_type_rsp) {
		if (icp->icmp_id != ident)
			return -1;			/* 'Twas not our ECHO */
		++nreceived;
		triptime = 0.0;
		if (timing) {
			struct timeval tv1;
#ifndef icmp_data
			tp = &icp->icmp_ip;
#else
			tp = icp->icmp_data;
#endif
			tp = (const char *)tp + phdr_len;

			if (cc - ICMP_MINLEN - phdr_len >= sizeof(tv1)) {
				/* Copy to avoid alignment problems: */
				memcpy(&tv1, tp, sizeof(tv1));
				tvsub(tv, &last);
				triptime = ((double)tv->tv_sec) * 1000.0 +
						((double)tv->tv_usec) / 1000.0;
				tsum += triptime;
				tsumsq += triptime * triptime;
				if (triptime < tmin)
					tmin = triptime;
				if (triptime > tmax)
					tmax = triptime;
			} else
				timing = 0;
		}

		seq = ntohs(icp->icmp_seq);

		if (TST(seq % mx_dup_ck)) {
			++nrepeats;
			--nreceived;
			dupflag = 1;
		} else {
			SET(seq % mx_dup_ck);
			dupflag = 0;
		}

		(void)printf("%d bytes from %s: icmp_seq=%u", cc,
				inet_ntoa(*(struct in_addr *)&from->sin_addr.s_addr),
				seq);
		(void)printf(" ttl=%d", ip->ip_ttl);
		if (timing)
			(void)printf(" time=%.3f ms", triptime);
		if (dupflag)
			(void)printf(" (DUP!)");
		if (recv_len != send_len) {
			(void)printf(
					"\nwrong total length %d instead of %d",
					recv_len, send_len);
		}
		printf("\n");
		reply = 1;
	} else {
		/*
		 * We've got something other than an ECHOREPLY.
		 * See if it's a reply to something that we sent.
		 * We can compare IP destination, protocol,
		 * and ICMP type and ID.
		 *
		 * Only print all the error messages if we are running
		 * as root to avoid leaking information not normally
		 * available to those not running as root.
		 */

//		return -1;
	}

	/* Display any IP options */
	cp = (u_char *)buf + sizeof(struct ip);

	for (; hlen > (int)sizeof(struct ip); --hlen, ++cp)
		switch (*cp) {
		case IPOPT_EOL:
			hlen = 0;
			break;
		case IPOPT_LSRR:
		case IPOPT_SSRR:
			(void)printf(*cp == IPOPT_LSRR ?
					"\nLSRR: " : "\nSSRR: ");
			j = cp[IPOPT_OLEN] - IPOPT_MINOFF + 1;
			hlen -= 2;
			cp += 2;
			if (j >= INADDR_LEN &&
					j <= hlen - (int)sizeof(struct ip)) {
				for (;;) {
					bcopy(++cp, &ina.s_addr, INADDR_LEN);
					if (ina.s_addr == 0)
						(void)printf("\t0.0.0.0");
					else
						(void)printf("\t%s",
								pr_addr(ina));
					hlen -= INADDR_LEN;
					cp += INADDR_LEN - 1;
					j -= INADDR_LEN;
					if (j < INADDR_LEN)
						break;
					(void)putchar('\n');
				}
			} else
				(void)printf("\t(truncated route)\n");
			break;
		case IPOPT_RR:
			j = cp[IPOPT_OLEN];		/* get length */
			i = cp[IPOPT_OFFSET];		/* and pointer */
			hlen -= 2;
			cp += 2;
			if (i > j)
				i = j;
			i = i - IPOPT_MINOFF + 1;
			if (i < 0 || i > (hlen - (int)sizeof(struct ip))) {
				old_rrlen = 0;
				continue;
			}
			if (i == old_rrlen
					&& !bcmp((char *)cp, old_rr, i))
			{
				(void)printf("\t(same route)");
				hlen -= i;
				cp += i;
				break;
			}
			old_rrlen = i;
			bcopy((char *)cp, old_rr, i);
			(void)printf("\nRR: ");
			if (i >= INADDR_LEN &&
					i <= hlen - (int)sizeof(struct ip)) {
				for (;;) {
					bcopy(++cp, &ina.s_addr, INADDR_LEN);
					if (ina.s_addr == 0)
						(void)printf("\t0.0.0.0");
					else
						(void)printf("\t%s",
								pr_addr(ina));
					hlen -= INADDR_LEN;
					cp += INADDR_LEN - 1;
					i -= INADDR_LEN;
					if (i < INADDR_LEN)
						break;
					(void)putchar('\n');
				}
			} else
				(void)printf("\t(truncated route)");
			break;
		case IPOPT_NOP:
			(void)printf("\nNOP");
			break;
		default:
			(void)printf("\nunknown option %x", *cp);
			break;
		}
	return reply;
}

/*
 * in_cksum --
 *	Checksum routine for Internet Protocol family headers (C Version)
 */
u_short
in_cksum(addr, len)
u_short *addr;
int len;
{
	int nleft, sum;
	u_short *w;
	union {
		u_short	us;
		u_char	uc[2];
	} last;
	u_short answer;

	nleft = len;
	sum = 0;
	w = addr;

	/*
	 * Our algorithm is simple, using a 32 bit accumulator (sum), we add
	 * sequential 16 bit words to it, and at the end, fold back all the
	 * carry bits from the top 16 bits into the lower 16 bits.
	 */
	while (nleft > 1)  {
		sum += *w++;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if (nleft == 1) {
		last.uc[0] = *(u_char *)w;
		last.uc[1] = 0;
		sum += last.us;
	}

	/* add back carry outs from top 16 bits to low 16 bits */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return(answer);
}

/*
 * tvsub --
 *	Subtract 2 timeval structs:  out = out - in.  Out is assumed to
 * be >= in.
 */
static void
tvsub(out, in)
struct timeval *out, *in;
{

	if ((out->tv_usec -= in->tv_usec) < 0) {
		--out->tv_sec;
		out->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}


/*
 * pr_addr --
 *	Return an ascii host address as a dotted quad and optionally with
 * a hostname.
 */
static char *
pr_addr(ina)
struct in_addr ina;
{
	struct hostent *hp;
	static char buf[16 + 3 + MAXHOSTNAMELEN];

	if (!(hp = gethostbyaddr((char *)&ina, 4, AF_INET)))
		return inet_ntoa(ina);
	else
		(void)snprintf(buf, sizeof(buf), "%s (%s)", hp->h_name,
		    inet_ntoa(ina));
	return(buf);
}

