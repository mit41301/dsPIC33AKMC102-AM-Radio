#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>

int main(void)
{
	for (float f = 0.0F; f < M_PI * 2.0f; f += 0.2f)
	{
		printf("\r\nPhi=%f, sin(Phi)=%+12.8f, cos(Phi)=%+12.8f",f,(float)sin(f),(float)cos(f));
	}
	return 0;
}
