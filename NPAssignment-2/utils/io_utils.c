/*
 * io_utils.c
 *
 *  Created on: Oct 10, 2015
 *      Author: sujan
 */

#include "../src/usp.h"

void Fscanf(FILE *fp, char *format, void *data){

	if(fscanf(fp,format,data) < 0){
		printf("Error in reading the file : %s",strerror(errno));
	}
}
