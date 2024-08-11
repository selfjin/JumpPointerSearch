#include "framework.h"
#include <windowsx.h>
#include <Windows.h>
#include <queue>
#include <vector>
#include <map>
#include <algorithm>
#include "JumpPointerSearch.h"


#define MAX_LOADSTRING 100



/*                                                          //
                    사용자 정의 전역 변수
*/                                                          //

#define GRID_WIDTH 100
#define GRID_HEIGHT 50

using namespace std;

char g_Tile[GRID_HEIGHT][GRID_WIDTH];




/*                                                          //
                     랜더 사용 전역 변수
*/                                                          //
HBRUSH g_hTileBrush;
HPEN g_hGridPen;

bool g_bErase = false;
bool g_bDrag = false;
bool g_bFirst = false;
bool g_bFinal = false;

bool g_bEndAStar = false;

pair<int, int> Start_Pos = { 0, 0 };
pair<int, int> End_Pos = { 0, 0 };



POINT g_Origin = { 0, 0 };
//POINT position = { 0, 0 };
//int g_MasterY = 0;
//int g_MasterX = 0;
int GRID_SIZE = 16;

HBITMAP g_hMemDCBitmap;
HBITMAP g_hMemDCBitmap_old;
HDC g_hMemDC;
RECT g_MemDCRect;



/*                                                          //
                      우아한 전방 선언
*/                                                          //


int Astar(char(*Map)[100], pair<int, int> start, pair<int, int> goal, int max_X, int max_Y, HWND hWnd);
int JPS(char(*Map)[100], pair<int, int> start, pair<int, int> goal, int max_X, int max_Y, HWND hWnd);

/*                                                          //
                      우아한 변수의 경계
*/                                                          //

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);



int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    //RegisterClass
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = ::LoadIcon(hInstance, MAKEINTRESOURCE(IDI_JUMPPOINTERSEARCH));
    wcex.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_JUMPPOINTERSEARCH);
    wcex.lpszClassName = L"abcde";
    wcex.hIconSm = ::LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    RegisterClassExW(&wcex);

    hInst = hInstance;

    HWND hWnd = ::CreateWindowW(L"abcde", L"Title", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    ::ShowWindow(hWnd, nCmdShow);
    ::UpdateWindow(hWnd);

    MSG msg;

    HDC hdc = GetDC(hWnd);
    while (::GetMessage(&msg, nullptr, 0, 0))
    {
        //if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            //Rectangle(hdc, position.x - 5, position.y - 5, position.x + 5, position.y + 5);
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);

        }
    }

}



void RenderGrid(HDC hdc)
{
    int iX = -g_Origin.x;
    int iY = -g_Origin.y;

    HPEN hOldPen = (HPEN)SelectObject(hdc, g_hGridPen);

    for (int i = 0; i <= GRID_WIDTH; i++)
    {
        MoveToEx(hdc, iX, 0, NULL);
        LineTo(hdc, iX, GRID_HEIGHT * GRID_SIZE);

        iX += GRID_SIZE;
    }

    iX = -g_Origin.x; // Reset iX for horizontal lines
    for (int i = 0; i <= GRID_HEIGHT; i++)
    {
        MoveToEx(hdc, 0, iY, NULL);
        LineTo(hdc, GRID_WIDTH * GRID_SIZE, iY);

        iY += GRID_SIZE;
    }

    SelectObject(hdc, hOldPen);
}



void RenderObstacle(HDC hdc)
{
    int iX = -g_Origin.x;
    int iY = -g_Origin.y;
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, g_hTileBrush);
    SelectObject(hdc, GetStockObject(NULL_PEN));

    for (int i = 0; i < GRID_WIDTH; i++)
    {
        for (int j = 0; j < GRID_HEIGHT; j++)
        {
            if (g_Tile[j][i] == 1)                      // 1은 벽의 타일입니다.
            {
                int tileX = i * GRID_SIZE - g_Origin.x;
                int tileY = j * GRID_SIZE - g_Origin.y;

                Rectangle(hdc, tileX, tileY, tileX + GRID_SIZE, tileY + GRID_SIZE);
            }
            else if (g_Tile[j][i] == 2)                 // 2는 출발지의 타일입니다.
            {
                int tileX = i * GRID_SIZE - g_Origin.x;
                int tileY = j * GRID_SIZE - g_Origin.y;

                HBRUSH TempBrush = CreateSolidBrush(RGB(0, 0, 0));
                HBRUSH TempOldBrush = (HBRUSH)SelectObject(hdc, TempBrush);
                Rectangle(hdc, tileX, tileY, tileX + GRID_SIZE, tileY + GRID_SIZE);

                SelectObject(hdc, TempOldBrush);

                DeleteObject(TempBrush);
            }
            else if (g_Tile[j][i] == 3)                 // 3은 목적지의 타일입니다.
            {
                int tileX = i * GRID_SIZE - g_Origin.x;
                int tileY = j * GRID_SIZE - g_Origin.y;

                HBRUSH TempBrush = CreateSolidBrush(RGB(0, 0, 255));
                HBRUSH TempOldBrush = (HBRUSH)SelectObject(hdc, TempBrush);
                Rectangle(hdc, tileX, tileY, tileX + GRID_SIZE, tileY + GRID_SIZE);

                SelectObject(hdc, TempOldBrush);

                DeleteObject(TempBrush);
            }
            else if (g_Tile[j][i] == 4)                 // 4은 Open_List의 타일입니다.
            {
                int tileX = i * GRID_SIZE - g_Origin.x;
                int tileY = j * GRID_SIZE - g_Origin.y;

                HBRUSH TempBrush = CreateSolidBrush(RGB(255, 0, 255));
                HBRUSH TempOldBrush = (HBRUSH)SelectObject(hdc, TempBrush);
                Rectangle(hdc, tileX, tileY, tileX + GRID_SIZE, tileY + GRID_SIZE);

                SelectObject(hdc, TempOldBrush);

                DeleteObject(TempBrush);
            }
            else if (g_Tile[j][i] == 5)                 // 5는 Closed_List의 타일입니다.
            {
                int tileX = i * GRID_SIZE - g_Origin.x;
                int tileY = j * GRID_SIZE - g_Origin.y;

                HBRUSH TempBrush = CreateSolidBrush(RGB(51, 255, 255));
                HBRUSH TempOldBrush = (HBRUSH)SelectObject(hdc, TempBrush);
                Rectangle(hdc, tileX, tileY, tileX + GRID_SIZE, tileY + GRID_SIZE);

                SelectObject(hdc, TempOldBrush);

                DeleteObject(TempBrush);
            }
            else if (g_Tile[j][i] == 6)                 // 6은 최단거리의 타일입니다.
            {
                int tileX = i * GRID_SIZE - g_Origin.x;
                int tileY = j * GRID_SIZE - g_Origin.y;

                HBRUSH TempBrush = CreateSolidBrush(RGB(51, 255, 0));
                HBRUSH TempOldBrush = (HBRUSH)SelectObject(hdc, TempBrush);
                Rectangle(hdc, tileX, tileY, tileX + GRID_SIZE, tileY + GRID_SIZE);

                SelectObject(hdc, TempOldBrush);

                DeleteObject(TempBrush);
            }


        }
    }


    SelectObject(hdc, hOldBrush);
}



LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {

    case WM_CREATE:
    {
        HDC hdc = GetDC(hWnd);
        GetClientRect(hWnd, &g_MemDCRect);
        g_hMemDCBitmap = CreateCompatibleBitmap(hdc, g_MemDCRect.right, g_MemDCRect.bottom);
        g_hMemDC = CreateCompatibleDC(hdc);
        ReleaseDC(hWnd, hdc);
        g_hMemDCBitmap_old = (HBITMAP)SelectObject(g_hMemDC, g_hMemDCBitmap);



        g_hGridPen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
        g_hTileBrush = CreateSolidBrush(RGB(100, 100, 100));
        break;
    }
    case WM_LBUTTONDOWN:
    {
        
        if (g_bFirst)
        {
            int xPos = GET_X_LPARAM(lParam);
            int yPos = GET_Y_LPARAM(lParam);
            int iTileX = (xPos + g_Origin.x) / GRID_SIZE;
            int iTileY = (yPos + g_Origin.y) / GRID_SIZE;

            if (iTileX >= GRID_WIDTH || iTileY >= GRID_HEIGHT)
                break;

            if (g_Tile[iTileY][iTileX] == 1)
                g_bErase = true;
            else
                g_bErase = false;

            g_Tile[iTileY][iTileX] = 2; //start;

            Start_Pos.first = iTileX;
            Start_Pos.second = iTileY;

            InvalidateRect(hWnd, NULL, false);

            g_bFirst = !g_bFirst;
            break;
        }

        if (g_bFinal)
        {
            int yPos = GET_Y_LPARAM(lParam);
            int xPos = GET_X_LPARAM(lParam);
            int iTileX = (xPos + g_Origin.x) / GRID_SIZE;
            int iTileY = (yPos + g_Origin.y) / GRID_SIZE;

            if (iTileX >= GRID_WIDTH || iTileY >= GRID_HEIGHT)
                break;

            if (g_Tile[iTileY][iTileX] == 1)
                g_bErase = true;
            else
                g_bErase = false;

            g_Tile[iTileY][iTileX] = 3; //start;

            End_Pos.first = iTileX;
            End_Pos.second = iTileY;
            InvalidateRect(hWnd, NULL, false);
            g_bFinal = !g_bFinal;
            break;
        }

        g_bDrag = true;

        int xPos = GET_X_LPARAM(lParam);
        int yPos = GET_Y_LPARAM(lParam);
        int iTileX = (xPos + g_Origin.x) / GRID_SIZE;
        int iTileY = (yPos + g_Origin.y) / GRID_SIZE;

        if (iTileX >= GRID_WIDTH || iTileY >= GRID_HEIGHT)
            break;

        if (g_Tile[iTileY][iTileX] == 1)
            g_bErase = true;
        else
            g_bErase = false;

        g_Tile[iTileY][iTileX] = !g_bErase;
        InvalidateRect(hWnd, NULL, false);

        break;
    }
    case WM_LBUTTONUP:
    {
        g_bDrag = false;
        break;
    }

    case WM_MOUSEWHEEL:
    {

        int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
        int oldGridSize = GRID_SIZE;

        if (zDelta > 0) {
            GRID_SIZE += 1;
        }
        else if (zDelta < 0) {
            GRID_SIZE -= 1;
        }

        if (GRID_SIZE < 1) GRID_SIZE = 1;

        g_Origin.x = g_Origin.x * GRID_SIZE / oldGridSize;
        g_Origin.y = g_Origin.y * GRID_SIZE / oldGridSize;

        InvalidateRect(hWnd, NULL, false);
        break;
    }

    case WM_KEYDOWN:
    {
        bool bInvalidate = false; // 화면 갱신 여부를 나타내는 플래그

        if (wParam == 'R') {
            // R 키가 눌렸을 때 실행할 코드
            MessageBox(hWnd, L"Reset!", L"Key Press", MB_OK);
            memset(g_Tile, 0, sizeof(g_Tile));
            InvalidateRect(hWnd, NULL, false);
        }
        if (wParam == 'T') {
            // R 키가 눌렸을 때 실행할 코드

            g_Origin.x = 0;
            g_Origin.y = 0;
            InvalidateRect(hWnd, NULL, false);
        }

        switch (wParam)
        {
        case 'R':
            memset(g_Tile, 0, sizeof(g_Tile));
            bInvalidate = true;
            break;
        case 'T':
            g_Origin.x = 0;
            g_Origin.y = 0;
            bInvalidate = true;
            break;
        case 'Q':
            g_bFirst = !g_bFirst;
            break;
        case 'E':
            g_bFinal = !g_bFinal;
            break;
        case 'S':
        {
            g_bEndAStar = false;
            //Astar(g_Tile, Start_Pos, End_Pos, GRID_WIDTH, GRID_HEIGHT, hWnd);

            JPS(g_Tile, Start_Pos, End_Pos, GRID_WIDTH, GRID_HEIGHT, hWnd);
            InvalidateRect(hWnd, NULL, false);
            break;
        }
        case VK_LEFT:
            g_Origin.x -= GRID_SIZE;
            bInvalidate = true;
            break;
        case VK_RIGHT:
            g_Origin.x += GRID_SIZE;
            bInvalidate = true;
            break;
        case VK_UP:
            g_Origin.y -= GRID_SIZE;
            bInvalidate = true;
            break;
        case VK_DOWN:
            g_Origin.y += GRID_SIZE;
            bInvalidate = true;
            break;
        default:
            break;
        }

        if (bInvalidate)
        {
            InvalidateRect(hWnd, NULL, false);
        }
        break;
    }
    break;
    case WM_MOUSEMOVE:
    {
        if (g_bDrag)
        {
            int xPos = GET_X_LPARAM(lParam);
            int yPos = GET_Y_LPARAM(lParam);
            int iTileX = (xPos + g_Origin.x * GRID_SIZE) / GRID_SIZE;
            int iTileY = (yPos + g_Origin.y * GRID_SIZE) / GRID_SIZE;

            if (iTileX >= 0 && iTileX < GRID_WIDTH && iTileY >= 0 && iTileY < GRID_HEIGHT)
            {
                g_Tile[iTileY][iTileX] = !g_bErase;
                InvalidateRect(hWnd, NULL, false);
            }
        }
        break;
    }
    case WM_PAINT:
    {
        PatBlt(g_hMemDC, 0, 0, g_MemDCRect.right, g_MemDCRect.bottom, WHITENESS);
        RenderObstacle(g_hMemDC);
        RenderGrid(g_hMemDC);

        // 우아한 메뉴얼에 대해 출력합니다.
        int x = 0;
        int y = 0;
        int lineSpacing = 20;


       /* WCHAR Manual1[] = L" Start Position Setting : [Q]";
        WCHAR Manual2[] = L" End Position Setting : [E]";
        WCHAR Manual3[] = L" Reset Grid : [R]";
        WCHAR Manual4[] = L" Reset Focus : [T]";
        WCHAR Manual5[] = L" Start Astar : [S]";
        TextOutW(g_hMemDC, x, y, Manual1, wcslen(Manual1));
        TextOutW(g_hMemDC, x, y + 20, Manual2, wcslen(Manual2));
        TextOutW(g_hMemDC, x, y + 40, Manual3, wcslen(Manual3));
        TextOutW(g_hMemDC, x, y + 60, Manual4, wcslen(Manual4));
        TextOutW(g_hMemDC, x, y + 80, Manual5, wcslen(Manual5));*/


        /*if (g_bEndAStar)              // Astar 완료 여부 체크를 위한 디버그용 코드
        {
            WCHAR power[] = L"POWER";
            TextOutW(g_hMemDC, 0, 0, power, wcslen(power));
        }*/




        PAINTSTRUCT ps;
        HDC hdc = ::BeginPaint(hWnd, &ps);
        BitBlt(hdc, 0, 0, g_MemDCRect.right, g_MemDCRect.bottom, g_hMemDC, 0, 0, SRCCOPY);

        //BitBlt(hdc, 0, 0, g_MemDCRect.right, g_MemDCRect.bottom, g_hMemDC, g_Origin.x * GRID_SIZE, g_Origin.y * GRID_SIZE, SRCCOPY);





        ::EndPaint(hWnd, &ps);
        break;
    }

    case WM_DESTROY:
    {
        SelectObject(g_hMemDC, g_hMemDCBitmap_old);
        DeleteObject(g_hMemDC);
        DeleteObject(g_hMemDCBitmap);
        ::PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}




/*                                                          //
*                       JPS Data .......
*/                                                          //

typedef struct Node
{
    Node(int x, int y) : _x(x), _y(y)
    {
    }

    pair<int, int> GetPos()
    {
        return std::make_pair(_x, _y);
    }
    int _x, _y;
    int G, H;
    int Direction = 0;
    Node* parent = nullptr;
};


struct cmp
{
    bool operator()(const Node* First, const Node* Second)
    {
        //if (First->G + First->H == Second->G + Second->H)		// F = G + H, F가 낮은 순으로 정렬, 힙이니 > 이것이 맞음	
        //{
        //    return First->H > Second->H;						// 만약 H 값이 동일할 시, H가 낮은 순으로.
        //}
        //else
        {
            return First->G + First->H > Second->G + Second->H;	// 동일하지 않다면, F 값이 낮은 순으로, 등호는 > 이것이 맞음.
        }
    }
};


Node* Open_List_Find[100][100];
bool Close_Check[100][100] = { 0, };

vector<pair<int, int>> dir = {
    {0, -1},   // ↑    0
    {1, -1},   // ↗    1
    {1, 0},    // →    2
    {1, 1},    // ↘    3
    {0, 1},    // ↓    4
    {-1, 1},   // ↙    5
    {-1, 0},   // ←    6
    {-1, -1}   // ↖    7
};

enum DirName
{
    UP,
    UP_RIGHT,
    RIGHT,
    RIGHT_DOWN,
    DOWN,
    LEFT_DOWN,
    LEFT,
    LEFT_UP
};


// 탐색할 벽의 위치, 탐색할 비어이어야 하는 곳의 위치 ( 이는 함수 내부 주석 참조 ), 맵
bool Corner_Check(pair<int, int> WallPos, pair<int, int> EmptyPos, char(*Map)[100])
{
    // 판단 시도를 하는 Wall의 좌표가 0보단 크고, GRID_WIDTH( 맵 좌표상 이곳은 '\0') 보다 크지 않으며, Map 상에서 Tile 속성 1 ( 이는 벽을 의미 ) 인 경우
    if (WallPos.first >= 0 && WallPos.first < GRID_WIDTH && WallPos.second >= 0 && WallPos.second < GRID_HEIGHT && Map[WallPos.second][WallPos.first] == 1)
    {   // 판단 시도를 하는 비어있는 곳( 그래야 코너인지 탐색이 되니까, 그림 1을 참조 ) 맵의 범위 안이면서, Map 상에서 Tile 속성 0 ( 이는 비어있는 곳을 의미 ) 인 경우
        if (EmptyPos.first >= 0 && EmptyPos.first < GRID_WIDTH && EmptyPos.second >= 0 && EmptyPos.second < GRID_HEIGHT && Map[EmptyPos.second][EmptyPos.first] == 0)
        {
            return true;
        }
    }
    return false;

    
    /*
        그림 1. 직선 상에서 코너 탐색이란? 
        
        비어 있어야 하는 곳 = ★
        코너 탐색을 시도중인 곳 = ● 

        ㅁㅁㅁㅁㅁㅁㅁㅁㅁㅁ■★
        ㅁㅁㅁㅁㅁㅁㅁㅁㅁㅁ●
        ㅁㅁㅁㅁㅁㅁㅁㅁㅁㅁ■★
        
        이해가 가는가? 이 조건을 만족해야 코너라는 것이다.
    */
}

/*
       첫 번째 인자 : Node
       두 번째 인자 : 상우하좌(시계 방향)
*/
void Line_Search(Node* node, int Direction_Number, char(*Map)[100], priority_queue<Node*, vector<Node*>, cmp>* pq,
                    pair<int, int> start, pair<int, int> goal)                                                      // JPS 직선 탐색
{
    pair<int, int> pos = node->GetPos();        // 탐색 시작 노드의 위치
    bool breakCheck = true;

    int nx = pos.first;             // 일단 현재 좌표 가져오세요.
    int ny = pos.second;            // 일단 현재 좌표 가져오세요.
    int nextG = node->G;            // G 값 갱신 전에 현재 G 값 가져옴
    while (breakCheck)
    {
        
        nx += dir[Direction_Number].first;          // x가 먼저인 걸 항상 인지
        ny += dir[Direction_Number].second;         // x가 먼저인 걸 항상 인지
        nextG += 10;

        //if (!(nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT)) // 
        //{
        //    breakCheck = false;
        //    continue;
        //}

        if (nx < 0 || nx >= GRID_WIDTH || ny < 0 || ny >= GRID_HEIGHT)
        {
            breakCheck = false;
            continue;
        }

        if (Map[ny][nx] == 1)                                             // 해당 좌표가 벽이라면 return 유도
        {
            breakCheck = false;
            continue;
        }

        if (nx == goal.first && ny == goal.second)                        // 만약 그곳이 목적지라면 노드 만들고 return 유도, 만들어진 노드는 pq의 휴리스틱 정렬에 의해 빠르게 뽑힐 거임
        {
            Node* newNode = new Node(nx, ny);  // 코너가 탐색된 그 위치에 Pq.Push 하기 위한 목적
            newNode->parent = node;            // 만들어질 노드의 부모는 인자로 들어온 최초의 Node
            newNode->G = nextG;                // 지금까지 증가된 G 값을 가짐
            newNode->H = abs(goal.first - nx) + abs(goal.second - ny) * 10; 
            newNode->Direction = Direction_Number; // 목적지라면 더 이상 탐색은 안 하지만, 그래도, 인자로 받았으니까. 


            Open_List_Find[newNode->_y][newNode->_x] = newNode;

            // 이거 진짜 심각함, 대체 왜 원본 맵 파일과 랜더를 같이 쓰는거임? 나중에 분리 시켜야 함. 
            // 대처 방안 1. g_Tile을 랜더로 쓰고, 원본 맵에 대한 
            g_Tile[newNode->_y][newNode->_x] = 4;


            pq->push(newNode);
            breakCheck = false;
            continue;
        }


        {
            int wallNx = nx + dir[((Direction_Number - 2) + 8) % 8].first;
            int wallNY = ny + dir[((Direction_Number - 2) + 8) % 8].second;

            int EmptyNx = nx + dir[((Direction_Number - 1) + 8) % 8].first;
            int EmptyNY = ny + dir[((Direction_Number - 1) + 8) % 8].second;


            if (Corner_Check(make_pair(wallNx, wallNY), make_pair(EmptyNx, EmptyNY), Map))  // 직선의 왼쪽 벽과 비어있는 곳 체크해서 참이면
            {
                Node* newNode = new Node(nx, ny);  // 코너가 탐색된 그 위치에 Pq.Push 하기 위한 목적, 직선은 그 자리에 바로 만든다.
                newNode->parent = node;
                newNode->G = nextG;
                newNode->H = abs(goal.first - nx) + abs(goal.second - ny) * 10;
                newNode->Direction = Direction_Number;

                
                Open_List_Find[newNode->_y][newNode->_x] = newNode;                         // Open_List_Find 사용 목적 -> 이미 node가 만들어졌다면, F값이 더 높은 경우에는
                                                                                            // 노드를 만들지 않을 목적.

                g_Tile[newNode->_y][newNode->_x] = 4;                                       // Open List 속성

                pq->push(newNode);

                breakCheck = false;
                continue;
            }
        }

        {
            int wallNx = nx + dir[((Direction_Number + 2) + 8) % 8].first;
            int wallNY = ny + dir[((Direction_Number + 2) + 8) % 8].second;

            int EmptyNx = nx + dir[((Direction_Number + 1) + 8) % 8].first;
            int EmptyNY = ny + dir[((Direction_Number + 1) + 8) % 8].second;


            if (Corner_Check(make_pair(wallNx, wallNY), make_pair(EmptyNx, EmptyNY), Map))  // 직선의 오른쪽 벽과 비어있어야 할 곳(우측 벽 앞)을 탐색해서 코너라고 판단이 된다면.
            {
                Node* newNode = new Node(nx, ny);  // 코너가 탐색된 그 위치에 Pq.Push 하기 위한 목적, 직선은 코너인 곳에 바로 노드를 만든다.
                newNode->parent = node;
                newNode->G = nextG;
                newNode->H = abs(goal.first - nx) + abs(goal.second - ny) * 10;
                newNode->Direction = Direction_Number;

                
                Open_List_Find[newNode->_y][newNode->_x] = newNode;

                g_Tile[newNode->_y][newNode->_x] = 4;                                       // Open_List 속성

                pq->push(newNode);
                breakCheck = false;
                continue;
            }
        }
            

    }

    return; // 노드를 만들었다면 While 문 안에서 만들었을 거임
}
 

// Sub_Search란, 대각선의 보조 탐색인 ( 대각을 제외한 2개의 방향, 그림 2 참조 )
bool Sub_Search(pair<int, int> pos, int Direction_Number, char(*Map)[100], priority_queue<Node*, vector<Node*>, cmp>* pq,
    pair<int, int> start, pair<int, int> goal) // JPS 대각 보조 탐색
{

    /*
        그림 2. 대각선의 보조 탐색이란?

        이미 이동한 대각선 탐색 = ↗
        현재 이동 위치 대각선 탐색 = ■
        대각 보조 탐색 = ●

        ● ● ● ㅁㅁㅁㅁㅁㅁㅁ
        ● ● ● ㅁㅁㅁㅁㅁㅁㅁ
        ● ● ■ ㅁㅁㅁㅁㅁㅁㅁ
        ● ↗ ● ● ● ● ● ● ● ●
        ↗ ● ● ● ● ● ● ● ●

       대각은 한 칸 이동할 때마다, 보조 탐색을 진행한다.

       이 함수는 대각에 코너가 존재하면, True를 반환한다.

       그러면 직선 탐색이랑 다른 점은 무엇이냐, True만 반환하여서 대각 탐색한 위치에서 Node를 생성하게 유도한다. 
    */

    bool breakCheck = true;

    int nx = pos.first;
    int ny = pos.second;
    while (breakCheck)
    {

        nx += dir[Direction_Number].first;
        ny += dir[Direction_Number].second;


        if (nx < 0 || nx >= GRID_WIDTH || ny < 0 || ny >= GRID_HEIGHT)
        {
            breakCheck = false;
            continue;
        }


        if (Map[ny][nx] == 1)
        {
            breakCheck = false;
            continue;
        }

        if (nx == goal.first && ny == goal.second)              // 대각의 위치에 노드를 만들거고, 그 (뽑힌 노드)가 주 탐색(직선 2개)를 진행해서 도착지 노드를 만들 것입니다.
        {
            return true;
        }


        {
            int wallNx = nx + dir[((Direction_Number - 2) + 8) % 8].first;
            int wallNY = ny + dir[((Direction_Number - 2) + 8) % 8].second;

            int EmptyNx = nx + dir[((Direction_Number - 1) + 8) % 8].first;
            int EmptyNY = ny + dir[((Direction_Number - 1) + 8) % 8].second;


            if (Corner_Check(make_pair(wallNx, wallNY), make_pair(EmptyNx, EmptyNY), Map))  // 직선의 코너 왼쪽
            {
                return true;
            }
        }

        {
            int wallNx = nx + dir[((Direction_Number + 2) + 8) % 8].first;
            int wallNY = ny + dir[((Direction_Number + 2) + 8) % 8].second;

            int EmptyNx = nx + dir[((Direction_Number + 1) + 8) % 8].first;
            int EmptyNY = ny + dir[((Direction_Number + 1) + 8) % 8].second;


            if (Corner_Check(make_pair(wallNx, wallNY), make_pair(EmptyNx, EmptyNY), Map))  // 직선의 코너 오른쪽
            {
                return true;
            }
        }


    }

    return false;
}

void Diagonal_Search(Node* node, int Direction_Number, char(*Map)[100], priority_queue<Node*, vector<Node*>, cmp>* pq,
    pair<int, int> start, pair<int, int> goal) // JPS 대각 탐색
{
    pair<int, int> pos = node->GetPos();
    bool breakCheck = true;

    int nx = pos.first;
    int ny = pos.second;
    int nextG = node->G;
    while (breakCheck)
    {

        nx += dir[Direction_Number].first;
        ny += dir[Direction_Number].second;
        nextG += 14;

        if (!(nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT))
        {
            breakCheck = false;
            continue;
        }

        if (Map[ny][nx] == 1)
        {
            breakCheck = false;
            continue;
        }

        if (nx == goal.first && ny == goal.second)
        {
            Node* newNode = new Node(nx, ny);  // 코너가 탐색된 그 위치에 Pq.Push 하기 위한 목적
            newNode->parent = node;
            newNode->G = nextG;
            newNode->H = abs(goal.first - nx) + abs(goal.second - ny) * 10;
            newNode->Direction = Direction_Number;


            Open_List_Find[newNode->_y][newNode->_x] = newNode;

            g_Tile[newNode->_y][newNode->_x] = 4;

            pq->push(newNode);


            breakCheck = false;
            continue;
        }

        {
            int wallNx = nx + dir[((Direction_Number - 3) + 8) % 8].first;
            int wallNY = ny + dir[((Direction_Number - 3) + 8) % 8].second;

            int EmptyNx = nx + dir[((Direction_Number - 2) + 8) % 8].first;
            int EmptyNY = ny + dir[((Direction_Number - 2) + 8) % 8].second;


            if (Corner_Check(make_pair(wallNx, wallNY), make_pair(EmptyNx, EmptyNY), Map))  // 직선의 코너 왼쪽
            {
                Node* newNode = new Node(nx, ny);  // 코너가 탐색된 그 위치에 Pq.Push 하기 위한 목적
                newNode->parent = node;
                newNode->G = nextG;
                newNode->H = abs(goal.first - nx) + abs(goal.second - ny) * 10;
                newNode->Direction = Direction_Number;

                
                Open_List_Find[newNode->_y][newNode->_x] = newNode;

                g_Tile[newNode->_y][newNode->_x] = 4;

                pq->push(newNode);
                breakCheck = false;
                continue;
            }
        }

        {
            int wallNx = nx + dir[((Direction_Number + 3) + 8) % 8].first;
            int wallNY = ny + dir[((Direction_Number + 3) + 8) % 8].second;

            int EmptyNx = nx + dir[((Direction_Number + 2) + 8) % 8].first;
            int EmptyNY = ny + dir[((Direction_Number + 2) + 8) % 8].second;


            if (Corner_Check(make_pair(wallNx, wallNY), make_pair(EmptyNx, EmptyNY), Map))  // 대각의 코너 체크
            {
                Node* newNode = new Node(nx, ny);  // 코너가 탐색된 그 위치에 Pq.Push 하기 위한 목적
                newNode->parent = node;
                newNode->G = nextG;
                newNode->H = abs(goal.first - nx) + abs(goal.second - ny) * 10;
                newNode->Direction = Direction_Number;

                
                Open_List_Find[newNode->_y][newNode->_x] = newNode;

                g_Tile[newNode->_y][newNode->_x] = 4;

                pq->push(newNode);
                breakCheck = false;
                continue;
            }
        }

        if (Sub_Search(make_pair(nx, ny), (((Direction_Number - 1) + 8) % 8), Map, pq, start, goal))     // 대각 보조 탐색 
        {

            Node* newNode = new Node(nx, ny);  // 코너가 탐색된 그 위치에 Pq.Push 하기 위한 목적
            newNode->parent = node;
            newNode->G = nextG;
            newNode->H = abs(goal.first - nx) + abs(goal.second - ny) * 10;
            newNode->Direction = Direction_Number;

            
            Open_List_Find[newNode->_y][newNode->_x] = newNode;

            g_Tile[newNode->_y][newNode->_x] = 4;

            pq->push(newNode);
            breakCheck = false;
            continue;
        }

        if (Sub_Search(make_pair(nx, ny), (((Direction_Number + 1) + 8) % 8), Map, pq, start, goal))     // 대각 보조 탐색
        {
            Node* newNode = new Node(nx, ny);  // 코너가 탐색된 그 위치에 Pq.Push 하기 위한 목적
            newNode->parent = node;
            newNode->G = nextG;
            newNode->H = abs(goal.first - nx) + abs(goal.second - ny) * 10;
            newNode->Direction = Direction_Number;

            
            Open_List_Find[newNode->_y][newNode->_x] = newNode;

            g_Tile[newNode->_y][newNode->_x] = 4;

            pq->push(newNode);
            breakCheck = false;
            continue;
        }

    }
}

int JPS(char(*Map)[100], pair<int, int> start, pair<int, int> goal, int max_X, int max_Y, HWND hWnd)
{
    priority_queue<Node*, vector<Node*>, cmp> pq;
                
    // Open_List_Check[100][100]은 현재 전역에 선언되어 있음.
    vector<Node*> Close_List;

    int dx[8] = { 0,1,0,-1,1,1,-1,-1 };				// 방향좌표 ↑→↓←↘↗↙↖   
    int dy[8] = { -1,0,1,0,1,-1,1,-1 };

    Node* startNode = new Node(start.first, start.second);
    startNode->G = 0;
    startNode->H = (abs(goal.first - startNode->_x) + abs(goal.second - startNode->_y)) * 10;
    startNode->parent = nullptr;

    Close_Check[startNode->_y][startNode->_x] = 1;  // 1: closelist
    Open_List_Find[startNode->_y][startNode->_x] = startNode;




    g_Tile[startNode->_y][startNode->_x] = 4;   // 4 : 오픈 리스트

    InvalidateRect(hWnd, NULL, false);

    for (int i = 0; i < 8; i++)
    {
        if (i % 2 == 0)
        {
            Line_Search(startNode, i, Map, &pq, start, goal);
        }
        else
        {
            Diagonal_Search(startNode, i, Map, &pq, start, goal);
        }
    }

    
    
    while (!pq.empty())
    {
        Node* Cur = pq.top();

        Close_List.push_back(Cur);

        pq.pop();

        if (Close_Check[Cur->_y][Cur->_x] != 0)
        {
            continue;
        }

        Close_Check[Cur->_y][Cur->_x] = true;

        if (Cur->_x == goal.first && Cur->_y == goal.second)
        {
            break; // Goal의 first는 x 좌표, 이 cpp의 모든 코드는 x가 먼저. 2차원 bool 배열은 예외. ex.. bool map[y][x]
        }
           


        g_Tile[startNode->_y][startNode->_x] = 5;   // 5: closelist
        InvalidateRect(hWnd, NULL, false);


        if (Cur->Direction % 2 == 0)
        {
            Line_Search(Cur, Cur->Direction, Map, &pq, start, goal);

            {
                int wallNx = Cur->_x + dir[((Cur->Direction - 2) + 8) % 8].first;
                int wallNY = Cur->_y + dir[((Cur->Direction - 2) + 8) % 8].second;

                int EmptyNx = Cur->_x + dir[((Cur->Direction - 1) + 8) % 8].first;
                int EmptyNY = Cur->_y + dir[((Cur->Direction - 1) + 8) % 8].second;


                if (Corner_Check(make_pair(wallNx, wallNY), make_pair(EmptyNx, EmptyNY), Map))  // 직선의 코너 왼쪽
                {
                    Diagonal_Search(Cur, ((Cur->Direction - 1) + 8) % 8, Map, &pq, start, goal);
                }
            }

            {
                int wallNx = Cur->_x + dir[((Cur->Direction + 2) + 8) % 8].first;
                int wallNY = Cur->_y + dir[((Cur->Direction + 2) + 8) % 8].second;

                int EmptyNx = Cur->_x + dir[((Cur->Direction + 1) + 8) % 8].first;
                int EmptyNY = Cur->_y + dir[((Cur->Direction + 1) + 8) % 8].second;


                if (Corner_Check(make_pair(wallNx, wallNY), make_pair(EmptyNx, EmptyNY), Map))  // 직선의 코너 오른쪽
                {
                    Diagonal_Search(Cur, ((Cur->Direction + 1) + 8) % 8, Map, &pq, start, goal);
                }
            }

            
        }
        else // 대각선 노드라면
        {
            Diagonal_Search(Cur, Cur->Direction, Map, &pq, start, goal);             // 먼저 대각 서치
            Line_Search(Cur, ((Cur->Direction + 1) + 8) % 8, Map, &pq, start, goal); // 이후 뽑힌 대각은 어느 코너에서 만들어 진 것인지 알아야 할 수도 있음.
            Line_Search(Cur, ((Cur->Direction - 1) + 8) % 8, Map, &pq, start, goal);

            if (Cur->_x == 3 && Cur->_y == 1)
            {
                int a = 1;
            }

            {
                int wallNx = Cur->_x + dir[((Cur->Direction - 3) + 8) % 8].first;
                int wallNY = Cur->_y + dir[((Cur->Direction - 3) + 8) % 8].second;

                int EmptyNx = Cur->_x + dir[((Cur->Direction - 2) + 8) % 8].first;
                int EmptyNY = Cur->_y + dir[((Cur->Direction - 2) + 8) % 8].second;


                if (Corner_Check(make_pair(wallNx, wallNY), make_pair(EmptyNx, EmptyNY), Map))  // 직선의 코너 왼쪽
                {
                    Diagonal_Search(Cur, ((Cur->Direction - 2) + 8) % 8, Map, &pq, start, goal);
                }
            }

            {
                
                int wallNx = Cur->_x + dir[((Cur->Direction + 3) + 8) % 8].first;
                int wallNY = Cur->_y + dir[((Cur->Direction + 3) + 8) % 8].second;

                int EmptyNx = Cur->_x + dir[((Cur->Direction + 2) + 8) % 8].first;
                int EmptyNY = Cur->_y + dir[((Cur->Direction + 2) + 8) % 8].second;


                if (Corner_Check(make_pair(wallNx, wallNY), make_pair(EmptyNx, EmptyNY), Map))  // 직선의 코너 오른쪽
                {
                    Diagonal_Search(Cur, ((Cur->Direction + 2) + 8) % 8, Map, &pq, start, goal);
                }
            }
        }

        
    }

    g_Tile[start.second][start.first] = 2;
    g_Tile[goal.second][goal.first] = 3;


    //최단 경로를 타고 가면서, 경로를 원복함
    Node* li = Close_List.back();


    while (li->parent != nullptr)           //  
    {
        g_Tile[li->parent->_y][li->parent->_x] = 6;
        li = li->parent;
    }

    
    

    std::fill(&Open_List_Find[0][0], &Open_List_Find[0][0] + 100 * 100, nullptr);  // 사이즈 계산 주의

    return 0;
}