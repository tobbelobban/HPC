#ifndef _MAPREDUCE_H
#define _MAPREDUCE_H

#define MASTER 0

class MapReduce {
public:
	int world_rank;
	int world_size;
	void init();
	void read(const char *);
	void write();
	void map();
	void reduce();
};

#endif
