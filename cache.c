#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "net.h"
#include "net.h"
#include "cache.h"

static cache_entry_t *cache = NULL;
static int cache_size = 0;
static int clock = 0;
static int num_queries = 0;
static int num_hits = 0;

int cache_create(int num_entries) {
  //only runs if cache is null (not created)
  if(cache == NULL){
    //ensures cache being created is proper size
    if(num_entries < 2 || num_entries > 4096){
      return -1;
    }
    //creates cache
    cache = calloc(num_entries, sizeof(cache_entry_t));
    cache_size = num_entries;
    return 1;
  }else{
    return -1;
  }
}

int cache_destroy(void) {
  //only destroys cache if it is not already "destroyed"
  if(cache == NULL){
    return -1;
  }else{
    free(cache);
    cache = NULL;
    cache_size = 0;
    return 1;
  }
}

int cache_lookup(int disk_num, int block_num, uint8_t *buf) {
  //increments num_queries each time a lookup is attempted
  num_queries++;

  if(buf == NULL || cache == NULL){
    return -1;
  }

  //loops through the cache, and checks each valid cache
  //then, if a cache with a matching disk and block is found, it is copied to the buf
  //and its clock value is updated
  for(int i=0; i<cache_size;++i){
    if(cache[i].valid){
    if (cache[i].disk_num == disk_num && cache[i].block_num == block_num) {
      num_hits++;
      memcpy(buf, cache[i].block, JBOD_BLOCK_SIZE);
      clock++;
      cache[i].access_time=clock;
      return 1;
    }
}
  }
  return -1;

}

void cache_update(int disk_num, int block_num, const uint8_t *buf) {
  if (cache == NULL || buf == NULL) {
    return;
  }

  //loops through the cache, in order to find the cache that is being updated, then updates that 
  //cache and its clock value is updated
  for (int i = 0; i < cache_size; i++) {
    if (cache[i].valid){
      if(cache[i].disk_num == disk_num && cache[i].block_num == block_num) {
      memcpy(cache[i].block, buf, JBOD_BLOCK_SIZE);
      clock++;
      cache[i].access_time = clock;
      cache[i].valid = true;
      return;
    }}
  }
}

int cache_insert(int disk_num, int block_num, const uint8_t *buf) {
  if(buf == NULL || cache == NULL){
    return -1;
  }

  //ensures the cache being inserted is of proper size
  if(disk_num < 0 || disk_num > 15 || block_num < 0 || block_num > 255){
    return -1;
  }

  //loops through the possible caches to ensure that it is not already in 
  for (int i = 1; i < cache_size; i++) {
    if (cache[i].valid && cache[i].disk_num == disk_num && cache[i].block_num == block_num) {
      return -1;
    }}

//now , it loops through until the first empty cache is found
//and it updates this cache with the data and information of what is being inserted (block num, buf, etc.)
//then it updates the access time
for (int i = 1; i < cache_size; i++) {
    if (!(cache[i].valid)) {
      cache[i].block_num = block_num;
      cache[i].disk_num = disk_num;
      memcpy(cache[i].block, buf, JBOD_BLOCK_SIZE);
      clock++;
      cache[i].access_time = clock;
      cache[i].valid = true;
      return 1;
    }}

//if there are no available caches, it uses least recent used in order to insert/replace a current cache
//intializes variables in order to store the access time of the first cache
int lru_cache = 0;
int lru_access_time = cache[0].access_time;

//loops through each cache, updating the lru_cache, which is basically the min function
//at end of loop, it lru_cache will hold the index for the least recently used cache
for (int i = 1; i < cache_size; i++) {
    if (cache[i].access_time < lru_access_time) {
      lru_access_time = cache[i].access_time;
      lru_cache = i;
    }
  }

  //updates the lru_cache with the information of the cache being inserted
  cache[lru_cache].block_num = block_num;
  cache[lru_cache].disk_num = disk_num;
  memcpy(cache[lru_cache].block, buf, JBOD_BLOCK_SIZE);
  clock++;
  cache[lru_cache].access_time = clock;  // Update clock_accesses to the current clock value
  cache[lru_cache].valid = true;
  return 1;

}
  

bool cache_enabled(void) {
  //ensures cache has a size larger than 2
  if (cache_size > 2){
    return true;
  }
    return false;
}

void cache_print_hit_rate(void) {
  fprintf(stderr, "Hit rate: %5.1f%%\n", 100 * (float) num_hits / num_queries);
}