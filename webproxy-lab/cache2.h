#ifndef CACHE2_H
#define CACHE2_H

#include "csapp.h"
#include <pthread.h>
#include <time.h>

#define HASH_SIZE 255
#define MAX_OBJECT_SIZE 102400
#define MAX_CACHE_SIZE 1049000

extern pthread_mutex_t cache_mutex;

// double linked list를 위한 node
typedef struct Node{
    char url[MAXLINE];                  // key: URL
    char content[MAX_OBJECT_SIZE];      // value: 실제 응답 데이터
    int content_size;                   // content 크기
    struct Node *prev;
    struct Node *next;
} Node;

// hashmap 선언
typedef struct {
    Node* table[HASH_SIZE];
} HashMap;

// LRU Cache 구조체
/*
capacity : 최대 저장 개수
size : 현재 저장된 개수
map : key로 node를 찾기 위한 hashmap
head / tail : 연결 리스트의 양 끝 (head = 가장 최근, tail = 가장 오래됨)
*/
typedef struct {
    int capacity;
    int size;
    HashMap* map;
    Node* head;    // 가장 최근에 사용된 항목
    Node* tail;    // 가장 오래 전에 사용된 항목 (제거 대상)
} LRUCache;

// 전역 LRU 캐시 인스턴스
extern LRUCache* global_cache;

// LRU Cache 함수들
LRUCache* lru_cache_create(int capacity);
void lru_cache_init();
Node* lru_cache_get(LRUCache* cache, char* url);
void lru_cache_put(LRUCache* cache, char* url, char* content, int content_size);
void lru_cache_free(LRUCache* cache);

// 내부 헬퍼 함수
unsigned long hash_function(char* str);
Node* create_node(char* url, char* content, int content_size);
void remove_node(LRUCache* cache, Node* node);
void move_to_head(LRUCache* cache, Node* node);
void add_to_head(LRUCache* cache, Node* node);
void remove_tail(LRUCache* cache);

#endif
