#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h> // Windows 콘솔 및 시스템 함수 사용

// 게임판 크기와 최대 뱀 길이 정의
#define WIDTH 30
#define HEIGHT 20
#define MAX_LENGTH 100

// 2D 좌표를 나타내는 구조체
typedef struct {
    int x, y;
} Point;

// 뱀 구조체
typedef struct {
    Point body[MAX_LENGTH]; // 뱀 몸통 좌표 배열
    int length;             // 현재 길이
    int dx, dy;             // 이동 방향
} Snake;

// 먹이 구조체
typedef struct {
    int x, y;
} Food;

/*
 * 커서를 특정 좌표로 이동시키는 함수
 * SetConsoleCursorPosition(HANDLE hConsoleOutput, COORD dwCursorPosition)
 * - hConsoleOutput: 콘솔 출력 핸들
 * - dwCursorPosition: 이동할 좌표(COORD 구조체)
 * COORD: x, y 좌표 구조체
 */
void gotoxy(int x, int y) {
    COORD pos = { x, y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

/*
 * 콘솔 커서를 숨기는 함수
 * SetConsoleCursorInfo(HANDLE hConsoleOutput, const CONSOLE_CURSOR_INFO *lpConsoleCursorInfo)
 * - hConsoleOutput: 콘솔 출력 핸들
 * - lpConsoleCursorInfo: 커서 정보 구조체
 * CONSOLE_CURSOR_INFO:
 *   dwSize: 커서 크기 (1~100)
 *   bVisible: TRUE/FALSE로 커서 보임 여부
 */
void hideCursor() {
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE); // 표준 출력 핸들 가져오기
    CONSOLE_CURSOR_INFO info = { 100, FALSE };          // 크기 100, 숨김
    SetConsoleCursorInfo(console, &info);            // 커서 정보 적용
}

// 게임판 그리기 (벽: #, 내부: 공백)
void drawBoard() {
    for (int y = 0;y <= HEIGHT;y++) {
        for (int x = 0;x <= WIDTH;x++) {
            if (x == 0 || x == WIDTH || y == 0 || y == HEIGHT) printf("#"); // 벽
            else printf(" "); // 빈 공간
        }
        printf("\n");
    }
}

// 자기 몸과 충돌했는지 확인
int checkSelfCollision(Snake* s) {
    for (int i = 1;i < s->length;i++) {
        if (s->body[0].x == s->body[i].x && s->body[0].y == s->body[i].y)
            return 1; // 충돌
    }
    return 0;
}

int main() {
    Snake snake;   // 뱀
    Food food;     // 먹이
    int score = 0; // 점수
    int baseSpeed = 150; // 기본 속도(ms)

    srand(time(0));  // 난수 초기화
    hideCursor();     // 콘솔 커서 숨기기

    // 뱀 초기 위치와 길이 설정
    snake.length = 3;
    snake.body[0].x = WIDTH / 2;      // 머리
    snake.body[0].y = HEIGHT / 2;
    snake.body[1].x = snake.body[0].x - 1;
    snake.body[1].y = snake.body[0].y;
    snake.body[2].x = snake.body[1].x - 1;
    snake.body[2].y = snake.body[1].y;
    snake.dx = 1; snake.dy = 0;     // 초기 이동 방향: 오른쪽

    // 먹이 초기 위치
    food.x = rand() % (WIDTH - 2) + 1;
    food.y = rand() % (HEIGHT - 2) + 1;

    drawBoard();  // 게임판 그리기
    gotoxy(food.x, food.y); printf("*"); // 먹이 표시
    for (int i = 0;i < snake.length;i++) {
        gotoxy(snake.body[i].x, snake.body[i].y);
        if (i == 0) printf("O"); // 머리
        else printf("o");      // 몸통
    }

    while (1) {
        // 키 입력 처리 (WASD)
        if (kbhit()) {
            char c = _getch();
            if (c == 'w' && snake.dy == 0) { snake.dx = 0; snake.dy = -1; } // 위
            else if (c == 's' && snake.dy == 0) { snake.dx = 0; snake.dy = 1; } // 아래
            else if (c == 'a' && snake.dx == 0) { snake.dx = -1; snake.dy = 0; } // 왼쪽
            else if (c == 'd' && snake.dx == 0) { snake.dx = 1; snake.dy = 0; }  // 오른쪽
        }

        // 이전 꼬리 지우기
        gotoxy(snake.body[snake.length - 1].x, snake.body[snake.length - 1].y);
        printf(" ");

        // 몸통 이동: 꼬리부터 한 칸씩 앞으로
        for (int i = snake.length - 1;i > 0;i--) snake.body[i] = snake.body[i - 1];

        // 머리 이동
        snake.body[0].x += snake.dx;
        snake.body[0].y += snake.dy;

        // 벽과 자기 몸 충돌 체크
        if (snake.body[0].x <= 0 || snake.body[0].x >= WIDTH || snake.body[0].y <= 0 || snake.body[0].y >= HEIGHT)
            break; // 벽 충돌 시 종료
        if (checkSelfCollision(&snake))
            break; // 자기 몸과 충돌 시 종료

        // 뱀 화면 그리기
        for (int i = 0;i < snake.length;i++) {
            gotoxy(snake.body[i].x, snake.body[i].y);
            if (i == 0) printf("O"); // 머리
            else printf("o");      // 몸통
        }

        // 먹이 먹기
        if (snake.body[0].x == food.x && snake.body[0].y == food.y) {
            if (snake.length < MAX_LENGTH) snake.length++; // 길이 증가
            score++; // 점수 증가

            // 속도 증가 (게임 점점 빨라짐)
            if (baseSpeed > 50) baseSpeed -= 5;

            // 새로운 먹이 생성 (뱀 몸과 겹치지 않도록)
            int valid = 0;
            while (!valid) {
                food.x = rand() % (WIDTH - 2) + 1;
                food.y = rand() % (HEIGHT - 2) + 1;
                valid = 1;
                for (int i = 0;i < snake.length;i++)
                    if (snake.body[i].x == food.x && food.y == snake.body[i].y) valid = 0;
            }
            gotoxy(food.x, food.y); printf("*"); // 먹이 표시
        }

        // 점수 표시
        gotoxy(0, HEIGHT + 1);
        printf("Score: %d", score);

        // X/Y 이동 속도 보정
        if (snake.dx != 0) Sleep(baseSpeed);           // 가로 이동
        else Sleep((int)(baseSpeed * 1.5));           // 세로 이동 보정
    }

    gotoxy(0, HEIGHT + 2);
    printf("Game Over! Final Score: %d\n", score); // 게임 종료 메시지
    return 0;
}
