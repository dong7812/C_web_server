#include <stdio.h>
#include "csapp.h"
#include "util/log.h"
#include "cache.h"

cache_entry cache[MAX_CACHE_ENTRIES];
int cache_count = 0;

pthread_mutex_t cache_mutex;

void init_cache(){
    pthread_mutex_init(&cache_mutex, NULL);
    for(int i= 0; i < MAX_CACHE_ENTRIES; i++){
        cache[i].valid = 0;
    }
    cache_count = 0;
}

void add_cache(char *url, char *content, int size){
    if (size > MAX_OBJECT_SIZE) return;

    pthread_mutex_lock(&cache_mutex);

    int idx = cache_count % MAX_CACHE_ENTRIES;

    strcpy(cache[idx].url, url);
    memcpy(cache[idx].content, content, size);
    cache[idx].size = size;
    cache[idx].valid = 1;
    
    cache_count++;
    pthread_mutex_unlock(&cache_mutex);
    log_msg("캐시 추가: %s (슬롯 %d)", url, idx);
}

int find_cache(char *url){
    pthread_mutex_lock(&cache_mutex);
    int limit = (cache_count < MAX_CACHE_ENTRIES) ? cache_count : MAX_CACHE_ENTRIES;
    for(int i = 0; i < limit; i++){
        if(cache[i].valid == 1 && strcmp(cache[i].url, url) == 0){
            log_msg("캐시 hit: %s", url);
            pthread_mutex_unlock(&cache_mutex);
            return i;
        }
    }
    log_msg("캐시 miss: %s", url);
    pthread_mutex_unlock(&cache_mutex);
    return -1;
}

char* get_cache_content(int idx, int *size){
    pthread_mutex_lock(&cache_mutex);
    if(idx >= 0 && idx < MAX_CACHE_ENTRIES && cache[idx].valid){
        *size = cache[idx].size;
        pthread_mutex_unlock(&cache_mutex);
        return cache[idx].content;
    }
    pthread_mutex_unlock(&cache_mutex);
    return NULL;
}