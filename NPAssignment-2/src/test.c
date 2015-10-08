#include<stdio.h>
#include<stdlib.h>
struct sock_in{

	int *ptr;
};


int main(){

	int a=10;		
	struct sock_in *sock = (struct sock_in *)malloc(sizeof(struct sock_in));
	sock->ptr = &a;
	
	int *ptr = sock->ptr;
	printf("%d\n",*ptr); 
	return 0;

}
