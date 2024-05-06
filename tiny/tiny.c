/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

// argc: argument count, argv: argument vector
int main(int argc, char **argv) {
  int listenfd, connfd; // listenfd: 서버 소켓 식별자, connfd: 클라이언트 소켓 식별자
  char hostname[MAXLINE], port[MAXLINE]; // 호스트 이름과 포트 번호를 저장할 버퍼
  socklen_t clientlen; // 클라이언트 주소 길이
  struct sockaddr_storage clientaddr; // 클라이언트 주소를 저장할 구조체

  /* Check command line args */
  // 인자가 2개가 아니면 에러 메시지 출력
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]); // 서버 소켓을 열고 리턴받은 소켓 식별자를 listenfd에 저장
  while (1) { // 무한 루프 - 서버가 종료될 때까지 계속 실행
    clientlen = sizeof(clientaddr); // 클라이언트 주소 길이
    // 클라이언트로부터 연결을 수락하고 리턴받은 소켓 식별자를 connfd에 저장
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen);  
    // 클라이언트의 주소를 호스트 이름과 포트 번호로 변환하여 출력                    
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);   
    Close(connfd);  
  }
}

void doit(int fd){
  int is_static; // 동적 컨텐츠인지 정적 컨텐츠인지 확인
  struct stat sbuf; // 버퍼의 상태를 저장할 구조체
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE]; // 파일 이름과 cgi 인자를 저장할 버퍼
  rio_t rio; // 버퍼를 저장할 구조체

  /* Read request line and headers */
  Rio_readinitb(&rio, fd); // 버퍼 초기화
  Rio_readlineb(&rio, buf, MAXLINE); // 버퍼에서 한 줄 읽기
  printf("Request headers:\n"); // 요청 헤더 출력
  printf("%s", buf); // 버퍼 출력
  sscanf(buf, "%s %s %s", method, uri, version);  // 버퍼에서 메소드, uri, 버전을 읽기

  // 메소드가 GET이 아닌 경우 에러 출력
  if (strcasecmp(method, "GET")) {  // strcasecmp: 대소문자 구분 없이 문자열 비교 같으면 0, 다르면 0이 아닌 값 리턴
    clienterror(fd, method, "501", "Not Implemented",
                "Tiny does not implement this method");
    return;
  } 
  read_requesthdrs(&rio); // request의 헤더 읽기

  /* Parse URI from GET request */
  is_static = parse_uri(uri, filename, cgiargs);  // uri를 파싱하여 정적 컨텐츠인지 동적 컨텐츠인지 확인

  // 파일이 존재하지 않는 경우 에러 출력
  if (stat(filename, &sbuf) < 0) {  
    clienterror(fd, filename, "404", "Not found",
                "Tiny couldn't find this file");
    return;
  }  

  // 정적 컨텐츠인 경우
  if (is_static) {  
    // 파일 읽기 권한이 없는 경우 에러 출력
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn't read the file");
      return;
    }
    serve_static(fd, filename, sbuf.st_size);
  }
  // 동적 컨텐츠인 경우 
  else {
    // 파일이 실행이 불가능한 경우 에러 출력
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn't run the CGI program");
      return;
    }
    serve_dynamic(fd, filename, cgiargs);
  }
}

// 클라이언트에게 에러 메시지를 전송하는 함수
void clienterror(int fd, char *cause, char *errnum,
char *shortmsg, char *longmsg)
{
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
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

// 요청 헤더를 읽는 함수
void read_requesthdrs(rio_t *rp) {
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  while (strcmp(buf, "\r\n")) {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

// uri를 파싱하는 함수
int parse_uri(char *uri, char *filename, char *cgiargs) {
  char *ptr;

  // strstr: uri에서 cgi-bin이 있는지 확인 찾으면 해당 문자열의 포인터 반환, 없으면 NULL 반환
  if (!strstr(uri, "cgi-bin")) {  // uri에 cgi-bin이 없는 경우 -> 정적 컨텐츠
    strcpy(cgiargs, "");  // cgiargs에 빈 문자열 복사
    strcpy(filename, ".");  // filename에 . 복사
    strcat(filename, uri);  // filename에 uri를 이어붙임
    if (uri[strlen(uri)-1] == '/')  // uri의 마지막 문자가 /인 경우
      strcat(filename, "home.html");  // filename에 home.html을 이어붙임
    return 1;  // 정적 컨텐츠
  } 
  else {  // uri에 cgi-bin이 있는 경우 -> 동적 컨텐츠
    ptr = index(uri, '?');  // uri에서 ?의 위치를 찾아 ptr에 저장
    if (ptr) {  // ptr이 NULL이 아닌 경우
      strcpy(cgiargs, ptr+1);  // cgiargs에 ptr+1부터 끝까지 복사
      *ptr = '\0';  // ptr을 NULL로 변경
    } 
    else{
      strcpy(cgiargs, "");  // cgiargs에 빈 문자열 복사
    }
    strcpy(filename, ".");  // filename에 . 복사
    strcat(filename, uri);  // filename에 uri를 이어붙임
    return 0;  // 동적 컨텐츠
  }
}

// 정적 컨텐츠를 제공하는 함수
void serve_static(int fd, char *filename, int filesize){
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  /* Send response headers to client */
  get_filetype(filename, filetype);  // 파일의 타입을 가져옴
  sprintf(buf, "HTTP/1.0 200 OK\r\n");  // 버퍼에 상태 코드 저장
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);  // 버퍼에 서버 정보 저장
  sprintf(buf, "%sConnection: close\r\n", buf);  // 버퍼에 연결 정보 저장
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);  // 버퍼에 파일 크기 저장
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);  // 버퍼에 파일 타입 저장
  Rio_writen(fd, buf, strlen(buf));  // 버퍼를 클라이언트에 전송
  printf("Response headers:\n");
  printf("%s", buf);

  /* Send response body to client */
  srcfd = Open(filename, O_RDONLY, 0);  // 파일을 읽기 전용으로 열고 리턴받은 파일 식별자를 srcfd에 저장
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);  // 파일을 메모리 매핑하고 리턴받은 메모리 주소를 srcp에 저장
  Close(srcfd);  // 파일을 닫음
  Rio_writen(fd, srcp, filesize);  // srcp를 클라이언트에 전송
  Munmap(srcp, filesize);  // 메모리 매핑 해제
}

// 파일의 타입을 가져오는 함수
void get_filetype(char *filename, char *filetype){
  if (strstr(filename, ".html"))  // filename에 .html이 있는 경우
    strcpy(filetype, "text/html");  // filetype에 text/html 복사
  else if (strstr(filename, ".gif"))  // filename에 .gif이 있는 경우
    strcpy(filetype, "image/gif");  // filetype에 image/gif 복사
  else if (strstr(filename, ".png"))  // filename에 .png이 있는 경우
    strcpy(filetype, "image/png");  // filetype에 image/png 복사
  else if (strstr(filename, ".jpg"))  // filename에 .jpg이 있는 경우
    strcpy(filetype, "image/jpeg");  // filetype에 image/jpeg 복사
  else  // 그 외의 경우
    strcpy(filetype, "text/plain");  // filetype에 text/plain 복사
}

void serve_dynamic(int fd, char *filename, char *cgiargs){
  char buf[MAXLINE], *emptylist[] = { NULL };

  /* Return first part of HTTP response */
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  if (Fork() == 0) { /* 새로운 자식 프로세스 생성 */
    /* Real server would set all CGI vars here */
    setenv("QUERY_STRING", cgiargs, 1);  /* 환경 변수를 cgiargs로 설정 */
    Dup2(fd, STDOUT_FILENO);         /* Redirect stdout to client */ /*자식의 표준 출력을 연결 파일 식별자로 재지정*/
    Execve(filename, emptylist, environ); /*CGI 프로그램 실행 */
  }
  // 자식 프로세스가 종료될 때까지 기다림
  Wait(NULL); /* Parent waits for and reaps child */
}