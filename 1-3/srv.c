#include <unistd.h>
#include <string.h>

#define MAX_BUF 4096

int main()
{
	char buf[MAX_BUF];
	char *split_buf[128];
	memset(buf, 0, sizeof(buf));
	read(0, buf, sizeof(buf));
	
	return 0;
}
