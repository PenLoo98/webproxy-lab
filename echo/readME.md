# 파일 경로 이동 
cd echoclient.c echoserver.c로 이동

# 파일 생성
make 

# 서버 열기 
./echoserver {포트번호}
예시: ./echoserver 1234

./echoserver {포트번호} &
여기서 &는 백그라운드 실행을 의미한다.

# 클라이언트 접속하기 
./echoclient {호스트명} {포트번호}
예시: ./echoclient localhost 1234