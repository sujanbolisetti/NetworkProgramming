/*
 * random.c
 *
 *  Created on: Oct 16, 2015
 *      Author: sujan
 */


#include "../src/usp.h"

unsigned int RANDOM_MIN = 0;
unsigned int RANDOM_MAX = 100;

int rseed;
void setRandomSeed(int seed)
{
	rseed = seed;
	srand(seed);
}

unsigned int rand_interval(unsigned int min, unsigned int max)
{
    	int r;
    	const unsigned int range = 1 + max - min;
    	const unsigned int buckets = RAND_MAX / range;
    	const unsigned int limit = buckets * range;

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
