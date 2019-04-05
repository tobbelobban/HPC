#ifndef __NBODY_H__
#define __NBODY_H__

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

struct Body {
	double position[3];
	double old_position[3];
	double velocity[3];
	double mass;
};

void nbody(struct Body*, int, int, int, double, double, double);

#endif
