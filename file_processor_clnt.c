#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdlib.h>
#include <time.h>

#define FILE 0
#define WRITE 1
#define READ 2

void write_to_FIFO(int fd[], char *line)
{
    char cd;
    int i = 0;
    int lineCount = 0;
    while (read(fd[FILE], &cd, 1) > 0)
    {
        line[i++] = cd;
        if (cd == '\n')
        {
            line[i] = '\0';
            lineCount++;
            printf("%d번째 줄 전송...\n", lineCount);
            write(fd[WRITE], line, strlen(line) + 1);

            char response[1024];
            int len = read(fd[READ], response, sizeof(response));
            if (len > 0)
            {
                response[len] = '\0';
                printf("%s", response);
            }

            i = 0;
            sleep(1);
        }
    }

    // 마지막 줄 개행 없을떄
    if (i > 0)
    {
        line[i] = '\0';
        lineCount++;
        printf("[%d번째 줄 전송]\n", lineCount);
        write(fd[WRITE], line, strlen(line) + 1);

        char response[1024];
        int len = read(fd[READ], response, sizeof(response));
        if (len > 0)
        {
            response[len] = '\0';
            printf("→ 서버 응답: %s", response);
        }
    }

    printf("\n모든 줄 전송 완료 END 메시지 보냄\n");
    write(fd[WRITE], "END", 4);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "사용법: %s <input_file> <mode>\n", argv[0]);
        exit(1);
    }

    int fd[3];
    char line[1024];

    fd[FILE] = open(argv[1], O_RDONLY);
    if (fd[FILE] < 0)
    {
        perror("파일 열기 실패");
        exit(1);
    }

    fd[WRITE] = open("to_server", O_WRONLY);
    fd[READ] = open("to_client", O_RDONLY);

    char modebuf[64];
    strncpy(modebuf, argv[2], sizeof(modebuf)-1);
    modebuf[sizeof(modebuf)-1] = '\0';
    modebuf[strcspn(modebuf, "\r\n")] = '\0';
    write(fd[WRITE], modebuf, strlen(modebuf) + 1);
    sleep(1);

    clock_t start = clock();
    write_to_FIFO(fd, line);

    // 서버에서 END랑 모드 라인  시간 받으면 통계 출력해줌
    char endMsg[128];
    int len = read(fd[READ], endMsg, sizeof(endMsg));
    if (len > 0)
    {
        endMsg[len] = '\0';
        if (strncmp(endMsg, "END|", 4) == 0)
        {
            char recvMode[50];
            int recvLineCount;
            double recvTime;
            sscanf(endMsg, "END|%49[^|]|%d|%lf", recvMode, &recvLineCount, &recvTime);

            printf("\n=== 처리 통계 ===\n");
            printf("  처리 모드: %s\n", recvMode);
            printf("  처리한 줄 수: %d줄\n", recvLineCount);
            printf("  소요 시간: %.2f초\n", recvTime);
        }
        else
        {
            printf("\nEND수신 싳패\n");
        }
    }

    close(fd[FILE]);
    close(fd[WRITE]);
    close(fd[READ]);
    return 0;
}
