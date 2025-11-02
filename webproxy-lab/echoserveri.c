#include "csapp.h"

void echo(int connfd);

/**
 * main - 반복적(iterative) Echo 서버 메인 함수
 * @argc: 명령행 인자 개수
 * @argv: 명령행 인자 배열 (argv[1] = 포트 번호)
 *
 * 지정된 포트에서 클라이언트 연결을 기다리고,
 * 연결이 들어오면 echo 서비스를 제공한다.
 * 한 번에 하나의 클라이언트만 처리하는 반복적 서버이다.
 */
int main(int argc, char **argv){
    int listendfd, connfd;              // 듣기 소켓, 연결 소켓
    socklen_t clientlen;                // 클라이언트 주소 구조체 크기

    struct sockaddr_storage cliendaddr; // 클라이언트 주소 정보
    char client_hostname[MAXLINE], client_port[MAXLINE];  // 클라이언트 호스트명, 포트

    // 명령행 인자 검사
    if(argc != 2){
        fprintf(stderr, "usage : %s <port>\n", argv[0]);
        exit(0);
    }

    // 듣기 소켓 생성 (지정된 포트에서 연결 요청 대기)
    listendfd = Open_listenfd(argv[1]);

    // 무한 루프: 클라이언트 연결 요청을 계속 처리
    while(1){
        clientlen = sizeof(struct sockaddr_storage);

        // 클라이언트 연결 수락
        connfd = Accept(listendfd, (SA *)&cliendaddr, &clientlen);

        // 클라이언트의 호스트명과 포트 번호 얻기
        Getnameinfo((SA *)&cliendaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        printf("connected to (%s, %s) \n", client_hostname, client_port);

        // Echo 서비스 제공
        echo(connfd);

        // 연결 종료
        Close(connfd);
    }

    exit(0);
}