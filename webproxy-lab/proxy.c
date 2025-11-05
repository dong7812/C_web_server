#include <stdio.h>
#include "csapp.h"
#include "util/log.h"
#include "cache2.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";
  
static void *thread_handler(void *vargp);

// echoserver
int main(int argc, char **argv)
{
  int listendfd, connfd; // 듣기 소켓, 연결 소켓
  socklen_t clientlen;   // 클라이언트 주소 구조체 크기

  struct sockaddr_storage cliendaddr;                  // 클라이언트 주소 정보
  char client_hostname[MAXLINE], client_port[MAXLINE]; // 클라이언트 호스트명, 포트

  // 명령행 인자 검사
  if (argc != 2)
  {
    fprintf(stderr, "usage : %s <port>\n", argv[0]);
    exit(0);
  }

  // LRU 캐시 초기화
  lru_cache_init();

  // 듣기 소켓 생성 (지정된 포트에서 연결 요청 대기)
  listendfd = Open_listenfd(argv[1]);

  // 무한 루프: 클라이언트 연결 요청을 계속 처리
  while (1)
  {
    // clientlen = sizeof(struct sockaddr_storage);

    // // 클라이언트 연결 수락
    // connfd = Accept(listendfd, (SA *)&cliendaddr, &clientlen);

    // // 클라이언트의 호스트명과 포트 번호 얻기
    // Getnameinfo((SA *)&cliendaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
    // log_msg("클라이언트 연결: (%s, %s)", client_hostname, client_port);

    // // http 요청 파싱 서비스 제공
    // doit(connfd);

    // // 연결 종료
    // Close(connfd);

    clientlen = sizeof(struct sockaddr_storage);
    int *connfdp = Malloc(sizeof(int));
    *connfdp = Accept(listendfd, (SA *)&cliendaddr, &clientlen);

    pthread_t tid;
    Pthread_create(&tid, NULL, thread_handler, connfdp);
  }

  exit(0);
}

void doit(int fd)
{
  rio_t client_rio, server_rio;
  char filename[MAXLINE], cgiargs[MAXLINE];
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char hostname[MAXLINE], path[MAXLINE], port[MAXLINE];
  int n;

  // rio 초기화
  // 여기까지는 client한테 받아서 변환
  Rio_readinitb(&client_rio, fd);
  // 요청 라인 읽기: "GET /home.html HTTP/1.1"
  rio_readlineb(&client_rio, buf, MAXLINE);

  sscanf(buf, "%s %s %s", method, uri, version);
  log_msg("요청: %s %s %s", method, uri, version);

  // GET, HEAD 메소드만 지원
  if (strcasecmp(method, "GET") && strcasecmp(method, "HEAD"))
  {
    clienterror(fd, method, "501", "Not Implemented",
                "Tiny does not implement this method");
    return;
  }

  parse_url(uri, hostname, port, path);
  log_msg("서버 연결 시도: %s:%s%s", hostname, port, path);

  // LRU 캐시 확인
  Node* cached_node = lru_cache_get(global_cache, uri);
  if (cached_node != NULL)
  {
    // 캐시 HIT! 바로 응답
    Rio_writen(fd, cached_node->content, cached_node->content_size);
    return;
  }

  // 실제 server로 전송
  int serverfd = Open_clientfd(hostname, port);
  log_msg("서버 연결 성공: %s:%s", hostname, port);
  Rio_readinitb(&server_rio, serverfd);

  // 원격 서버에 HTTP 요청 전달
  sprintf(buf, "GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n",
          path, hostname);
  Rio_writen(serverfd, buf, strlen(buf));

  // 클라이언트 헤더를 읽으면서 서버로 전달
  Rio_readlineb(&client_rio, buf, MAXLINE);
  while (strcmp(buf, "\r\n"))
  {
    Rio_writen(serverfd, buf, strlen(buf));
    log_msg("전송2: %s", buf);
    Rio_readlineb(&client_rio, buf, MAXLINE);
  }
  Rio_writen(serverfd, "\r\n", 2);
  log_msg("요청 헤더 전달 완료");

  // 서버 응답을 클라이언트에게 전달하면서 캐시에 저장할 데이터 누적
  // 스택 오버플로우 방지를 위해 힙에 할당
  char *response_buf = (char *)Malloc(MAX_OBJECT_SIZE);
  int total_size = 0;

  while ((n = Rio_readnb(&server_rio, buf, MAXLINE)) > 0)
  {
    Rio_writen(fd, buf, n);

    // 캐시 크기 제한 체크하며 누적
    if (total_size + n <= MAX_OBJECT_SIZE)
    {
      memcpy(response_buf + total_size, buf, n);
      total_size += n;
    }
    else
    {
      // 크기 초과하면 캐시 저장 포기
      total_size = MAX_OBJECT_SIZE + 1;
    }
  }
  log_msg("응답 전달 완료");

  Close(serverfd);

  // 크기가 적절하면 LRU 캐시에 저장
  if (total_size > 0 && total_size <= MAX_OBJECT_SIZE)
  {
    lru_cache_put(global_cache, uri, response_buf, total_size);
  }

  // 메모리 해제
  Free(response_buf);

  log_msg("요청 처리 완료: %s:%s%s", hostname, port, path);
}

void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else if (strstr(filename, ".mpg") || strstr(filename, ".mpeg"))
    strcpy(filetype, "video/mpeg");
  else if (strstr(filename, ".mp4"))
    strcpy(filetype, "video/mp4");
  else
    strcpy(filetype, "text/plain");
}

void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  /* Build the HTTP response body */
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor="
                "ffffff"
                ">\r\n",
          body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  /* Print the HTTP response */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}

void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  printf("%s", buf);
  while (strcmp(buf, "\r\n"))
  {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

void parse_url(char *url, char *hostname, char *port, char *path)
{
  // url parsing 하기
  // 1. http:// 건너띄기
  // 2. 포트 번호 찾기 :뒤에 번호
  // 3. 그러면 앞은 자동으로 host
  char *hostbegin, *hostend, *pathbegin, *portbegin;
  // "http://" 건너뛰기
  hostbegin = strstr(url, "//");
  if (hostbegin)
  {
    // "//" 건너뛰기
    hostbegin += 2;
  }
  else
  {
    hostbegin = url;
  }

  // 포트 번호 찾기 (: 기호)
  portbegin = strchr(hostbegin, ':');
  pathbegin = strchr(hostbegin, '/');

  if (portbegin && (!pathbegin || portbegin < pathbegin))
  {
    // 포트가 명시됨
    strncpy(hostname, hostbegin, portbegin - hostbegin);
    hostname[portbegin - hostbegin] = '\0';

    hostend = pathbegin ? pathbegin : (portbegin + strlen(portbegin));
    strncpy(port, portbegin + 1, hostend - portbegin - 1);
    port[hostend - portbegin - 1] = '\0';
  }
  else
  {
    // 포트가 없음: 기본 포트 80
    hostend = pathbegin ? pathbegin : (hostbegin + strlen(hostbegin));
    strncpy(hostname, hostbegin, hostend - hostbegin);
    hostname[hostend - hostbegin] = '\0';
    // 기본 포트
    strcpy(port, "80");
  }

  // 경로 파싱
  if (pathbegin)
  {
    strcpy(path, pathbegin);
  }
  else
  {
    strcpy(path, "/"); // 기본 경로
  }
}

/*
 * parse_uri - parse URI into filename and CGI args
 *             return 0 if dynamic content, 1 if static
 */
int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;

  if (!strstr(uri, "cgi-bin"))
  { /* Static content */
    strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    if (uri[strlen(uri) - 1] == '/')
      strcat(filename, "home.html");
    return 1;
  }
  else
  {                         /* Dynamic content */
    ptr = strchr(uri, '?'); // index() 대신 strchr() 사용
    if (ptr)
    {
      strcpy(cgiargs, ptr + 1);
      *ptr = '\0';
    }
    else
      strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
}

/*
 * serve_static - copy a file back to the client
 */
void serve_static(int fd, char *filename, int filesize, char *method)
{
  int srcfd;
  char *srcp, filetype[MAXLINE];

  char buf[MAXBUF];
  char *p = buf;
  int n;
  int remaining = sizeof(buf);

  /* Send response headers to client */
  get_filetype(filename, filetype);

  /* Build the HTTP response headers correctly - use separate buffers or append */
  n = snprintf(p, remaining, "HTTP/1.0 200 OK\r\n");
  p += n;
  remaining -= n;

  n = snprintf(p, remaining, "Server: Tiny Web Server\r\n");
  p += n;
  remaining -= n;

  n = snprintf(p, remaining, "Connection: close\r\n");
  p += n;
  remaining -= n;

  n = snprintf(p, remaining, "Content-length: %d\r\n", filesize);
  p += n;
  remaining -= n;

  n = snprintf(p, remaining, "Content-type: %s\r\n\r\n", filetype);
  p += n;
  remaining -= n;

  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers:\n");
  printf("%s", buf);

  // http head 메소드 지원
  if (strcasecmp(method, "HEAD") != 0)
  {
    srcfd = Open(filename, O_RDONLY, 0);

    srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    Close(srcfd);
    Rio_writen(fd, srcp, filesize);
    Munmap(srcp, filesize);
  }
}

/*
 * serve_dynamic - run a CGI program on behalf of the client
 */
void serve_dynamic(int fd, char *filename, char *cgiargs, char *method)
{
  char buf[MAXLINE], *emptylist[] = {NULL};

  /* Return first part of HTTP response */
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  // HEAD 요청이면 헤더만 보내고 종료
  if (strcasecmp(method, "HEAD") == 0)
  {
    sprintf(buf, "\r\n");
    Rio_writen(fd, buf, strlen(buf));
    return;
  }

  if (Fork() == 0)
  {
    /* Child */
    setenv("QUERY_STRING", cgiargs, 1);
    Dup2(fd, STDOUT_FILENO);              /* Redirect stdout to client */
    Execve(filename, emptylist, environ); /* Run CGI program */
  }
  Wait(NULL); /* Parent waits for and reaps child */
}

void *thread_handler(void *vargp)
{
  int connfd = *((int *)vargp);
  Pthread_detach(Pthread_self());
  Free(vargp);
  doit(connfd);
  Close(connfd);
  return NULL;
}
