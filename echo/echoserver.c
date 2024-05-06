#include "csapp.h"

void echo(int connfd){
    size_t n;
    char buf[MAXLINE];
    rio_t rio;

    Rio_readinitb(&rio, connfd); // rio를 초기화하고, rio를 사용해 connfd를 읽는다.
    while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0){ // connfd로부터 한 줄을 읽어 buf에 저장, 최대 MAXLINE-1 바이트까지 읽는다.
        printf("server received %d bytes\n", (int)n); // 서버가 받은 바이트 수를 출력
        Rio_writen(connfd, buf, n); // connfd에 buf의 데이터를 클라이언트에 전송한다.
    }
}

// argc: argument count
// argv: argument vector - 입력받은 인자의 배열
int main(int argc, char **argv){
    int listenfd, connfd; // listenfd: listen file descriptor, connfd: connection file descriptor
    socklen_t clientlen; // clientlen: 클라이언트 주소 길이
    struct sockaddr_storage clientaddr; // clientaddr: 클라이언트 주소를 저장하는 구조체
    char client_hostname[MAXLINE], client_port[MAXLINE]; // client_hostname: client host name, client_port: client port number

    if(argc != 2){ // 인자가 2개가 아니면 에러 메시지 출력
        fprintf(stderr, "usage: %s <port>\n", argv[0]); // stderr: standard error
        exit(0); // 프로그램 종료
    }

    listenfd = Open_listenfd(argv[1]); // Open_listenfd 함수를 호출하여 listenfd를 반환받는다.
    while(1){ // 무한 루프 - 서버가 종료될 때까지 계속해서 클라이언트의 연결을 기다린다.
        clientlen = sizeof(struct sockaddr_storage); // 클라이언트 주소 길이를 초기화 
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // Accept 함수를 호출하여 클라이언트와 연결하고 connfd를 반환받는다.
        Getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0); // 클라이언트의 호스트 이름과 포트 번호를 가져온다.
        printf("Connected to (%s, %s)\n", client_hostname, client_port); // 연결된 클라이언트의 호스트 이름과 포트 번호를 출력한다.
        echo(connfd); // connfd를 인자로 echo 함수를 호출한다.
        Close(connfd); // connfd를 닫아 클라이언트 연결을 종료하고 사용한 자원을 반환한다.
    }
    exit(0); // 프로그램 종료
}