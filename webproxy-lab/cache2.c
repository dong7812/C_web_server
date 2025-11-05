#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csapp.h"
#include "util/log.h"
#include "cache2.h"

pthread_mutex_t cache_mutex;
LRUCache* global_cache = NULL;

// djb2 해시 함수 - collision이 적은 우수한 해시 함수
unsigned long hash_function(char* str) {
    unsigned long hash = 5381;
    int c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }

    return hash % HASH_SIZE;
}

// 새 노드 생성
Node* create_node(char* url, char* content, int content_size) {
    Node* node = (Node*)Malloc(sizeof(Node));
    strcpy(node->url, url);
    memcpy(node->content, content, content_size);
    node->content_size = content_size;
    node->prev = NULL;
    node->next = NULL;
    return node;
}

// LRU Cache 생성
LRUCache* lru_cache_create(int capacity) {
    LRUCache* cache = (LRUCache*)Malloc(sizeof(LRUCache));
    cache->capacity = capacity;
    cache->size = 0;

    // HashMap 초기화
    cache->map = (HashMap*)Malloc(sizeof(HashMap));
    for (int i = 0; i < HASH_SIZE; i++) {
        cache->map->table[i] = NULL;
    }

    // Dummy head와 tail 노드 생성 (경계 처리 간단화)
    cache->head = (Node*)Malloc(sizeof(Node));
    cache->tail = (Node*)Malloc(sizeof(Node));
    cache->head->next = cache->tail;
    cache->tail->prev = cache->head;
    cache->head->prev = NULL;
    cache->tail->next = NULL;

    return cache;
}

// 전역 캐시 초기화
void lru_cache_init() {
    pthread_mutex_init(&cache_mutex, NULL);
    global_cache = lru_cache_create(10);  // 최대 10개 캐시 항목
    log_msg("LRU 캐시 초기화 완료 (capacity: %d)", global_cache->capacity);
}

// 노드를 리스트에서 제거 (해시맵에서는 제거 안 함)
void remove_node(LRUCache* cache, Node* node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

// 노드를 head 바로 뒤에 추가 (가장 최근 사용)
void add_to_head(LRUCache* cache, Node* node) {
    node->next = cache->head->next;
    node->prev = cache->head;
    cache->head->next->prev = node;
    cache->head->next = node;
}

// 노드를 head로 이동 (최근 사용으로 업데이트)
void move_to_head(LRUCache* cache, Node* node) {
    remove_node(cache, node);
    add_to_head(cache, node);
}

// tail 앞의 노드 제거 (가장 오래된 항목 제거)
void remove_tail(LRUCache* cache) {
    Node* node = cache->tail->prev;
    if (node == cache->head) {
        return;  // 캐시가 비어있음
    }

    // 해시맵에서 제거
    unsigned long hash = hash_function(node->url);
    Node* current = cache->map->table[hash];
    Node* prev_node = NULL;

    while (current != NULL) {
        if (strcmp(current->url, node->url) == 0) {
            if (prev_node == NULL) {
                cache->map->table[hash] = current->next;
            } else {
                prev_node->next = current->next;
            }
            break;
        }
        prev_node = current;
        current = current->next;
    }

    // 리스트에서 제거
    remove_node(cache, node);
    log_msg("LRU: 오래된 항목 제거 - %s", node->url);
    Free(node);
    cache->size--;
}

// URL로 캐시에서 조회 (캐시 히트 시 head로 이동)
Node* lru_cache_get(LRUCache* cache, char* url) {
    pthread_mutex_lock(&cache_mutex);

    unsigned long hash = hash_function(url);
    Node* current = cache->map->table[hash];

    // 해시맵에서 URL 찾기 (체이닝 처리)
    while (current != NULL) {
        if (strcmp(current->url, url) == 0) {
            // 캐시 히트! 최근 사용으로 이동
            move_to_head(cache, current);
            log_msg("LRU 캐시 HIT: %s", url);
            pthread_mutex_unlock(&cache_mutex);
            return current;
        }
        current = current->next;
    }

    // 캐시 미스
    log_msg("LRU 캐시 MISS: %s", url);
    pthread_mutex_unlock(&cache_mutex);
    return NULL;
}

// 캐시에 항목 추가 (이미 있으면 업데이트, 없으면 새로 추가)
void lru_cache_put(LRUCache* cache, char* url, char* content, int content_size) {
    if (content_size > MAX_OBJECT_SIZE) {
        log_msg("LRU: 크기 초과로 캐시 저장 안 함 - %s (%d bytes)", url, content_size);
        return;
    }

    pthread_mutex_lock(&cache_mutex);

    unsigned long hash = hash_function(url);
    Node* current = cache->map->table[hash];
    Node* prev_node = NULL;

    // 이미 존재하는지 확인
    while (current != NULL) {
        if (strcmp(current->url, url) == 0) {
            // 이미 있으면 업데이트
            memcpy(current->content, content, content_size);
            current->content_size = content_size;
            move_to_head(cache, current);
            log_msg("LRU: 기존 항목 업데이트 - %s", url);
            pthread_mutex_unlock(&cache_mutex);
            return;
        }
        prev_node = current;
        current = current->next;
    }

    // 새 노드 생성
    Node* new_node = create_node(url, content, content_size);

    // 해시맵에 추가 (체이닝으로 collision 처리)
    new_node->next = cache->map->table[hash];
    cache->map->table[hash] = new_node;

    // 리스트의 head에 추가
    add_to_head(cache, new_node);
    cache->size++;

    // 용량 초과 시 tail 제거
    if (cache->size > cache->capacity) {
        remove_tail(cache);
    }

    log_msg("LRU: 새 항목 추가 - %s (%d bytes, size: %d/%d)",
            url, content_size, cache->size, cache->capacity);

    pthread_mutex_unlock(&cache_mutex);
}

// 캐시 전체 해제
void lru_cache_free(LRUCache* cache) {
    pthread_mutex_lock(&cache_mutex);

    Node* current = cache->head->next;
    while (current != cache->tail) {
        Node* next = current->next;
        Free(current);
        current = next;
    }

    Free(cache->head);
    Free(cache->tail);
    Free(cache->map);
    Free(cache);

    pthread_mutex_unlock(&cache_mutex);
    pthread_mutex_destroy(&cache_mutex);
}
