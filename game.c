#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
enum MOVE { ARROW, WASD };
struct coordinate {
    int x, y;
} Player, Exit;
// 遊戲狀態結構
typedef struct {
    int mazeSize;
    int** maze;
    struct coordinate Player, Exit;
    int cellSize;
    int layer;
    enum MOVE move;
    SDL_Texture* wallTexture;
    SDL_Texture* playerTexture;
    SDL_Texture* exitTexture;
} GameState;
// 函數聲明
void initSDL(SDL_Window** window, SDL_Renderer** renderer);
int showmessage(const char* s, SDL_Window* window);
void initGame(GameState* game, SDL_Window* window, SDL_Renderer* renderer);
int canVisit(GameState* game, int x, int y);
void createMaze(GameState* game, int x, int y);
void handleInput(GameState* game,
                 SDL_Event* event,
                 SDL_Renderer* renderer,
                 int* quit);
void render(SDL_Renderer* renderer, GameState* game);
void cleanup(SDL_Window* window, SDL_Renderer* renderer, GameState* game);
SDL_Texture* loadTexture(SDL_Renderer* renderer, const char* path);
int sizeMenu(SDL_Window* window, SDL_Renderer* renderer);
int moveMenu(SDL_Window* window, SDL_Renderer* renderer);
void setrender(SDL_Renderer* renderer, int size);
void save(GameState* game);
int read(GameState* game, SDL_Window* window);
int playmusic(Mix_Chunk* sound, const char* path);
int setmaze(GameState* game);
// 主函數
int main(int argc, char* argv[]) {
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    Mix_Chunk* sound = NULL;
    GameState game;
    SDL_Event event;
    char message[100];
    int quit = 0;
    int channel;
    srand(time(NULL));
    // 初始化 SDL
    initSDL(&window, &renderer);
    channel = playmusic(sound, "sound\\begin.ogg");
    if (showmessage(u8"是否還原進度", window)) {
        if (!read(&game, window)) {
            // 初始化遊戲
            initGame(&game, window, renderer);
        }
    } else {
        // 初始化遊戲
        initGame(&game, window, renderer);
    }
    setrender(renderer, game.mazeSize);
    // 載入圖片
    game.wallTexture = loadTexture(renderer, "picture\\brick.png");
    game.playerTexture = loadTexture(renderer, "picture\\pl_r.png");
    game.exitTexture = loadTexture(renderer, "picture\\door.png");
    Mix_FadeOutChannel(channel, 2000);  // 用 2秒漸弱停止
    Mix_FreeChunk(sound);               // 釋放舊的音效資源
    channel = playmusic(sound, "sound\\bgm.ogg");
    // 主循環
    while (!quit) {
        while (SDL_PollEvent(&event)) {
            handleInput(&game, &event, renderer, &quit);
            if (game.layer % 5 == 0) {
                SDL_DestroyTexture(game.wallTexture);
                game.wallTexture = loadTexture(renderer, "picture\\brick1.png");
            } else {
                SDL_DestroyTexture(game.wallTexture);
                game.wallTexture = loadTexture(renderer, "picture\\brick.png");
            }
        }
        if (game.Player.x == game.Exit.x && game.Player.y == game.Exit.y) {
            snprintf(message, sizeof(message), u8"進入下一層(第%d層)",
                     ++game.layer);
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, u8"恭喜",
                                     message, window);
            setmaze(&game);
        }
        render(renderer, &game);
        if (quit) {
            if (showmessage(u8"確定退出遊戲?", window))
                ;
            else {
                quit = 0;
            }
        }
    }
    if (showmessage(u8"是否存檔", window))
        save(&game);
    cleanup(window, renderer, &game);
    return 0;
}
int read(GameState* game, SDL_Window* window) {
    FILE* fp = fopen("data.txt", "r");
    if (fp == NULL) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, u8"警告", u8"無此檔案",
                                 window);
        return 0;
    }
    fscanf(fp, "%d %d %d %d %d %d %d %d", &game->mazeSize, &game->Player.x,
           &game->Player.y, &game->Exit.x, &game->Exit.y, &game->cellSize,
           &game->layer, &game->move);
    game->maze = malloc(sizeof(int*) * game->mazeSize);
    for (int i = 0; i < game->mazeSize; i++) {
        game->maze[i] = (int*)malloc(sizeof(int) * game->mazeSize);
    }
    for (int i = 0; i < game->mazeSize; i++) {
        for (int j = 0; j < game->mazeSize; j++)
            fscanf(fp, "%d", &game->maze[i][j]);
    }
    fclose(fp);
    return 1;
}
void save(GameState* game) {
    FILE* fp = fopen("data.txt", "w");
    fprintf(fp, "%d %d %d %d %d %d %d %d\n", game->mazeSize, game->Player.x,
            game->Player.y, game->Exit.x, game->Exit.y, game->cellSize,
            game->layer, game->move);
    for (int i = 0; i < game->mazeSize; i++) {
        for (int j = 0; j < game->mazeSize; j++)
            fprintf(fp, "%d ", game->maze[i][j]);
    }

    fclose(fp);
}
// 初始化 SDL
void initSDL(SDL_Window** window, SDL_Renderer** renderer) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL 初始化失敗: %s\n", SDL_GetError());
        exit(1);
    }
    *window = SDL_CreateWindow(
        u8"迷宮遊戲", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!*window) {
        printf("窗口創建失敗: %s\n", SDL_GetError());
        exit(1);
    }
    *renderer = SDL_CreateRenderer(
        *window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!*renderer) {
        printf("渲染器創建失敗: %s\n", SDL_GetError());
        exit(1);
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        printf("SDL_image 初始化失敗: %s\n", IMG_GetError());
        exit(1);
    }
    // 初始化 SDL_ttf
    if (TTF_Init() == -1) {
        printf("TTF_Init 初始化失敗: %s\n", TTF_GetError());
        exit(1);
    }
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        exit(1);
    }
    // 初始化 SDL_mixer
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) < 0) {
        printf("SDL_mixer could not initialize! Mix_Error: %s\n",
               Mix_GetError());
        exit(1);
    }
}
int playmusic(Mix_Chunk* sound, const char* path) {
    sound = Mix_LoadWAV(path);
    if (!sound) {
        printf("Failed to load sound ! Mix_Error: %s\n", Mix_GetError());
        Mix_CloseAudio();
        SDL_Quit();
    }
    return Mix_PlayChannel(-1, sound, -1);
}
// 初始化遊戲
void initGame(GameState* game, SDL_Window* window, SDL_Renderer* renderer) {
    // 顯示選單並獲取地圖大小
    game->mazeSize = sizeMenu(window, renderer);
    // 顯示選單並獲取移動方式
    game->move = moveMenu(window, renderer);
    game->layer = 1;
    // 設定迷宮大小
    game->cellSize = WINDOW_WIDTH / game->mazeSize;
    game->maze = malloc(sizeof(int*) * game->mazeSize);
    for (int i = 0; i < game->mazeSize; i++) {
        game->maze[i] = malloc(sizeof(int) * game->mazeSize);
    }
    setmaze(game);
}
// 判斷是否能訪問
int canVisit(GameState* game, int x, int y) {
    return x > 0 && x < game->mazeSize - 1 && y > 0 && y < game->mazeSize - 1 &&
           game->maze[y][x] == 1;
}
// 生成迷宮
void createMaze(GameState* game, int x, int y) {
    game->maze[y][x] = 0;
    int dir[4][2] = {{0, -2}, {0, 2}, {-2, 0}, {2, 0}};
    for (int i = 0; i < 4; i++) {
        int r = rand() % 4;
        int tempX = dir[i][0];
        int tempY = dir[i][1];
        dir[i][0] = dir[r][0];
        dir[i][1] = dir[r][1];
        dir[r][0] = tempX;
        dir[r][1] = tempY;
    }
    for (int i = 0; i < 4; i++) {
        int nx = x + dir[i][0];
        int ny = y + dir[i][1];
        int wx = x + dir[i][0] / 2;
        int wy = y + dir[i][1] / 2;
        if (canVisit(game, nx, ny)) {
            game->maze[wy][wx] = 0;
            createMaze(game, nx, ny);
        }
    }
}
int setmaze(GameState* game) {
    // 初始化迷宮為牆
    for (int i = 0; i < game->mazeSize; i++) {
        for (int j = 0; j < game->mazeSize; j++) {
            game->maze[i][j] = 1;
        }
    }
    // 設置玩家初始位置
    game->Player.x = (rand() % (game->mazeSize / 2)) * 2 + 1;
    game->Player.y = (rand() % (game->mazeSize / 2)) * 2 + 1;
    // 生成迷宮
    createMaze(game, game->Player.x, game->Player.y);
    // 設置出口
    do {
        game->Exit.x = (rand() % (game->mazeSize / 2)) * 2 + 1;
        game->Exit.y = (rand() % (game->mazeSize / 2)) * 2 + 1;
    } while (
        (game->Exit.x == game->Player.x && game->Exit.y == game->Player.y) ||
        game->maze[game->Exit.y][game->Exit.x] != 0);
}
// 處理輸入
void handleInput(GameState* game,
                 SDL_Event* event,
                 SDL_Renderer* renderer,
                 int* quit) {
    if (event->type == SDL_QUIT) {
        *quit = 1;
    } else if (event->type == SDL_KEYDOWN) {
        int nextX = game->Player.x;
        int nextY = game->Player.y;
        SDL_Texture* newTexture = NULL;
        if (game->move == ARROW) {
            switch (event->key.keysym.sym) {
                case SDLK_UP:
                    nextY--;
                    break;
                case SDLK_DOWN:
                    nextY++;
                    break;
                case SDLK_LEFT:  // 左
                    nextX--;
                    newTexture = loadTexture(renderer, "picture\\pl_l.png");
                    break;
                case SDLK_RIGHT:  // 右
                    nextX++;
                    newTexture = loadTexture(renderer, "picture\\pl_r.png");
                    break;
                case SDLK_ESCAPE:
                    *quit = 1;
                    break;
            }
        } else if (game->move == WASD) {
            switch (event->key.keysym.sym) {
                case SDLK_w:
                    nextY--;
                    break;
                case SDLK_s:
                    nextY++;
                    break;
                case SDLK_a:  // 左
                    nextX--;
                    newTexture = loadTexture(renderer, "picture\\pl_l.png");
                    break;
                case SDLK_d:  // 右
                    nextX++;
                    newTexture = loadTexture(renderer, "picture\\pl_r.png");
                    break;
                case SDLK_ESCAPE:
                    *quit = 1;
                    break;
            }
        }
        if (newTexture != NULL) {
            if (game->playerTexture != NULL) {
                SDL_DestroyTexture(game->playerTexture);  // 釋放舊的圖片
            }
            game->playerTexture = newTexture;
        }
        if (nextX >= 0 && nextX < game->mazeSize && nextY >= 0 &&
            nextY < game->mazeSize && game->maze[nextY][nextX] == 0) {
            game->Player.x = nextX;
            game->Player.y = nextY;
        }
    }
}
// 渲染遊戲
void render(SDL_Renderer* renderer, GameState* game) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    for (int y = 0; y < game->mazeSize; y++) {
        for (int x = 0; x < game->mazeSize; x++) {
            SDL_Rect rect = {x * game->cellSize, y * game->cellSize,
                             game->cellSize, game->cellSize};
            if (x == game->Player.x && y == game->Player.y) {
                SDL_RenderCopy(renderer, game->playerTexture, NULL, &rect);
            } else if (x == game->Exit.x && y == game->Exit.y) {
                SDL_RenderCopy(renderer, game->exitTexture, NULL, &rect);
            } else if (game->maze[y][x] == 1) {
                SDL_RenderCopy(renderer, game->wallTexture, NULL, &rect);
            }
        }
    }
    SDL_RenderPresent(renderer);
}
// 清理資源
void cleanup(SDL_Window* window, SDL_Renderer* renderer, GameState* game) {
    SDL_DestroyTexture(game->wallTexture);
    SDL_DestroyTexture(game->playerTexture);
    SDL_DestroyTexture(game->exitTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    for (int i = 0; i < game->mazeSize; i++) {
        free(game->maze[i]);
    }
    free(game->maze);
}
// 載入圖片
SDL_Texture* loadTexture(SDL_Renderer* renderer, const char* path) {
    SDL_Surface* surface = IMG_Load(path);
    if (!surface) {
        printf("圖片載入失敗: %s\n", IMG_GetError());
        return NULL;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture) {
        printf("創建紋理失敗: %s\n", SDL_GetError());  // 輸出具體錯誤訊息
    }
    return texture;
}
// 顯示選單
int sizeMenu(SDL_Window* window, SDL_Renderer* renderer) {
    SDL_Event event;
    int selected = 0;  // 0: 第一選項，1: 第二選項，2: 第三選項
    int size = 0;
    // 載入背景圖片
    SDL_Texture* background = loadTexture(renderer, "picture\\ba.png");
    if (!background) {
        printf("無法加載背景圖像: %s\n", IMG_GetError());
        return -1;
    }
    // 載入字型
    TTF_Font* titleFont = TTF_OpenFont("TTF\\title.ttf", 48);
    if (!titleFont) {
        printf("無法加載標題字型: %s\n", TTF_GetError());
        SDL_DestroyTexture(background);
        return -1;
    }
    TTF_Font* font = TTF_OpenFont("TTF\\option.ttf", 24);
    if (!font) {
        printf("無法加載選項字型: %s\n", TTF_GetError());
        SDL_DestroyTexture(background);
        TTF_CloseFont(titleFont);
        return -1;
    }
    // 主迴圈
    while (1) {
        // 獲取視窗大小
        int winWidth, winHeight;
        SDL_GetWindowSize(window, &winWidth, &winHeight);
        // 動態調整背景大小
        SDL_Rect bgRect = {0, 0, winWidth, winHeight};
        // 清空畫面
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        // 渲染背景
        SDL_RenderCopy(renderer, background, NULL, &bgRect);
        // 渲染標題
        SDL_Color red = {238, 64, 0, 255};
        SDL_Surface* titleSurface =
            TTF_RenderUTF8_Blended(titleFont, u8"迷宮遊戲", red);
        SDL_Texture* titleTexture =
            SDL_CreateTextureFromSurface(renderer, titleSurface);
        SDL_Rect titleRect = {winWidth / 2 - 200, winHeight / 10, 400, 100};
        SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);
        SDL_FreeSurface(titleSurface);
        SDL_DestroyTexture(titleTexture);
        // 渲染提示
        SDL_Color white = {255, 255, 255, 255};
        SDL_Surface* hintSurface =
            TTF_RenderUTF8_Blended(font, u8"選擇地圖大小", white);
        SDL_Texture* hintTexture =
            SDL_CreateTextureFromSurface(renderer, hintSurface);
        SDL_Rect hintRect = {winWidth / 2 - 200, winHeight / 10 + 150, 400, 50};
        SDL_RenderCopy(renderer, hintTexture, NULL, &hintRect);
        SDL_FreeSurface(hintSurface);
        SDL_DestroyTexture(hintTexture);
        // 選項文字
        const char* options[] = {" 21 x 21", " 33 x 33", " 41 x 41"};
        // 渲染選項
        for (int i = 0; i < 3; i++) {
            SDL_Color color = (i == selected) ? (SDL_Color){255, 255, 0, 255}
                                              : white;  // 高亮當前選項
            SDL_Surface* optSurface =
                TTF_RenderUTF8_Blended(font, options[i], color);
            SDL_Texture* optTexture =
                SDL_CreateTextureFromSurface(renderer, optSurface);
            SDL_Rect optionRect = {winWidth / 2 - 200,
                                   winHeight / 8 + 200 + i * 100, 400, 50};
            SDL_RenderCopy(renderer, optTexture, NULL, &optionRect);
            SDL_FreeSurface(optSurface);
            SDL_DestroyTexture(optTexture);
        }
        SDL_RenderPresent(renderer);
        // 處理事件
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                exit(1);
            }
            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_UP:
                    case SDLK_w:
                        selected = (selected + 2) % 3;  // 往上選
                        break;
                    case SDLK_DOWN:
                    case SDLK_s:
                        selected = (selected + 1) % 3;  // 往下選
                        break;
                    case SDLK_RETURN:  // 按 Enter 確認選擇
                        size = (selected == 0) ? 21 : (selected == 1) ? 33 : 41;
                        break;
                }
            }
        }
        if (size == 21 || size == 33 || size == 41)
            break;
    }
    SDL_DestroyTexture(background);
    TTF_CloseFont(titleFont);
    TTF_CloseFont(font);
    return size;
}
int moveMenu(SDL_Window* window, SDL_Renderer* renderer) {
    int selected = 0;  // 0 代表方向鍵，1 代表選項 WASD
    SDL_Event event;
    int quit = 0;
    // 載入背景和選項圖片
    SDL_Texture* bgTexture = loadTexture(renderer, "picture\\bg.png");
    SDL_Texture* op1Texture = loadTexture(renderer, "picture\\arrow.png");
    SDL_Texture* op2Texture = loadTexture(renderer, "picture\\wasd.png");
    if (!bgTexture || !op1Texture || !op2Texture) {
        return -1;  // 若載入失敗，直接返回
    }
    // 載入字型
    TTF_Font* titleFont = TTF_OpenFont("TTF\\title.ttf", 48);
    if (!titleFont) {
        printf("無法加載標題字型: %s\n", TTF_GetError());
        SDL_DestroyTexture(bgTexture);
        return -1;
    }
    TTF_Font* font = TTF_OpenFont("TTF\\option.ttf", 36);
    if (!font) {
        printf("無法加載選項字型: %s\n", TTF_GetError());
        SDL_DestroyTexture(bgTexture);
        TTF_CloseFont(titleFont);
        return -1;
    }
    while (!quit) {
        int winWidth, winHeight;
        SDL_GetWindowSize(window, &winWidth, &winHeight);
        // 繪製畫面
        SDL_RenderClear(renderer);
        // 繪製背景
        SDL_RenderCopy(renderer, bgTexture, NULL, NULL);
        // 渲染標題
        SDL_Color red = {238, 64, 0, 255};
        SDL_Surface* titleSurface =
            TTF_RenderUTF8_Blended(titleFont, u8"迷宮遊戲", red);
        SDL_Texture* titleTexture =
            SDL_CreateTextureFromSurface(renderer, titleSurface);
        SDL_Rect titleRect = {winWidth / 2 - 200, winHeight / 10, 400, 100};
        SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);
        SDL_FreeSurface(titleSurface);
        SDL_DestroyTexture(titleTexture);
        // 渲染提示
        SDL_Color white = {255, 255, 255, 255};
        SDL_Surface* hintSurface =
            TTF_RenderUTF8_Blended(font, u8"選擇移動方式", white);
        SDL_Texture* hintTexture =
            SDL_CreateTextureFromSurface(renderer, hintSurface);
        SDL_Rect hintRect = {winWidth / 2 - 200, winHeight / 10 + 150, 400, 50};
        SDL_RenderCopy(renderer, hintTexture, NULL, &hintRect);
        SDL_FreeSurface(hintSurface);
        SDL_DestroyTexture(hintTexture);
        // 繪製高亮框
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 32);  // 黃色很透明
        SDL_Rect highlightRect =
            selected == 0
                ? (SDL_Rect){winWidth / 2 - 200, winHeight / 8 + 250, 340, 140}
                : (SDL_Rect){winWidth / 2 - 200, winHeight / 8 + 400, 340, 140};
        SDL_RenderFillRect(renderer, &highlightRect);
        // 繪製選項
        SDL_Rect option1Rect = {winWidth / 2 - 200, winHeight / 8 + 300, 300,
                                100};
        SDL_Rect option2Rect = {winWidth / 2 - 200, winHeight / 8 + 400, 300,
                                100};
        SDL_RenderCopy(renderer, op1Texture, NULL, &option1Rect);
        SDL_RenderCopy(renderer, op2Texture, NULL, &option2Rect);
        SDL_RenderPresent(renderer);
        // 處理事件
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                exit(1);
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_UP:
                    case SDLK_w:
                        selected = 0;  // 選擇第一個選項
                        break;
                    case SDLK_DOWN:
                    case SDLK_s:
                        selected = 1;  // 選擇第二個選項
                        break;
                    case SDLK_RETURN:  // 按 Enter 確認選擇
                        quit = 1;
                        break;
                }
            }
        }
    }
    // 釋放資源
    SDL_DestroyTexture(bgTexture);
    SDL_DestroyTexture(op1Texture);
    SDL_DestroyTexture(op2Texture);
    TTF_CloseFont(titleFont);
    TTF_CloseFont(font);
    return selected;
}
void setrender(SDL_Renderer* renderer, int size) {
    switch (size) {
        case 41:
            SDL_RenderSetLogicalSize(renderer, 780, 780);
            break;
        case 33:
            SDL_RenderSetLogicalSize(renderer, 790, 790);
            break;
        case 21:
            SDL_RenderSetLogicalSize(renderer, 800, 800);
            break;
    }
}
int showmessage(const char* s, SDL_Window* window) {
    const SDL_MessageBoxButtonData buttons[] = {
        {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, u8"是"},
        {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, u8"否"}};
    // 定義訊息框的資料
    const SDL_MessageBoxData messageBoxData = {
        SDL_MESSAGEBOX_INFORMATION,  // 訊息框類型
        window,                      // 父窗口，設為 NULL
        u8"提示",                    // 訊息框標題
        s,                           // 訊息內容
        SDL_arraysize(buttons),      // 按鈕數量
        buttons,                     // 按鈕陣列
        NULL                         // 顏色方案
    };
    // 儲存玩家的選擇
    int buttonId;
    if (SDL_ShowMessageBox(&messageBoxData, &buttonId) < 0)
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error",
                                 "Failed to display the message box.", NULL);
    return buttonId;
}
