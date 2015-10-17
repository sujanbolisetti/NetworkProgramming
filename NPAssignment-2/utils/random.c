/*
 * random.c
 *
 *  Created on: Oct 16, 2015
 *      Author: sujan
 */


#include "../src/usp.h"

unsigned int RANDOM_MIN = 0;
unsigned int RANDOM_MAX = 100;

unsigned int rand_interval(unsigned int min, unsigned int max)
{
    	int r;
    	const unsigned int range = 1 + max - min;
    	const unsigned int buckets = RAND_MAX / range;
    	const unsigned int limit = buckets * range;

    /* Create equal size buckets all in a row, then fire randomly towards
     * the buckets until you land in one of them. All buckets are equally
     * likely. If you land off the end of the line of buckets, try again. */
    	do
    	{
        	r = rand();
    	}
	while (r >= limit);

    	return min + (r / buckets);
}

unsigned int is_in_limits(float r)
{
	unsigned int ran = r*100;
	if(rand_interval(RANDOM_MIN, RANDOM_MAX) < ran)
		return 1;
	return 0;
}

