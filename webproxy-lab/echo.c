#include "csapp.h"

/**
 * echo - 클라이언트로부터 받은 데이터를 그대로 되돌려주는 함수
 * @connfd: 클라이언트와 연결된 소켓 파일 디스크립터
 *
 * 클라이언트가 보낸 텍스트 라인을 읽어서 그대로 다시 클라이언트에게 전송한다.
 * 클라이언트가 연결을 끊을 때까지 반복적으로 echo를 수행한다.
 */
void echo(int connfd){
    size_t n;
    char buf[MAXLINE];      // 데이터를 저장할 버퍼
    rio_t rio;              // Robust I/O 구조체

    // Rio(Robust I/O) 버퍼 초기화
    Rio_readinitb(&rio, connfd);

    // 클라이언트로부터 한 줄씩 읽어서 echo
    while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0){
        printf("server received %d bytes\n", (int)n);
        Rio_writen(connfd, buf, n);  // 받은 데이터를 그대로 클라이언트에게 전송
    }
}