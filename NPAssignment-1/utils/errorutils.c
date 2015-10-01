/*
 * errorutils.c
 *		Error message displaying functions.
 *
 *  Created on: Sep 17, 2015
 *      Author: sujan
 */

#include "../src/usp.h"

/**
 * Helper function to format the error message.
 */
char* create_error_message(char *error_msg,int errorno){

	char* errorbuff = malloc(sizeof(char)*100);

	strcat(errorbuff,error_msg);
	strcat(errorbuff,strerror(errorno));

	return errorbuff;
}

