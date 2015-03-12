// Hex math taken from the amazing Amit Patel @ http://www.redblobgames.com/grids/hexagons/

#include <SDL.h>
#include <SDL_ttf.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

#define PI 3.1415927f

static bool running;

int gridOriginX = 30;
int gridOriginY = 30;
int gridWidth = 14;
int gridHeight = 14;
int hexSize = 30;
float hexWidth = (3.0f/2.0f) * hexSize;
float hexHeight = sqrt(3.0f) * hexSize;

int fontSize = 22;
TTF_Font* font;

struct Map
{
    int width;
    int height;
    void* data;
};

struct GameState
{
    Map map;

    int currentPlayer;
    int playerVelocity[2];
    SDL_Point playerLoc[2];
};


bool isDigit(int c)
{
    return c >= '0' && c <= '9';
}

int getUIntFromByteStream(FILE* inFile)
{
    int result = 0;
    int nextByte;
    while(isDigit(nextByte = fgetc(inFile)))
    {
        if(nextByte == EOF)
        {
            printf("Error reading map file, unexpected EOF\n");
            return -1;
        }
        result = (10*result+(nextByte-'0'));
    }
    return result;
}

void loadMap(char* filename, GameState* gameState)
{
    FILE* inFile;
    fopen_s(&inFile, filename, "r");
    
    // Load Map Data
    gameState->map.width = getUIntFromByteStream(inFile);
    gameState->map.height = getUIntFromByteStream(inFile);
    gameState->map.data = malloc(gameState->map.width * gameState->map.height);
    for(int y=0; y<gameState->map.height; ++y)
    {
        fread((void*)((char*)gameState->map.data + gameState->map.width*y), 1, gameState->map.width, inFile);
        fgetc(inFile); // Skip the newline byte
    }
	fclose(inFile);

    // Assign player start locations
    for(int y=0; y<gameState->map.height; ++y)
    {
        for(int x=0; x<gameState->map.width; ++x)
        {
            char c = *((char*)gameState->map.data + gameState->map.width*y + x);
            if(c == 'A')
            {
                gameState->playerLoc[0].x = x;
                gameState->playerLoc[0].y = y;
            }
            else if(c == 'B')
            {
                gameState->playerLoc[1].x = x;
                gameState->playerLoc[1].y = y;
            }
        }
    }
}

// TODO: Implement this properly (at the moment it's SUPER inefficient)
void text(SDL_Renderer* renderer, char* text, int x, int y)
{
    int textLength = 0;
    while(text[textLength] != 0)
    {
        ++textLength;
    }
    
    SDL_Color White = {255, 255, 255};
    SDL_Surface* surfaceMessage = TTF_RenderText_Solid(font, text, White);
    SDL_Texture* TextMessage = SDL_CreateTextureFromSurface(renderer, surfaceMessage);

    SDL_Rect Message_rect;
    Message_rect.x = x;
    Message_rect.y = y;
    Message_rect.w = (int)(fontSize*(3.0f/4.0f))*textLength;
    Message_rect.h = fontSize+5; // controls the height of the rect
    printf("%s\n", SDL_GetError());
    //Now since it's a texture, you have to put RenderCopy in your game loop area, the area where the whole code executes
    SDL_RenderCopy(renderer, TextMessage, NULL, &Message_rect); //you put the renderer's name first, the Message, the crop size(you can ignore this if you don't want to dabble with cropping), and the rect which is the size and coordinate of your texture
    SDL_FreeSurface(surfaceMessage);
    SDL_DestroyTexture(TextMessage);
}

int round(float x)
{
    return (int)(x+0.5f);
}

int screenToHexX(int screenX)
{
    return (int)round((screenX - gridOriginX)/hexWidth);
}

int screenToHexY(int screenX, int screenY)
{
    int hexX = screenToHexX(screenX);
    if(hexX%2 == 0)
    {
        screenY -= (int)(hexHeight/2.0f);
    }
    return (int)round((screenY - gridOriginY)/hexHeight);
}

int hexToScreenX(int hexX)
{
    return gridOriginX + (int)(hexWidth*hexX);
}

int hexToScreenY(int hexX, int hexY)
{
    int result = gridOriginY + (int)(hexHeight*hexY);
    if(hexX%2 == 0)
    {
        result += (int)(hexHeight/2.0f);
    }
    return result;
}

int drawRegularHex(SDL_Renderer* renderer, int centreX, int centreY, int radius)
{
    SDL_Point hexPoints[7];
    for(int i=0; i<7; ++i)
    {
        float angle = (60.0f*i);
        float angle_rad = (PI/180) * angle;

        hexPoints[i].x = (int)(centreX + radius*cosf(angle_rad));
        hexPoints[i].y = (int)(centreY + radius*sinf(angle_rad));
    }
    return SDL_RenderDrawLines(renderer, hexPoints, 7);
}

void fillRect(SDL_Renderer* renderer, int centreX, int centreY, int width, int height)
{
    SDL_Rect rect;
    rect.x = centreX-width/2;
    rect.y = centreY-height/2;
    rect.w = width;
    rect.h = height;
    SDL_RenderFillRect(renderer, &rect);
}

void render(SDL_Renderer* renderer, GameState* gameState)
{
    // Clear to black
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    // Draw Hex Grid
    for(int y=0; y<gridHeight; ++y)
    {
        for(int x=0; x<gridWidth; ++x)
        {
            int centreX = (int)(gridOriginX + x*hexWidth);
            int centreY = (int)(gridOriginY + y*hexHeight);
            if(x%2 == 0)
            {
                centreY += (int)(hexHeight/2.0f);
            }
            drawRegularHex(renderer, centreX, centreY, hexSize);

            char hexType = *((char*)gameState->map.data + (y*gameState->map.width + x));
            if(hexType != 'X')
            {
                int screenX = hexToScreenX(x);
                int screenY = hexToScreenY(x, y);
                fillRect(renderer, screenX, screenY, (int)(hexWidth/2), (int)(hexHeight/2));
            }
        }
    }

    // Draw player hex's
    int playerScreenX;
    int playerScreenY;
    playerScreenX = hexToScreenX(gameState->playerLoc[0].x);
    playerScreenY = hexToScreenY(gameState->playerLoc[0].x, gameState->playerLoc[0].y);
    SDL_SetRenderDrawColor(renderer, 255, 105, 97, 255);
    fillRect(renderer, playerScreenX, playerScreenY, (int)(hexWidth/2), (int)(hexHeight/2));
    playerScreenX = hexToScreenX(gameState->playerLoc[1].x);
    playerScreenY = hexToScreenY(gameState->playerLoc[1].x, gameState->playerLoc[1].y);
    SDL_SetRenderDrawColor(renderer, 97, 168, 255, 255);
    fillRect(renderer, playerScreenX, playerScreenY, (int)(hexWidth/2), (int)(hexHeight/2));

    // Draw Mouseover'd Hex
    int mouseX;
    int mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);
    int mouseHexX = screenToHexX(mouseX);
    int mouseHexY = screenToHexY(mouseX, mouseY);
    if(mouseHexX < 0)
    {
        mouseHexX = 0;
    }
    else if(mouseHexX >= gridWidth)
    {
        mouseHexX = gridWidth-1;
    }
    if(mouseHexY < 0)
    {
        mouseHexY = 0;
    }
    else if(mouseHexY >= gridHeight)
    {
        mouseHexY = gridHeight-1;
    }
    int roundedMouseX = hexToScreenX(mouseHexX);
    int roundedMouseY = hexToScreenY(mouseHexX, mouseHexY);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    fillRect(renderer, roundedMouseX, roundedMouseY, (int)(hexWidth/2), (int)(hexHeight/2));

    // Draw Text
    text(renderer, "HexRacer!", 650, 10);
    char mouseXStr[4];
    char mouseYStr[4];
    _itoa_s(mouseX, mouseXStr, 10);
    _itoa_s(mouseY, mouseYStr, 10);
    text(renderer, mouseXStr, 650, 50);
    text(renderer, mouseYStr, 750, 50);

    SDL_RenderPresent(renderer);
}

void SDLHandleEvent(SDL_Event* event)
{
    switch(event->type)
    {
        case SDL_QUIT:
        {
            running = false;
        } break;
        case SDL_KEYDOWN:
        case SDL_KEYUP:
        {
            SDL_Keycode keyCode = event->key.keysym.sym;
            if(keyCode == SDLK_w && !event->key.repeat && event->key.state == SDL_PRESSED)
            {
                printf("W\n");
            }
            if(keyCode == SDLK_ESCAPE)
            {
                running = false;
            }
        } break;
        case SDL_WINDOWEVENT:
        {
            switch(event->window.event)
            {
                case SDL_WINDOWEVENT_RESIZED:
                {
                } break;
            }
        } break;
        default:
        {
        } break;
    }
}

int main(int argc, char** argv)
{
    if(SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        // Couldn't load SDL
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Error", "Unable to load SDL!", 0);    
        return 1;
    }

    if(TTF_Init()==-1)
    {
        printf("TTF_Init: %s\n", TTF_GetError());
        return 1;
    }

    SDL_Window* window  = SDL_CreateWindow("Hex Racing", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                           800, 800, SDL_WINDOW_RESIZABLE);
    if(window)
    {
        SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
        if(renderer)
        {
            GameState gameState;
            loadMap("test.map", &gameState);
            
            gameState.currentPlayer = 0;
            gameState.playerVelocity[0] = 1;
            gameState.playerVelocity[1] = 1;

            font = TTF_OpenFont("ubuntumono.ttf", fontSize);

            running = true;
            while(running)
            {
                SDL_Event event;
                while(SDL_PollEvent(&event))
                {
                    SDLHandleEvent(&event);    
                }

                render(renderer, &gameState);
            }

            free(gameState.map.data);
        }
        else
        {
            // Couldn't create renderer
        }
    }
    else
    {
        // Couldn't create window
    }

    SDL_Quit();
    return 0;
}
