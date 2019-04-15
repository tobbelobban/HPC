#include "nbody.h"

void nbody(struct Body *bodies, int steps, int output_steps, int N, double G, double DT, double EPS)
{
	FILE *checkpoint = NULL;
	char buffer[1024];

//	double t1, t2;

	for (int i = 0; i < steps; i++) {
		if (output_steps != 0 && (i + output_steps) % output_steps == 0) {
			snprintf(buffer, 1024, "%d.txt", i);
			checkpoint = fopen(buffer, "w");
		}

		//t1 = omp_get_wtime();
		int j;
		#pragma omp parallel for
		for (j = 0; j < N; j++) {
			double fx = 0.0;
			double fy = 0.0;
			double fz = 0.0;

			double dx;
			double dy;
			double dz;

			double f;
			double r;

			double ax;
			double ay;
			double az;
			
			for (int k = 0; k < N; k++) {
				if (j != k) {
					dx = bodies[j].position[0] - bodies[k].old_position[0];
					dy = bodies[j].position[1] - bodies[k].old_position[1];
					dz = bodies[j].position[2] - bodies[k].old_position[2];
					r = sqrt(dx * dx + dy * dy + dz * dz);
					f = -G * (bodies[j].mass * bodies[k].mass) / pow((r * r) + (EPS * EPS), 1.5);

					fx += f * dx / r;
					fy += f * dy / r;
					fz += f * dz / r;
				}
			}

			ax = fx / bodies[j].mass;
			ay = fy / bodies[j].mass;
			az = fz / bodies[j].mass;
		
			bodies[j].velocity[0] += ax * DT;
			bodies[j].velocity[1] += ay * DT;
			bodies[j].velocity[2] += az * DT;
		
			bodies[j].position[0] += bodies[j].velocity[0] * DT;
			bodies[j].position[1] += bodies[j].velocity[1] * DT;
			bodies[j].position[2] += bodies[j].velocity[2] * DT;
		}

		#pragma omp parallel for
		for (j = 0; j < N; j++) {
			bodies[j].old_position[0] = bodies[j].position[0];
			bodies[j].old_position[1] = bodies[j].position[1];
			bodies[j].old_position[2] = bodies[j].position[2];

			if (checkpoint != NULL)
				fprintf(checkpoint, "%d\t%f\t%f\t%f\n\n\n", j, bodies[j].position[0], bodies[j].position[1], bodies[j].position[2]);
		}

		//t2 = omp_get_wtime();
	
		if (checkpoint != NULL) {
			fclose(checkpoint);
			checkpoint = NULL;
		}

		// printf("step = %d, runtime: %f\n", i, t2 - t1);
	}
}
