#include <stdio.h>
#include <stdarg.h>
#include "log.h"

/**
 * log_msg - 로그 메시지를 출력하는 유틸리티 함수
 * @format: printf 형식의 포맷 문자열
 * @...: 가변 인자
 *
 * 사용 예:
 *   log_msg("서버 연결: %s:%s", hostname, port);
 *   log_msg("요청 처리 완료");
 */
void log_msg(const char *format, ...) {
    va_list args;
    printf("[이동규] ");
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
    fflush(stdout);
}
