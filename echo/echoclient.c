#include "csapp.h"

// argc: argument count
// argv: argument vector - 입력받은 인자의 배열
int main(int argc, char **argv){
    int clientfd; // client file descriptor
    char *host, *port, buf[MAXLINE];
    rio_t rio; // rio: robust I/O - 새로 작성한 I/O 라이브러리

    if(argc != 3){ // 인자가 3개가 아니면 에러 메시지 출력
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]); // stderr: standard error
        exit(0); // 프로그램 종료
    }
    host = argv[1]; // host: 첫번째 인자
    port = argv[2]; // port: 두번째 인자

    // Open_clientfd 함수를 호출하여 서버와 연결하고 리턴받은 소켓 식별자를 clientfd에 저장
    clientfd = Open_clientfd(host, port); // Open_clientfd: clientfd를 반환하는 함수
    Rio_readinitb(&rio, clientfd); // rio를 초기화하고, rio를 사용해 clientfd를 읽는다.

    while (Fgets(buf, MAXLINE, stdin) != NULL){ // 받은 입력을 읽어 buf에 저장한다. 입력이 없으면 NULL을 반환하며 종료한다.
        Rio_writen(clientfd, buf, strlen(buf)); // clientfd에 buf의 데이터를 서버에 전송한다.
        Rio_readlineb(&rio, buf, MAXLINE); // rio 구조체를 통해 파일 디스크립터 clientfd로부터 한 줄을 읽어 buf에 저장, 최대 MAXLINE-1 바이트까지 읽는다.
        Fputs(buf, stdout); // buf에 저장된 문자열을 표준 출력 stdout에 출력
    }
    Close(clientfd); // clientfd를 닫아 클라이언트 연결을 종료하고 사용한 자원을 반환한다.
    exit(0); // 프로그램 종료
}