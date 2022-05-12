#include "cachelab.h"
#include "getopt.h"
#include "stdlib.h"
#include "unistd.h"
#include "stdio.h"
#include "stdint.h"

#define true 1
#define false 0

const char *usage = "Usage: ./csim-ref [-hv] -s <s> -E <E> -b <b> -t <tracefile>";


typedef struct {
    uint64_t tag;
    uint64_t LRU_time_counter;
    _Bool valid;
}cache_line;

typedef cache_line *cache_line_entry;
typedef cache_line_entry *cache_set_entry;

uint32_t hit = 0;
uint32_t miss = 0;
uint32_t eviction = 0;

cache_set_entry cache_initialize(uint64_t S, uint64_t E)
{
    cache_set_entry c;

    if ((c = calloc(S, sizeof(cache_line_entry))) == NULL) {
	perror("Failed to calloc cache_set_entry.");
	exit(EXIT_FAILURE);
    }

    for (int i = 0; i < S ; i++) {
	if ((c[i] = calloc(E, sizeof(cache_line)) == NULL))
	    perror("Failed to calloc line in sets.");
    }
    
    return c;
}

void cache_free(cache_set_entry c, uint64_t S)
{
    for (int i = 0; i < S; i++)
	free(c[i]);
    free(c);
}

void cache_test(cache_line_entry search_line, uint64_t E, uint64_t tag, _Bool verbose)
{
    uint64_t oldest_time = UINT64_MAX;
    uint64_t youngest_time = 0;
    uint64_t oldest_block = UINT64_MAX;
    _Bool hit_flag = false;
    for (int i = 0; i < E; i++) {
        if ((search_line[i].tag == tag) && search_line[i].valid) {
	    if (verbose) printf("hit.\n");
	    hit_flag = true;
	    hit++;
	    search_line[i].LRU_time_counter++;
	    break;
	}
    }

    if (!hit_flag) {
	if (verbose) printf("miss.\n");
	miss++;
	for (int i = 0; i < E; i++) {
	    if (search_line[i].LRU_time_counter < oldest_time) {
		oldest_time = search_line[i].LRU_time_counter;
		oldest_block = i;
	    }
	    if (search_line[i].LRU_time_counter > youngest_time) {
		youngest_time = search_line[i].LRU_time_counter;
	    }
	}

	search_line[oldest_block].LRU_time_counter = youngest_time + 1;
	search_line[oldest_block].tag = tag;

	if (search_line[oldest_block].valid) {
	    if (verbose) printf(" and eviction.\n");
	    eviction++;
	}else {
	    if (verbose) printf("\n");
	    search_line[oldest_block].valid = true;
	}
    }
}

void cache_access(FILE* tracefile, cache_set_entry* c, uint64_t s, uint64_t E, uint64_t b, uint64_t S, _Bool verbose)
{
    char ch;
    uint64_t address;
    while ((fscanf(tracefile, " %c %lx%*[^\n]", &ch, &address)) == 2) {
        if (ch == 'I')
	    continue;
	else {
	    uint64_t set_index_mask = (1 << s) - 1;
	    uint64_t set_index = (address >> b) & set_index_mask;
	    uint64_t tag = (address >> b) >> s;
	    cache_line_entry search_line = c[set_index];

	    if (ch == 'S' || ch == 'L') {
		if (verbose) printf("%c %lx ", ch, address);
		cache_test(search_line, E, tag, verbose);
	    }else if (ch == 'M') {
		if (verbose) printf("%c %lx ", ch, address);
		cache_test(search_line, E, tag, verbose);
		cache_test(search_line, E, tag, verbose);
	    }else {
		continue;
	    }
	}
    }
}


int main(int argc, char* argv[])
{
    FILE* tracefile = NULL;
    cache_set_entry cache = NULL;
    _Bool verbose = 0;
    uint64_t s = 0;
    uint64_t E = 0;
    uint64_t b = 0;
    uint64_t S = 0;
    
    char opt;

    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
	switch (opt) {
	    case 'h':
		fprintf(stdout, usage, argv[0]);
		break;
	    case 'v':
		verbose = 1;
		break;
	    case 's':
		s = atoi(optarg);
		if (s <= 0) {
		    fprintf(stdout, usage, argv[0]);
		    exit(EXIT_FAILURE);
		}
		S = 1 << s;
		break;
	    case 'E':
		E = atoi(optarg);
		if (E <= 0) {
		    fprintf(stdout, usage, argv[0]);
		    exit(EXIT_FAILURE);
		}
		break;
	    case 'b':
		b = atoi(optarg);
		if (b <= 0) {
		    fprintf(stdout, usage, argv[0]);
		    exit(EXIT_FAILURE);
		}
		break;
	    case 't':
		if ((tracefile = fopen(optarg, "r")) == NULL) {
		    perror("Failed to open tracefile.");
		    exit(EXIT_FAILURE);
		}
		break;
	    default:
		fprintf(stdout, usage, argv[0]);
		exit(EXIT_FAILURE);
	}
    }
    if (!s || !E || !b || !tracefile) {
	fprintf(stdout, usage, argv[0]);
	exit(EXIT_FAILURE);
    }
    cache = cache_initialize(S, E);
    cache_access(tracefile, cache, s, E, b, S, verbose);
    cache_free(cache, S);
    printSummary(hit, miss, eviction);
    return 0;
}
