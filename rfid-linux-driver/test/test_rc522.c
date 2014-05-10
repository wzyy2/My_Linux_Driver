#include <stdio.h> 
#include <string.h> 
#include <stdlib.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <errno.h> 
#include <arpa/inet.h> 
#include <sys/time.h> 
#include <sys/types.h> 
#include <sys/stat.h> 
#include <fcntl.h> 
#include <sys/ioctl.h> 
#include <math.h> 


static enum IO_CMD { 
	READ_CARD = 3, 
	CHANGE_PASSWD = 1, 
	CHANGE_BLOCK = 2, 
	CHANGE_KEY = 5, 
	WRITE_CARD = 4, 
	GET_ID = 6, 
}; 

int main(int argc, char** argv) 
{ 
	int rc522_fd; 
	int i, read_num; 
	char r[256];
	char a[16]; 
	printf("test: rc522 %s %s\n", __DATE__, __TIME__); 
	printf("test: before open rc522_fd\n"); 
	rc522_fd = open("/dev/rfid_rc522_dev", O_RDWR); 
	printf("test: rc522_fd=%d\n", rc522_fd); 
	if(rc522_fd == -1) 
	{ 
		printf("test: Error Opening rc522\n"); 
		return(-1); 
	} 
	printf("test: wait 01\n"); 
	sleep(1); //wait 
	printf("test: wait 02\n"); 
	/******* Never to open ********/ 

#if 0 
	write(rc522_fd, bufpw1, sizeof(bufpw1)); 
#endif 
	ioctl(rc522_fd, GET_ID, &(a[0]));//参数3：选第0块 */
	printf("%d%d%d%d", a[0],a[1],a[2],a[3]); 
	for(i = 0; i < 3; i++) //读三次卡号 
	{ 
		read_num = read(rc522_fd, r, 0); 
		printf("test: i=%d read_num=%d ", i, read_num); 
		if(read_num > 0){ 
			printf("r=[0x%.2X]", r[0]); 
		} 

		printf("\n"); 
		sleep(1); 
	}

	return 0; 
} 

