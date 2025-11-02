#include "csapp.h"

/**
 * main - Echo 클라이언트 메인 함수
 * @argc: 명령행 인자 개수
 * @argv: 명령행 인자 배열 (argv[1] = 호스트명, argv[2] = 포트 번호)
 *
 * Echo 서버에 연결하여 사용자가 입력한 텍스트를 서버로 전송하고,
 * 서버로부터 되돌아온 echo 메시지를 화면에 출력한다.
 */
int main(int arc, char **argv){
    int clientfd;                       // 서버와 연결된 소켓 파일 디스크립터
    char *host, *port, buf[MAXLINE];    // 호스트명, 포트, 버퍼
    rio_t rio;                          // Robust I/O 구조체

    // 명령행 인자 검사
    if(arc != 3){
        fprintf(stderr, "usage:%s <host> <port>\n", argv[0]);
    }

    // 명령행 인자로부터 host와 port를 입력받음
    host = argv[1];
    port = argv[2];

    // 서버에 연결
    clientfd = Open_clientfd(host, port);

    // Rio(Robust I/O) 버퍼 초기화
    Rio_readinitb(&rio, clientfd);

    // 사용자 입력을 받아서 서버로 전송하고, 서버의 응답을 출력
    // EOF(Ctrl+D)가 입력될 때까지 계속 반복
    while(Fgets(buf, MAXLINE, stdin) != NULL){
        Rio_writen(clientfd, buf, strlen(buf));  // 서버로 전송
        rio_readlineb(&rio, buf, MAXLINE);       // 서버로부터 응답 받기
        Fputs(buf, stdout);                       // 화면에 출력
    }

    // 연결 종료
    Close(clientfd);
    exit(0);
}