/*
 * server.c
 *
 *  Created on: Oct 4, 2015
 *      Author: sujan
 */

#include "usp.h"

int main(int argc, char **argv){

	FILE *fp;

	fp=fopen("server.in","r");

	int portNumber,slidingWindowSize;


	fscanf(fp,"%d",&portNumber);
	fscanf(fp,"%d",&slidingWindowSize);

	printf("%d %d\n",portNumber,slidingWindowSize);
}



