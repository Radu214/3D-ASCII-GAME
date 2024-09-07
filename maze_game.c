#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>
#include<unistd.h>

// Console screen size
int nScreenWidth = 120;
int nScreenHeight = 40;

// World dimensions
int nMapWidth = 16;
int nMapHeight = 16;

// Player start position
float fPlayerX = 14.7f;
float fPlayerY = 5.09f;
float fPlayerA = 0.0f;

// Field of view
float fFOV = 3.14159f / 4.0f;

// Maximum rendering distance
float fDepth = 16.0f;

// Walking speed
float fSpeed = 375.0f;

void map_generator(char **temp_map)
{
    for(int i = 0; i < nMapHeight; i++)
        for(int j = 0; j < nMapWidth; j++)
        {
            temp_map[i][j] = ' ';
        }

    for(int i = 0; i < nMapHeight; i++)
        for(int j = 0; j < nMapWidth; j++)
        {
            if(i == 0 || j == 0 || i == nMapHeight - 1 || j == nMapWidth - 1)
                temp_map[i][j] = '#';
            else {
                int x = rand() % 7;
                if(x == 1)
                    temp_map[i][j] = '#';
            }
        }
    //Spawn point
    temp_map[14][5] = ' ';
}



int main()
{
    // Initialize ncurses
    initscr();
    cbreak();
    noecho();
    curs_set(FALSE);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);

    srand(time(NULL));

    // Create screen buffer
    char **screen = (char**)malloc(nScreenHeight * sizeof(char*));
    for (int i = 0; i < nScreenHeight; i++)
        screen[i] = (char*)malloc(nScreenWidth * sizeof(char));

    // Create map of world space # = wall block, . = space
    char **map = (char**)malloc(nMapHeight * sizeof(char*));
    for (int i = 0; i < nMapHeight; i++)
        map[i] = (char*)malloc(nMapWidth * sizeof(char));
    map_generator(map);

    clock_t tp1 = clock();
    clock_t tp2;

    while (1)
    {
        tp2 = clock();
        float fElapsedTime = (float)(tp2 - tp1) / CLOCKS_PER_SEC;
        tp1 = tp2;

        int ch = getch();  // Get user input

        switch (ch)
        {
            // Handle CCW rotation
            case 'a': case 'A':
                fPlayerA -= fSpeed * fElapsedTime;
                break;
            
            // Handle CW rotation
            case 'd': case 'D':
                fPlayerA += fSpeed * fElapsedTime;
                break;

            // Handle forwards movement & collision
            case 'w': case 'W':
                fPlayerX += sinf(fPlayerA) * fSpeed * fElapsedTime;
                fPlayerY += cosf(fPlayerA) * fSpeed * fElapsedTime;
                if (map[(int)fPlayerY][(int)fPlayerX] == '#')
                {
                    fPlayerX -= sinf(fPlayerA) * fSpeed * fElapsedTime;
                    fPlayerY -= cosf(fPlayerA) * fSpeed * fElapsedTime;
                }
                break;

            // Handle backwards movement & collision
            case 's': case 'S':
                fPlayerX -= sinf(fPlayerA) * fSpeed * fElapsedTime;
                fPlayerY -= cosf(fPlayerA) * fSpeed * fElapsedTime;
                if (map[(int)fPlayerY][(int)fPlayerX] == '#')
                {
                    fPlayerX += sinf(fPlayerA) * fSpeed * fElapsedTime;
                    fPlayerY += cosf(fPlayerA) * fSpeed * fElapsedTime;
                }
                break;
            case 'e': case 'E':
                clear();
                map_generator(map);
                fPlayerX = 14.7f;
                fPlayerY = 5.09f;
                continue;

        }
        for (int x = 0; x < nScreenWidth; x++)
        {
            float fRayAngle = (fPlayerA - fFOV / 2.0f) + ((float)x / (float)nScreenWidth) * fFOV;
            float fStepSize = 0.1f;
            float fDistanceToWall = 0.0f;

            bool bHitWall = false;
            bool bBoundary = false;

            float fEyeX = sinf(fRayAngle);
            float fEyeY = cosf(fRayAngle);

            while (!bHitWall && fDistanceToWall < fDepth)
            {
                fDistanceToWall += fStepSize;
                int nTestX = (int)(fPlayerX + fEyeX * fDistanceToWall);
                int nTestY = (int)(fPlayerY + fEyeY * fDistanceToWall);

                if (nTestX < 0 || nTestX >= nMapWidth || nTestY < 0 || nTestY >= nMapHeight)
                {
                    bHitWall = true;
                    fDistanceToWall = fDepth;
                }
                else
                {
                    if (map[nTestY][nTestX] == '#')
                    {
                        bHitWall = true;

                        // To highlight tile boundaries
                        float p[4];
                        for (int tx = 0; tx < 2; tx++)
                            for (int ty = 0; ty < 2; ty++)
                            {
                                float vy = (float)nTestY + ty - fPlayerY;
                                float vx = (float)nTestX + tx - fPlayerX;
                                float d = sqrt(vx * vx + vy * vy);
                                float dot = (fEyeX * vx / d) + (fEyeY * vy / d);
                                p[tx * 2 + ty] = dot;
                            }
                        for (int i = 0; i < 4; i++)
                        {
                            if (acos(p[i]) < 0.01) bBoundary = true;
                        }
                    }
                }
            }

            int nCeiling = (float)(nScreenHeight / 2.0) - nScreenHeight / ((float)fDistanceToWall);
            int nFloor = nScreenHeight - nCeiling;

            // Shade
            char nShade = ' ';
            if (fDistanceToWall <= fDepth / 4.0f) nShade = '#';
            else if (fDistanceToWall < fDepth / 3.0f) nShade = 'x';
            else if (fDistanceToWall < fDepth / 2.0f) nShade = '~';
            else if (fDistanceToWall < fDepth) nShade = '.';

            if (bBoundary) nShade = ' ';

            for (int y = 0; y < nScreenHeight; y++)
            {
                if (y <= nCeiling)
                    screen[y][x] = ' ';
                else if (y > nCeiling && y <= nFloor)
                    screen[y][x] = nShade;
                else
                {
                    float b = 1.0f - (((float)y - nScreenHeight / 2.0f) / ((float)nScreenHeight / 2.0f));
                    if (b < 0.25) nShade = '#';
                    else if (b < 0.5) nShade = 'x';
                    else if (b < 0.75) nShade = '.';
                    else if (b < 0.9) nShade = '-';
                    else nShade = ' ';
                    screen[y][x] = nShade;
                }
            }
        }

        // Display frame
        for (int y = 0; y < nScreenHeight; y++)
            for (int x = 0; x < nScreenWidth; x++)
                mvaddch(y, x, screen[y][x]);

        refresh();

    }

    // Free screen buffer
    for (int i = 0; i < nScreenHeight; i++)
        free(screen[i]);
    free(screen);

    // Free map buffer
    for (int i = 0; i < nMapHeight; i++)
        free(map[i]);
    free(map);

    return 0;
}
