#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h> // Windows 콘솔 및 시스템 함수 사용

// 게임판 크기와 최대 뱀 길이 정의
//#define WIDTH 60
//#define HEIGHT 25
#define MAX_LENGTH 100

// 2D 좌표를 나타내는 구조체
typedef struct
{
    int x, y;
} Point;

// 뱀 구조체
typedef struct
{
    Point body[MAX_LENGTH]; // 뱀 몸통 좌표 배열
    int length;             // 현재 길이
    int dx, dy;             // 이동 방향
} Snake;

// 먹이 구조체
typedef struct
{
    int x, y;
} Food;

/*
   커서를 특정 좌표로 이동시키는 함수(이동시키지 않으면 cmd창이 계속 버벅거리게 됨)
   SetConsoleCursorPosition(GetStdHandle, pos)
   - GetStdHandle: 화면, 키보드 조작할 수 있게 해주는 함수
   - COORD: x, y 좌표가 있는 구조체
 */
void gotoxy(int x, int y)
{
    COORD pos = { x, y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos); //STD_OUTPUT_HANDLE : 모니터 화면
}

/*
   콘솔 커서를 숨기는 함수(화면에 깜박이는 입력 커서 표시 없애기 위해)
   SetConsoleCursorInfo(GetStdHandle, info)

   CONSOLE_CURSOR_INFO(dwSize, bVisible)
   - dwSize: 커서 크기 (1~100)
   - bVisible: TRUE/FALSE로 커서 보임 여부
 */
void hideCursor()
{
    CONSOLE_CURSOR_INFO info = { 100, FALSE };          // 크기 100, 숨김
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);            // 커서 정보 적용
}

// 게임판 그리기 (벽: #, 내부: 공백)
void drawBoard(int* width, int* height)
{
    int y;
    int x;

    for (y = 0; y <= height; y++)
    {
        for (x = 0;x <= width; x++)
        {
            if (x == 0 || x == width || y == 0 || y == height)
            {
                printf("#"); // 벽
            }
            else printf(" "); // 빈 공간
        }
        printf("\n");
    }
}

// 자기 몸과 충돌했는지 확인
int checkSelfCollision(Snake* s)
{
    int i;

    for (i = 1; i < s->length; i++)
    {
        if (s->body[0].x == s->body[i].x && s->body[0].y == s->body[i].y)
            return 1; // 충돌
    }
    return 0;
}


// 특수 먹이 이벤트
void FoodEvent(Point specialFood[], int specialCount, int* specialActive, time_t* specialStart, Snake* snake, int* width, int* height) 
{
    time_t now = time(NULL);

    // 10초마다 이벤트 생성
    if (!(*specialActive) && ((now - *specialStart) >= 10)) 
    {
        *specialActive = 1;
        *specialStart = now;

        for (int i = 0; i < specialCount; i++) 
        {
            specialFood[i].x = rand() % 58+ 1;
            specialFood[i].y = rand() % 23+ 1;
            gotoxy(specialFood[i].x, specialFood[i].y);
            printf("$"); // 특수 먹이 표시
        }
    }

    // 특수 먹이 먹기
    if (*specialActive) 
    {
        for (int i = 0; i < specialCount; i++) 
        {
            if (snake->body[0].x == specialFood[i].x && snake->body[0].y == specialFood[i].y) 
            {
                if (snake->length > 0) 
                {
                    // 화면에서 꼬리 지우기
                    gotoxy(snake->body[snake->length - 1].x, snake->body[snake->length - 1].y);
                    printf(" ");
                    snake->length--; // 몸 길이 감소
                }
                gotoxy(specialFood[i].x, specialFood[i].y);
                printf(" ");
                specialFood[i].x = -1; specialFood[i].y = -1;
            }
        }

        // 10초 지나면 이벤트 종료
        if (now - *specialStart >= 10) 
        {
            *specialActive = 0;
            for (int i = 0; i < specialCount; i++) 
            {
                if (specialFood[i].x != -1) 
                {
                    gotoxy(specialFood[i].x, specialFood[i].y);
                    printf(" ");
                }
            }
        }
    }
}

// 벽 축소 이벤트
void WallShrinkEvent(int* width, int* height, int shrinkX, int shrinkY, time_t* lastShrink, Snake* snake, Food* food) {
    time_t now = time(NULL);

    if (now - *lastShrink >= 10) {
        *lastShrink = now;

        *width -= shrinkX;
        *height -= shrinkY;

        for (int i = 0; i < snake->length; i++) {
            if (snake->body[i].x >= *width || snake->body[i].y >= *height) {
                snake->length = 0;
                return;
            }
        }

        if (food->x >= *width || food->y >= *height) {
            gotoxy(food->x, food->y);
            printf(" ");
            food->x = "#"; food->y = "#";
        }

        for (int y = 0; y <= *height; y++)
            for (int x = 0; x <= *width; x++)
                if (x == 0 || x == *width || y == 0 || y == *height) {
                    gotoxy(x, y);
                    printf("#");
                }
    }
}

int main()
{
    int width = 60;
    int height = 25;
    Snake snake;   // 뱀
    Food food;     // 먹이
    int score = 0; // 점수
    int baseSpeed = 150; // 기본 속도(ms)

    srand(time(0));  // 난수 초기화
    hideCursor();     // 콘솔 커서 숨기기

    // 뱀 초기 위치와 길이 설정
    snake.length = 3;
    snake.body[0].x = width / 2;      // 머리
    snake.body[0].y = height / 2;
    snake.body[1].x = snake.body[0].x - 1;
    snake.body[1].y = snake.body[0].y;
    snake.body[2].x = snake.body[1].x - 1;
    snake.body[2].y = snake.body[1].y;
    snake.dx = 1;
    snake.dy = 0;     // 초기 이동 방향: 오른쪽

    // 먹이 초기 위치
    food.x = rand() % (width - 2) + 1;
    food.y = rand() % (height - 2) + 1;

    drawBoard(width, height);  // 게임판 그리기
    gotoxy(food.x, food.y); printf("*"); // 먹이 표시
    for (int i = 0;i < snake.length;i++)
    {
        gotoxy(snake.body[i].x, snake.body[i].y);
        if (i == 0)
        {
            printf("O"); // 머리
        }
        else
        {
            printf("o"); // 몸통
        }
    }

    time_t startTime = time(NULL); //time_t : 초를 저장하는 정수형 타입

    // 특수 이벤트 변수
    Food specialFood[30];
    int specialCount = 30;
    int specialActive = 0;
    time_t specialStart = startTime;
    time_t lastShrink = startTime;

    while (1)
    {
        // 키 입력 처리 (WASD)
        if (_kbhit()) //kbhit : 키보드 입력이 있는지 확인하는함수, 있으면 1, 없으면 0.
        {
            char c = _getch(); //getch함수는 입력이 있을 때까지 프로그램이 멈춰있기 때문에 kbhit사용
            if (c == 'w' && snake.dy == 0) { snake.dx = 0; snake.dy = -1; } // 위
            else if (c == 's' && snake.dy == 0) { snake.dx = 0; snake.dy = 1; } // 아래
            else if (c == 'a' && snake.dx == 0) { snake.dx = -1; snake.dy = 0; } // 왼쪽
            else if (c == 'd' && snake.dx == 0) { snake.dx = 1; snake.dy = 0; }  // 오른쪽
        }

        // 이전 꼬리 지우기
        gotoxy(snake.body[snake.length - 1].x, snake.body[snake.length - 1].y);
        printf(" ");

        // 몸통 이동: 꼬리부터 한 칸씩 앞으로
        for (int i = snake.length - 1;i > 0;i--)
        {
            snake.body[i] = snake.body[i - 1];
        }

        // 머리 이동
        snake.body[0].x += snake.dx;
        snake.body[0].y += snake.dy;

        // 벽과 자기 몸 충돌 체크
        if (snake.body[0].x <= 0 || snake.body[0].x >= width || snake.body[0].y <= 0 || snake.body[0].y >= height)
        {
            break; // 벽 충돌 시 종료
        }
        if (checkSelfCollision(&snake))
        {
            break; // 자기 몸과 충돌 시 종료
        }
        if (snake.length == 0)
        {
            break; //머리와 몸통 중 하나도 남아있지 않을 시 종료
        }

        // 뱀 화면 그리기
        for (int i = 0;i < snake.length;i++)
        {
            gotoxy(snake.body[i].x, snake.body[i].y);
            if (i == 0)
            {
                printf("O"); // 머리
            }
            else
            {
                printf("o"); // 몸통
            }
        }

        // 먹이 먹기
        if (snake.body[0].x == food.x && snake.body[0].y == food.y)
        {
            if (snake.length < MAX_LENGTH)
            {
                snake.length++; // 길이 증가
            }
            score++; // 점수 증가

            // 속도 증가 (게임 점점 빨라짐)
            if (baseSpeed > 50)
            {
                baseSpeed -= 5;
            }

            // 새로운 먹이 생성 (뱀 몸과 겹치지 않도록)
            int valid = 0;

            while (!valid)
            {
                food.x = rand() % (width - 2) + 1;
                food.y = rand() % (height - 2) + 1;
                valid = 1;
                for (int i = 0;i < snake.length;i++)
                {
                    if (snake.body[i].x == food.x && food.y == snake.body[i].y)
                    {
                        valid = 0;
                    }
                }
            }
            gotoxy(food.x, food.y);
            printf("*"); // 먹이 표시
        }


        FoodEvent(specialFood, specialCount, &specialActive, &specialStart, &snake, width, height);
        WallShrinkEvent(&width, &height, 1, 1, &lastShrink, &snake, &food);

        // 점수 표시
        gotoxy(0, height + 1);
        printf("Score: %d\n", score);

        time_t now = time(NULL);
        int total = (int)(now - startTime);

        gotoxy(0, height + 2);
        printf("Time: %02d:%02d  \n", total / 60, total % 60);

        Sleep(baseSpeed); //Sleep : 프로그램 실행을 일정시간동안 멈추는 함수

    }
    gotoxy(0, height + 2);
    printf("Game Over! Final Score: %d\n", score); // 게임 종료 메시지

    time_t now = time(NULL);
    int total = (int)(now - startTime);
    printf("Time: %02d:%02d  \n", total / 60, total % 60);

    return 0;
}
