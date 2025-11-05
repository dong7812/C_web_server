#ifndef CACHE_H
#define CACHE_H

#define MAX_CACHE_ENTRIES 10
#define MAX_OBJECT_SIZE 102400

#include <pthread.h>
extern pthread_mutex_t cache_mutex;
typedef struct{
    char url[MAXLINE];
    char content[MAX_OBJECT_SIZE];
    int size;
    int valid;
} cache_entry;
/* cache 초기화 */
void init_cache();
/* cache 스택 쌓기 */
void add_cache(char *url, char *content, int size);
/* cache 찾기 */
int find_cache(char *url);
/* cache 내용 가져오기 */
char* get_cache_content(int idx, int *size);
#endif
