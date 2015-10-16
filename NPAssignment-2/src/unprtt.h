/*
 * unprtt.h
 *
 *  Created on: Oct 14, 2015
 *      Author: sujan
 */

#ifndef UNPRTT_H_
#define UNPRTT_H_

#include	"usp.h"

struct rtt_info {
  int		rtt_rtt;	/* most recent measured RTT, in seconds */
  int		rtt_srtt;	/* smoothed RTT estimator, in seconds */
  int		rtt_rttvar;	/* smoothed mean deviation, in seconds */
  int		rtt_rto;	/* current RTO to use, in milli-seconds */
  int		rtt_nrexmt;	/* # times retransmitted: 0, 1, 2, ... */
  uint32_t	rtt_base;	/* # sec since 1/1/1970 at start */
};

#define	RTT_RXTMIN      1000 /* min retransmit timeout value, in milli-seconds */
#define	RTT_RXTMAX      3000 /* max retransmit timeout value, in milli-seconds */
#define	RTT_MAXNREXMT 	12	/* max # times to retransmit */

				/* function prototypes */
void	 rtt_debug(struct rtt_info *);
void	 rtt_init(struct rtt_info *);
void	 rtt_newpack(struct rtt_info *);
int		 rtt_start(struct rtt_info *);
void	 rtt_stop(struct rtt_info *, uint32_t);
int		 rtt_timeout(struct rtt_info *);
uint32_t rtt_ts(struct rtt_info *);

extern int	rtt_d_flag;	/* can be set to nonzero for addl info */


#endif /* UNPRTT_H_ */
