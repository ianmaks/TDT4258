#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef enum { dm, fa } cache_map_t;
typedef enum { uc, sc } cache_org_t;
typedef enum { instruction, data } access_t;

typedef struct {
  uint32_t address;
  uint32_t tag;
  uint32_t index;
  uint32_t offset;
  access_t accesstype;
} mem_access_t;

typedef struct {
  uint64_t accesses;
  uint64_t hits;
  uint64_t misses;
  uint64_t evictions;
} cache_stat_t;

// Caches are arrays of 32-bit values, where each value is an address
// There is no valid bit or dirty bit, but a memory address of 0 is counts as invalid.
// This could lead to a false hit if the memory address 0 is accessed.
// If this cache simulator was upgraded to contain data references and writes to memory,
// implementing valid and dirty bits into the structure would need to be done.
uint32_t *cache;
uint32_t *instruction_cache;

int offset_bits = 6;
int index_bits = 0;
int tag_bits = 0;

// When using a fully associative cache, the cache head pointer is used to keep track of
// which cache block to evict when a miss occurs. The cache head pointer is incremented
// after each access, and wraps around to 0 when it reaches the end of the cache, simulating FIFO.
int data_cache_head = 0;
int instruction_cache_head = 0;


// Messing around with pointers
int* dch_pointer = &data_cache_head;
int* ich_pointer = &instruction_cache_head;

// DECLARE CACHES AND COUNTERS FOR THE STATS HERE
uint32_t cache_size;
uint32_t block_size = 64;
cache_map_t cache_mapping;
cache_org_t cache_org;

// USE THIS FOR YOUR CACHE STATISTICS
cache_stat_t cache_statistics;
char *file_name;

/* Reads a memory access from the trace file and returns
 * 1) access type (instruction or data access
 * 2) memory address
 */
mem_access_t read_transaction(FILE* ptr_file) {
  char type;
  mem_access_t access;

  if (fscanf(ptr_file, "%c %x\n", &type, &access.address) == 2) {
    if (type != 'I' && type != 'D') {
      printf("Unkown access type\n");
      exit(0);
    }
    access.accesstype = (type == 'I') ? instruction : data;
    return access;
  }

  /* If there are no more entries in the file,
   * return an address 0 that will terminate the infinite loop in main
   */
  access.address = 0;
  return access;
}



// Allocate memory for the cache, and initialize all entries to 0
uint32_t* init_cache(uint32_t cache_size, cache_map_t cache_mapping, cache_org_t cache_org) {
  uint32_t *new_cache = (uint32_t *) malloc(sizeof(uint32_t)*(cache_size/block_size));
  memset(new_cache, 0, sizeof(uint32_t)*(cache_size/block_size));
  uint32_t i;
 
  return new_cache;
}

// fa_access performs a fully associative cache access
// It is passed the cache, the access, and the cache head, of which the 
// cache and cache head will be either a data cache or an instruction cache

void fa_access(uint32_t* cache, mem_access_t access, int* cache_head) {
  // each access is recorded
  cache_statistics.accesses++;

  // check if the cache contains the address
  uint32_t i;
  for (i = 0; i < cache_size/block_size; i++) {
    if (cache[i] >> 6 == access.address >> 6) {
      // each hit is recorded
      cache_statistics.hits++;
      // printf("Cache hit: %x %x\n", cache[i], access.address);
      return;
    }
  }
  // if the for loop does not return, then the cache does not contain the address
  cache_statistics.misses++;
  // if the content of the cache line is not 0, then a cache line is evicted, which is recorded
  uint32_t evictee = cache[*cache_head];
  if (evictee != 0) {
    cache_statistics.evictions++;
  }
  // the oldest cache line pointed to by the cache head is set to the new address
  cache[*cache_head] = access.address;
  return;
}

// dm_access performs a direct mapped cache access
// It is passed the cache and the access

void dm_access(uint32_t* cache, mem_access_t access) {
  cache_statistics.accesses++;
  // The index bits are isolated to check the contents of the relevant cache line
  if (cache[(access.address << tag_bits) >> tag_bits + 6] >> 6 == access.address >> 6) {
      cache_statistics.hits++;
      return;
    }
  cache_statistics.misses++;
  if (cache[(access.address << tag_bits) >> tag_bits + 6] != 0) {
    cache_statistics.evictions++;
  }
  cache[(access.address << tag_bits) >> tag_bits + 6] = access.address;
  return;
}


int main(int argc, char** argv) {
  // Reset statistics:
  memset(&cache_statistics, 0, sizeof(cache_stat_t));

  /* Read command-line parameters and initialize:
   * cache_size, cache_mapping and cache_org variables
   */
  /* IMPORTANT: *IF* YOU ADD COMMAND LINE PARAMETERS (you really don't need to),
   * MAKE SURE TO ADD THEM IN THE END AND CHOOSE SENSIBLE DEFAULTS SUCH THAT WE
   * CAN RUN THE RESULTING BINARY WITHOUT HAVING TO SUPPLY MORE PARAMETERS THAN
   * SPECIFIED IN THE UNMODIFIED FILE (cache_size, cache_mapping and cache_org)
   */
  if (argc != 5) { /* argc should be 2 for correct execution */
    printf(
        "Usage: ./cache_sim [cache size: 128-4096] [cache mapping: dm|fa] "
        "[cache organization: uc|sc]\n");
    exit(0);
  } else {
    /* argv[0] is program name, parameters start with argv[1] */

    /* Set cache size */
    cache_size = atoi(argv[1]);

    /* Set Cache Mapping */
    // index bits are initialised here, as they are dependent on the cache mapping
    if (strcmp(argv[2], "dm") == 0) {
      cache_mapping = dm;
      index_bits = log2(cache_size/block_size);
    } else if (strcmp(argv[2], "fa") == 0) {
      cache_mapping = fa;
      index_bits = 0;
    } else {
      printf("Unknown cache mapping\n");
      exit(0);
    }
    // Tag bits can be set here, as they are independent of the cache mapping
    tag_bits = 32 - offset_bits - index_bits;

   

    // Initialize caches
    // If split cache, two caches are initialized
    if (strcmp(argv[3], "uc") == 0) {
      cache_org = uc;
      cache = init_cache(cache_size, cache_mapping, cache_org);

    } else if (strcmp(argv[3], "sc") == 0) {
      cache_org = sc;
      cache = init_cache(cache_size/2, cache_mapping, cache_org);
      instruction_cache = init_cache(cache_size/2, cache_mapping, cache_org);
      // If split and direct mapped, the index bits are reduced by 1, and the tag bits are increased by 1
      if (cache_mapping == dm) {
        index_bits -= 1;
        tag_bits += 1;
      }
      
      // printf("Tag bits: %d\n", index_bits);
    } else {
      printf("Unknown cache organization\n");
      exit(0);
    }
  }

  /* Open the file mem_trace.txt to read memory accesses */
  file_name = argv[4];
  FILE* ptr_file;
  ptr_file = fopen(file_name, "r");
  if (!ptr_file) {
    printf("Unable to open the trace file\n");
    exit(1);
  }

  // printf("Tag bits: %d\n", tag_bits);
  // printf("Index bits: %d\n", index_bits);
  // printf("Offset bits: %d\n", offset_bits);

  /* Loop until whole trace file has been read */
  mem_access_t access;
  while (1) {
    access = read_transaction(ptr_file);
    // If no transactions left, break out of loop
    if (access.address == 0) break;
    
    // FA + UC
    if (cache_mapping == fa && cache_org == uc) {
      fa_access(cache, access, dch_pointer);

      // Moving the cache_head pointer is done outside of the fa_access function 
      // to avoid re-checking which cache is being used every access
      *dch_pointer = (*dch_pointer + 1) % (cache_size/block_size);

    } 
    
    // DM + UC
    else if (cache_mapping == dm && cache_org == uc){
      dm_access(cache, access);
    } 
    
    // FA + SC
    else if (cache_mapping == fa && cache_org == sc) {
      if (access.accesstype == instruction) {
        fa_access(instruction_cache, access, ich_pointer);
        *ich_pointer = (*ich_pointer + 1) % (cache_size/(2*block_size));
      } else {
        fa_access(cache, access, dch_pointer);
        *dch_pointer = (*dch_pointer + 1) % (cache_size/(2*block_size));
      }
    } 
    
    // DM + SC
    else {
      if (access.accesstype == instruction) {
        dm_access(instruction_cache, access);
      } else {
        dm_access(cache, access);
      }
    }
  }

  /* Print the statistics */
  // DO NOT CHANGE THE FOLLOWING LINES!
  printf("\nCache Statistics\n");
  printf("-----------------\n\n");
  printf("Accesses: %ld\n", cache_statistics.accesses);
  printf("Hits:     %ld\n", cache_statistics.hits);
  printf("Misses:   %ld\n", cache_statistics.misses);
  printf("Evictions:%ld\n", cache_statistics.evictions);
  printf("Hit Rate: %.4f\n",
         (double)cache_statistics.hits / cache_statistics.accesses);
  // DO NOT CHANGE UNTIL HERE
  // You can extend the memory statistic printing if you like!

  /* Close the trace file */
  fclose(ptr_file);
  return 1;
}
