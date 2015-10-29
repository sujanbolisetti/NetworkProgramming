/*
 * random.c
 *
 *  Created on: Oct 16, 2015
 *       Authors: sujan, sidhartha
 */


#include "../src/usp.h"

unsigned int RANDOM_MIN = 0;
unsigned int RANDOM_MAX = 100;

void setRandomSeed(int seed)
{
        srand(seed);
        if(DEBUG)
        	printf("Random seed set to %d\n", seed);
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

bool is_in_limits(float r)
{
        unsigned int ran = r*100;
        if(rand_interval(RANDOM_MIN, RANDOM_MAX) <= ran)
        {
        	return true;
        }

        return false;
}
