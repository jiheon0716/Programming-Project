#include <stdio.h>
#include <conio.h> // _kbhit(), _getch() 함수 사용
#include <stdlib.h> // rand(), srand() 함수 사용
#include <time.h> // time() 함수 사용
#include <windows.h> // COORD, SetConsoleCursorPosition, GetStdHandle, SetConsoleCursorInfo, Sleep 함수 사용

#define MAX_LENGTH 100
#define MAX_MINES 10         
#define MAX_SPECIAL_FOOD 25	
#define MAX_BLOCKADE_LENGTH 15 

// --- 환경 변화 이벤트 선언(초기 시작 시간, 주기, 지속 시간) ---
#define REVERSE_DELAY 30     
#define REVERSE_CYCLE 15     
#define REVERSE_DURATION 5    

#define BLOCKADE_DELAY 40    
#define BLOCKADE_CYCLE 10   
#define BLOCKADE_DURATION 5  
#define BLOCKADE_MIN_LENGTH 12 //최소 길이 (12칸)
#define BLOCKADE_MAX_LENGTH 15 //최대 길이 (15칸)

#define FOG_DELAY 50       
#define FOG_CYCLE 20        
#define FOG_DURATION 10     

#define NORMAL_FOOD_CYCLE 10 
#define NORMAL_FOOD_DELAY 5  

#define SPECIAL_FOOD_CYCLE 10
#define SPECIAL_FOOD_DELAY 10

#define MINE_DELAY 20     
#define MINE_CYCLE 10      
#define MINE_DURATION 5   


// 구조체 정의
typedef struct //게임 내 모든 좌표
{ 
    int x, y; 
} Point;

typedef struct //뱀의 몸통 배열, 현재 길이, 이동 방향
{ 
    Point body[MAX_LENGTH]; 
    int length; 
    int dx, dy; 
} Snake;

typedef struct //먹이 좌표
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
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
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
    CONSOLE_CURSOR_INFO info = { 100, FALSE };
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
}

//게임판 그리기 (가로 60, 세로 25)
void drawBoard(int width, int height)
{
    for (int x = 0; x <= width; x++)
    {
        gotoxy(x, 0); printf("#");
        gotoxy(x, height); printf("#");
    }
    for (int y = 0; y <= height; y++)
    {
        gotoxy(0, y); printf("#");
        gotoxy(width, y); printf("#");
    }
}

// 자기 몸 충돌 체크
int checkSelfCollision(Snake* s)
{
    for (int i = 1; i < s->length; i++)
    {
        if (s->body[0].x == s->body[i].x && s->body[0].y == s->body[i].y)
        {
            return 1;
        }
    }
    return 0;
}

// 지뢰 이벤트: 주기적으로 지뢰('X')를 생성하고, 뱀과의 충돌을 확인
int MineEvent(Point mines[], int mineCount, int* active, time_t* start, Snake* snake, int width, int height, time_t startTime)
{
    time_t now = time(NULL);

    // MINE_DELAY가 지나지 않았으면, 지뢰를 비활성화 상태로 유지
    if ((now - startTime) < MINE_DELAY)
    {
        if (*active) // 이미 활성화된 지뢰가 있다면 제거
        {
            for (int i = 0; i < mineCount; i++)
            {
                gotoxy(mines[i].x, mines[i].y); printf(" ");
                mines[i].x = -1;
            }
            *active = 0;
        }
        return 0;
    }

    int elapsed = (int)(now - *start); // 현재 주기에서의 경과 시간
    int collision = 0;

    if (elapsed < MINE_DURATION) // 지뢰 활성 시간 (MINE_DURATION) 동안
    {
        if (!(*active)) // 비활성화 상태였다면 지뢰 생성 및 활성화
        {
            *active = 1;
            for (int i = 0; i < mineCount; i++)
            {
                int valid = 0;
                while (!valid) // 뱀의 몸통과 겹치지 않는 위치에 지뢰 랜덤 생성
                {
                    mines[i].x = rand() % (width - 2) + 1;
                    mines[i].y = rand() % (height - 2) + 1;
                    valid = 1;
                    for (int k = 0; k < snake->length; k++)
                    {
                        if (snake->body[k].x == mines[i].x && snake->body[k].y == mines[i].y)
                        {
                            valid = 0;
                            break;
                        }
                    }
                }
                gotoxy(mines[i].x, mines[i].y); printf("X");
            }
        }
        // 뱀 머리와 지뢰의 충돌 체크
        for (int i = 0; i < mineCount; i++)
        {
            if (snake->body[0].x == mines[i].x && snake->body[0].y == mines[i].y)
            {
                collision = 1; // 충돌 발생 시 게임 오버
            }
        }
    }
    else //주기가 끝났다면 타이머 재설정
    {
        *start = now;
    }
    return collision;
}

// 일반 먹이 이벤트 NORMAL_FOOD_CYCLE 주기로 일반 먹이('*')를 재생성
void NormalFoodEvent(Food* normalFood, time_t* normalStart, Snake* snake, int width, int height)
{
    time_t now = time(NULL);

    // 음식 주기 시간이 지났다면
    if ((now - *normalStart) >= NORMAL_FOOD_CYCLE)
    {

        if (normalFood->x != -1) // 기존 먹이가 있다면 화면에서 제거
        {
            gotoxy(normalFood->x, normalFood->y); printf(" ");
        }

        int valid = 0;
        while (!valid) // 뱀 몸통과 겹치지 않는 새로운 위치에 먹이 랜덤 생성
        {
            normalFood->x = rand() % (width - 2) + 1;
            normalFood->y = rand() % (height - 2) + 1;
            valid = 1;

            for (int k = 0; k < snake->length; k++)
            {
                if (snake->body[k].x == normalFood->x && snake->body[k].y == normalFood->y)
                {
                    valid = 0;
                    break;
                }
            }
        }
        gotoxy(normalFood->x, normalFood->y); 
        printf("*"); // 화면에 먹이 출력

        *normalStart = now; // 타이머 재설정
    }
}

// 특수 먹이 이벤트 SPECIAL_FOOD_CYCLE 주기로 특수 먹이('$')를 재생성하고, 뱀과의 충돌 시 길이를 감소
void SpecialFoodEvent(Point specialFood[], int specialCount, time_t* specialStart, Snake* snake, int width, int height, time_t startTime)
{
    time_t now = time(NULL);

    if ((now - startTime) < SPECIAL_FOOD_DELAY)
    {
        return;
    }

    // SPECIAL_FOOD_CYCLE 시간이 지났다면 특수 먹이 재생성
    if ((now - *specialStart) >= SPECIAL_FOOD_CYCLE)
    {
        for (int i = 0; i < specialCount; i++)
        {
            if (specialFood[i].x != -1)
            {
                gotoxy(specialFood[i].x, specialFood[i].y); 
                printf(" ");
                specialFood[i].x = -1;
            }
        }
        // 새로운 특수 먹이 생성
        for (int i = 0; i < specialCount; i++)
        {
            int valid = 0;
            while (!valid) // 뱀 몸통과 겹치지 않는 위치에 랜덤 생성
            {
                specialFood[i].x = rand() % (width - 2) + 1;
                specialFood[i].y = rand() % (height - 2) + 1;
                valid = 1;
                for (int k = 0; k < snake->length; k++)
                {
                    if (snake->body[k].x == specialFood[i].x && snake->body[k].y == specialFood[i].y)
                    {
                        valid = 0;
                        break;
                    }
                }
            }
            gotoxy(specialFood[i].x, specialFood[i].y); 
            printf("$"); // 화면에 특수 먹이 출력
        }
        *specialStart = now; // 타이머 재설정
    }

    // 뱀 머리와 특수 먹이의 충돌 체크
    for (int i = 0; i < specialCount; i++)
    {
        if (specialFood[i].x == -1)
        {
            continue;
        }
        if (snake->body[0].x == specialFood[i].x && snake->body[0].y == specialFood[i].y)
        {
            if (snake->length > 1) // 뱀의 길이가 1보다 클 경우에만 감소
            {
                // 뱀 꼬리 제거 (화면에서 지우고 길이 감소)
                gotoxy(snake->body[snake->length - 1].x, snake->body[snake->length - 1].y); 
                printf(" ");
                snake->length--;
            }
            // 섭취한 특수 먹이 제거
            gotoxy(specialFood[i].x, specialFood[i].y); printf(" ");
            specialFood[i].x = -1; specialFood[i].y = -1;
        }
    }
}

// 조작 반전 이벤트 로직 REVERSE_DELAY 후 주기마다 지속 시간 동안 조작 반전(isReversed = 1)
void ReverseEventLogic(time_t startTime, int* isReversed)
{
    time_t now = time(NULL);
    int totalElapsed = (int)(now - startTime); // 게임 시작 후 총 경과 시간

    int cycleElapsed;
    // 초기 지연 시간(REVERSE_DELAY)을 제외한 시간 계산
    if (totalElapsed >= REVERSE_DELAY)
    {
        cycleElapsed = totalElapsed - REVERSE_DELAY;
    }
    else
    {
        cycleElapsed = 0;
    }

    // 주기의 나머지 시간이 지속 시간보다 작으면 활성화
    if (cycleElapsed > 0 && cycleElapsed % REVERSE_CYCLE < REVERSE_DURATION)
    {
        *isReversed = 1;
    }
    else
    {
        *isReversed = 0;
    }
}

// 일시적 장애물 이벤트: BLOCKADE_DELAY 후 주기마다 지속 시간 동안 일시적 장애물(| 또는 -) 생성
void BlockadeEventLogic(time_t startTime, Point tempWall[], int* blockadeActive, int* blockadeLength, int width, int height)
{
    time_t now = time(NULL);
    int totalElapsed = (int)(now - startTime);

    // 초기 지연 시간(BLOCKADE_DELAY) 미만이면 비활성화 상태 유지
    if (totalElapsed < BLOCKADE_DELAY)
    {
        if (*blockadeActive) // 혹시 모를 활성 상태 정리
        {
            for (int i = 0; i < *blockadeLength; i++)
            {
                gotoxy(tempWall[i].x, tempWall[i].y); printf(" ");
                tempWall[i].x = -1;
            }
            *blockadeActive = 0;
        }
        return;
    }

    int cycleElapsed = totalElapsed - BLOCKADE_DELAY;

    // 활성 시간 (BLOCKADE_DURATION) 동안
    if (cycleElapsed % BLOCKADE_CYCLE < BLOCKADE_DURATION)
    {
        if (!(*blockadeActive)) // 비활성화 상태였다면 장애물 생성 및 활성화
        {
            *blockadeActive = 1;
            int isVertical = rand() % 2;// 수직(|) 또는 수평(-) 결정

            //길이를 BLOCKADE_MIN_LENGTH(12)와 BLOCKADE_MAX_LENGTH(15)사이로 설정
            int length = BLOCKADE_MIN_LENGTH + (rand() % (BLOCKADE_MAX_LENGTH - BLOCKADE_MIN_LENGTH + 1));

            int startX, startY;
            *blockadeLength = length;

            // 장애물 시작 위치 랜덤 결정 (경계선 안쪽 2칸 이상 여백 확보)
            if (isVertical)
            {
                startX = rand() % (width - 4) + 2;
                startY = rand() % (height - length - 2) + 1;
            }
            else
            {
                startX = rand() % (width - length - 2) + 1;
                startY = rand() % (height - 4) + 2;
            }

            // 장애물 그리기
            for (int i = 0; i < length; i++)
            {
                if (isVertical)
                {
                    tempWall[i].x = startX;
                    tempWall[i].y = startY + i;
                    gotoxy(startX, startY + i); printf("|");
                }
                else
                {
                    tempWall[i].x = startX + i;
                    tempWall[i].y = startY;
                    gotoxy(startX + i, startY); printf("-");
                }
            }
        }
    }
    else // 비활성 시간
    {
        if (*blockadeActive) // 활성화 상태였다면 장애물 제거 및 비활성화
        {
            for (int i = 0; i < *blockadeLength; i++)
            {
                gotoxy(tempWall[i].x, tempWall[i].y);
                printf(" ");
                tempWall[i].x = -1;
            }
            *blockadeActive = 0;
        }
    }
}

// 맵 시야 감소 이벤트 로직 FOG_DELAY 후 주기마다 동안 맵 깜빡임(시야 감소) 적용
void FogEventLogic(time_t startTime, int* isFoggy, int* flashOn)
{
    time_t now = time(NULL);
    int totalElapsed = (int)(now - startTime);

    if (totalElapsed < FOG_DELAY)
    {
        *isFoggy = 0;
        *flashOn = 1; // 깜빡임 비활성화 상태에서는 항상 보이도록 (flashOn = 1)
        return;
    }

    int cycleElapsed = totalElapsed - FOG_DELAY;

    // 활성 시간 (FOG_DURATION) 동안
    if (cycleElapsed % FOG_CYCLE < FOG_DURATION)
    {
        *isFoggy = 1; // 시야 감소 이벤트 활성화

        // 매 틱마다 flashOn 상태 반전 (깜빡임 효과)
        *flashOn = !(*flashOn);

    }
    else // 비활성 시간
    {
        *isFoggy = 0; // 시야 감소 이벤트 비활성화
        *flashOn = 1; // 깜빡임 비활성화 상태에서는 항상 보이도록
    }
}


int main()
{
    int width = 60; //게임판 가로 크기
    int height = 25; //게임판 세로 크기
    Snake snake;
    Food normalFood;
    int score = 0;
    int baseSpeed = 150; // 기본 이동 속도 (ms) - 값이 작을수록 빠름

    srand((unsigned int)time(0)); // 랜덤 시드 초기화
    hideCursor(); // 커서 숨김

    // 뱀 초기화 
    snake.length = 3;
    // 초기 위치 설정 (중앙)
    snake.body[0].x = width / 2;
    snake.body[0].y = height / 2;
    snake.body[1].x = snake.body[0].x - 1;
    snake.body[1].y = snake.body[0].y;
    snake.body[2].x = snake.body[1].x - 1;
    snake.body[2].y = snake.body[1].y;
    snake.dx = 1; snake.dy = 0; // 초기 이동 방향 (오른쪽)

    normalFood.x = -1; // 일반 먹이 초기 비활성화
    normalFood.y = -1;

    drawBoard(width, height); // 게임판 그리기

    for (int i = 0; i < snake.length; i++)
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
     
    time_t startTime = time(NULL); // 게임 시작 시간 기록

    // 일반 먹이 타이머 초기화 (첫 일반 먹이 생성을 NORMAL_FOOD_DELAY 초 후에 발생시키기 위해)
    time_t normalStart = startTime - (NORMAL_FOOD_CYCLE - NORMAL_FOOD_DELAY);
    time_t specialStart = startTime; // 특수 먹이 타이머 초기화

    Point specialFood[MAX_SPECIAL_FOOD];
    int specialCount = MAX_SPECIAL_FOOD;

    // 이벤트 변수
    time_t mineStart = startTime;
    Point mines[MAX_MINES];
    int mineCount = 10;
    int mineActive = 0;

    int isReversed = 0; // 조작 반전 상태
    Point tempWall[MAX_BLOCKADE_LENGTH];
    int blockadeActive = 0; // 일시적 장애물 활성 상태
    int blockadeLength = 0; 

    int isFoggy = 0; // 시야 감소 이벤트 상태
    time_t flashTime = time(NULL);
    int flashOn = 1; // 깜빡임 상태 (1: 보임, 0: 숨김)

    // 특수 먹이 초기 비활성화
    for (int i = 0; i < specialCount; i++)
    {
        specialFood[i].x = -1;
    }

    //게임 루프 시작
    while (1)
    {
        time_t now = time(NULL);
        int totalElapsed = (int)(now - startTime);

        // 이벤트 실행 
        ReverseEventLogic(startTime, &isReversed);
        BlockadeEventLogic(startTime, tempWall, &blockadeActive, &blockadeLength, width, height);
        FogEventLogic(startTime, &isFoggy, &flashOn);

        // --- 키 입력 로직 ---
        if (_kbhit()) //kbhit : 키보드 입력이 있는지 확인하는함수, 있으면 1, 없으면 0.
        {
            char c = _getch(); //getch함수는 입력이 있을 때까지 프로그램이 멈춰있기 때문에 kbhit사용
            int temp_dx = 0, temp_dy = 0;

            if (isReversed)
            {
                if (c == 'w' && snake.dy == 0) { temp_dx = 0; temp_dy = 1; }
                else if (c == 's' && snake.dy == 0) { temp_dx = 0; temp_dy = -1; }
                else if (c == 'a' && snake.dx == 0) { temp_dx = 1; temp_dy = 0; }
                else if (c == 'd' && snake.dx == 0) { temp_dx = -1; temp_dy = 0; }
            }
            else
            {
                if (c == 'w' && snake.dy == 0) { temp_dx = 0; temp_dy = -1; }
                else if (c == 's' && snake.dy == 0) { temp_dx = 0; temp_dy = 1; }
                else if (c == 'a' && snake.dx == 0) { temp_dx = -1; temp_dy = 0; }
                else if (c == 'd' && snake.dx == 0) { temp_dx = 1; temp_dy = 0; }
            }

            if (temp_dx != 0 || temp_dy != 0)
            {
                snake.dx = temp_dx;
                snake.dy = temp_dy;
            }
        }

        // 뱀 이동 및 충돌 체크 (생략)
        gotoxy(snake.body[snake.length - 1].x, snake.body[snake.length - 1].y);
        printf(" ");

        // 몸통 이동: 꼬리부터 시작하여 앞 body의 위치를 따라감
        for (int i = snake.length - 1; i > 0; i--)
        {
            snake.body[i] = snake.body[i - 1];
        }

        // 머리 이동: 현재 방향(dx, dy)으로 한 칸 이동
        snake.body[0].x += snake.dx;
        snake.body[0].y += snake.dy;

        // 일시적 장애물 충돌 체크
        if (blockadeActive)
        {
            for (int i = 0; i < blockadeLength; i++)
            {
                if (tempWall[i].x != -1 && snake.body[0].x == tempWall[i].x && snake.body[0].y == tempWall[i].y)
                {
                    snake.body[0].x -= snake.dx;
                    snake.body[0].y -= snake.dy;
                    snake.dx = 0;
                    snake.dy = 0;
                    break;
                }
            }
        }

        // 벽 충돌 체크
        if (snake.body[0].x <= 0 || snake.body[0].x >= width ||
            snake.body[0].y <= 0 || snake.body[0].y >= height)
        {
            break;
        }

        // 자기 몸 충돌 체크
        if (checkSelfCollision(&snake))
        {
            break;
        }

        //화면 출력 및 깜빡임
        // 시야 감소 이벤트 중이고, 현재 '숨겨야 하는' 틱(flashOn=0)인 경우
        if (isFoggy && !flashOn)
        {
            for (int y = 1; y < height; y++) // 맵 내부 전체를 공백으로 지움
            {
                for (int x = 1; x < width; x++)
                {
                    gotoxy(x, y);
                    printf(" ");
                }
            }
        }

        // 시야 감소 이벤트가 아니거나, 시야 감소 이벤트 중이지만 현재 보여줘야 하는 틱인 경우
        if (!isFoggy || flashOn)
        {
            // 모든 요소를 다시 그립니다.
            for (int i = 0; i < snake.length; i++)
            {
                gotoxy(snake.body[i].x, snake.body[i].y);
                if (i == 0)
                {
                    printf("O");
                }
                else
                {
                    printf("o");
                }
            }

            if (normalFood.x != -1)
            {
                gotoxy(normalFood.x, normalFood.y); printf("*");
            }
            for (int i = 0; i < specialCount; i++)
            {
                if (specialFood[i].x != -1)
                {
                    gotoxy(specialFood[i].x, specialFood[i].y); printf("$");
                }
            }
            if (mineActive)
            {
                for (int i = 0; i < mineCount; i++)
                {
                    if (mines[i].x != -1)
                    {
                        gotoxy(mines[i].x, mines[i].y); printf("X");
                    }
                }
            }
            if (blockadeActive)
            {
                for (int i = 0; i < blockadeLength; i++)
                {
                    if (tempWall[i].x != -1)
                    {
                        gotoxy(tempWall[i].x, tempWall[i].y);
                        if (tempWall[i].x == tempWall[0].x)
                        {
                            printf("|");
                        }
                        else
                        {
                            printf("-");
                        }
                    }
                }
            }
        }

        // 일반 먹이 처리
        if (snake.body[0].x == normalFood.x && snake.body[0].y == normalFood.y)
        {
            if (snake.length < MAX_LENGTH) // 최대 길이보다 짧으면
            {
                snake.length++; // 길이 증가
            }
            score++;

            if (baseSpeed > 50) // 최고 속도(50ms)보다 느리면
            {
                baseSpeed -= 5; // 속도 증가 (딜레이 감소)
            }

            gotoxy(normalFood.x, normalFood.y); 
            printf(" "); //먹이 먹으면 지우기

            int valid = 0;
            while (!valid) // 뱀 몸통과 겹치지 않는 새로운 위치에 먹이 랜덤 생성
            {
                normalFood.x = rand() % (width - 2) + 1;
                normalFood.y = rand() % (height - 2) + 1;
                valid = 1;
                for (int i = 0; i < snake.length; i++)
                {
                    if (snake.body[i].x == normalFood.x && snake.body[i].y == normalFood.y)
                    {
                        valid = 0;
                        break;
                    }
                }
            }
            gotoxy(normalFood.x, normalFood.y); printf("*");
            normalStart = time(NULL);
        }

        // 먹이 이벤트 주기적 실행 (미섭취 시 재생성 처리)
        NormalFoodEvent(&normalFood, &normalStart, &snake, width, height);
        SpecialFoodEvent(specialFood, specialCount, &specialStart, &snake, width, height, startTime);

        // 지뢰 충돌 체크 및 이벤트 주기적 실행
        if (MineEvent(mines, mineCount, &mineActive, &mineStart, &snake, width, height, startTime))
        {
            break;
        }

        //점수 및 알림 표시
        gotoxy(0, height + 1);
        printf("Score: %d   Speed: %dms  Time: %d s", score, baseSpeed, totalElapsed);

        gotoxy(0, height + 2);
        //조작 반전 이벤트 하단에 알려주기
        if (isReversed)
        {
            printf(">>> 조작 반전 이벤트! <<< ");
        }
        else
        {
            printf("                                    ");
        }

        Sleep(baseSpeed); // 뱀의 속도(딜레이)만큼 대기
    }

    //게임 종료
    gotoxy(0, height + 3);
    printf("Game Over! Final Score: %d\n", score);

    return 0;
}