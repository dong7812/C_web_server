CC = gcc
CFLAGS = -O0 -Wall -I . -g
LIB = -lpthread

# 기본 포트 설정 (변경 가능: make run PORT=9999)
PORT ?= 8080

# 기본 타겟: tiny 서버와 CGI 프로그램들 빌드
all: tiny cgi

# Tiny 웹 서버 빌드
tiny: tiny.c csapp.o
	$(CC) $(CFLAGS) -o tiny tiny.c csapp.o $(LIB)

# csapp 라이브러리 컴파일
csapp.o: csapp.c
	$(CC) $(CFLAGS) -c csapp.c

# CGI 프로그램들 빌드
cgi:
	(cd cgi-bin; make)

# 서버 실행 (포트 지정 가능, 기본값 8080)
run: tiny
	./tiny $(PORT)

# 빌드 파일 정리
clean:
	rm -f *.o tiny *~
	(cd cgi-bin; make clean)

