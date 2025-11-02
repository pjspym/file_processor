#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#define READ 0
#define WRITE 1

int count_words(const char *line)
{
    int in_word = 0, count = 0;
    for (int i = 0; line[i] != '\0'; i++)
    {
        if (line[i] != ' ' && line[i] != '\n' && line[i] != '\t')
        {
            if (!in_word)
            {
                count++;
                in_word = 1;
            }
        }
        else
        {
            in_word = 0;
        }
    }
    return count;
}

int count_chars(const char *line)
{
    int count = 0;
    for (int i = 0; line[i] != '\0'; i++)
    {
        if (line[i] != ' ' && line[i] != '\n' && line[i] != '\t')
            count++;
    }
    return count;
}

void to_upper(char *line)
{
    for (int i = 0; line[i]; i++)
        line[i] = (char)toupper((unsigned char)line[i]);
}

void to_lower(char *line)
{
    for (int i = 0; line[i]; i++)
        line[i] = (char)tolower((unsigned char)line[i]);
}

void reverse_line(char *line)
{
    int len = strlen(line);
    int has_newline = 0;
    if (len > 0 && line[len - 1] == '\n') {
        has_newline = 1;
        len--;
    }
    if (len <= 0) {
        return;
    }
    for (int i = 0; i < len / 2; i++)
    {
        char tmp = line[i];
        line[i] = line[len - i - 1];
        line[len - i - 1] = tmp;
    }
    if (has_newline) {
        line[len] = '\n';
        line[len+1] = '\0';
    } else {
        line[len] = '\0';
    }
}

int main()
{
    char mode[50];
    char line[1024];
    unlink("to_server");
    unlink("to_client");

    mkfifo("to_server", 0600);
    mkfifo("to_client", 0600);

    int fd[2];
    fd[READ] = open("to_server", O_RDONLY);
    fd[WRITE] = open("to_client", O_WRONLY);

    ssize_t r = read(fd[READ], mode, sizeof(mode)-1);
    if (r <= 0) {
        perror("mode read 실패");
        return 1;
    }
    //끝에 \0넣고 \r\n찾아서 \0으로 바꿈
    mode[r] = '\0';
    mode[strcspn(mode, "\r\n")] = '\0';
    // 앞뒤 공백 제거 (간단한 처리)
    // 왼쪽 공백
    char *mstart = mode;
    while (*mstart == ' ' || *mstart == '\t')
        mstart++;
    if (mstart != mode)
        memmove(mode, mstart, strlen(mstart)+1);
    // 오른쪽 공백
    int ml = strlen(mode);
    while (ml > 0 && (mode[ml-1] == ' ' || mode[ml-1] == '\t'))
    {
        mode[ml-1] = '\0';
        ml--;
    }

    int lineCount = 0;
    clock_t start = clock();

    while (1)
    {
        ssize_t n = read(fd[READ], line, sizeof(line)-1);
        if (n <= 0) {
            break;
        }
        line[n] = '\0';

        char tmp[128];
        strncpy(tmp, line, sizeof(tmp)-1);
        tmp[sizeof(tmp)-1] = '\0';
        tmp[strcspn(tmp, "\r\n")] = '\0';
        if (strcmp(tmp, "END") == 0)
        {
            printf("\nEND 종료\n");
            break;
        }

        lineCount++;
        printf("%d번째 줄 처리 중...\n", lineCount);

        char response[2048];
        response[0] = '\0';

        if (strcmp(mode, "count") == 0)
        {
            int charCount = count_chars(line);
            int wordCount = count_words(line);
            snprintf(response, sizeof(response), "%d번째 줄 결과 수신: Line: %d, chars: %d, words %d\n",lineCount,lineCount, charCount, wordCount);
        }
        else if (strcmp(mode, "upper") == 0)
        {
            to_upper(line);
            snprintf(response, sizeof(response), "%d번째 줄 결과 수신 : %s", lineCount, line);
            if (response[strlen(response)-1] != '\n') strcat(response, "\n");
        }
        else if (strcmp(mode, "lower") == 0)
        {
            to_lower(line);
            snprintf(response, sizeof(response), "%d번째 줄 결과 수신 : %s", lineCount, line);
            if (response[strlen(response)-1] != '\n') strcat(response, "\n");
        }
        else if (strcmp(mode, "reverse") == 0)
        {
            reverse_line(line);
            snprintf(response, sizeof(response), "%d번째 줄 결과 수신 : %s", lineCount, line);
            if (response[strlen(response)-1] != '\n') strcat(response, "\n");
        }
        else
        {
            snprintf(response, sizeof(response), "알 수 없는 모드\n");
        }

        write(fd[WRITE], response, strlen(response) + 1);
    }

    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;

    // 클라이언트에 END보내면서 모드랑 라인카운트 시간 전송
    char endMsg[256];
    snprintf(endMsg, sizeof(endMsg), "END|%s|%d|%.2f", mode, lineCount, elapsed);
    write(fd[WRITE], endMsg, strlen(endMsg) + 1);

    close(fd[READ]);
    close(fd[WRITE]);
    unlink("to_server");
    unlink("to_client");
    return 0;
}
