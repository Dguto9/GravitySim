#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

#include <SDL2/SDL.h>
// Normally SDL2 will redefine the main entry point of the program for Windows applications
// this doesn't seem to play nice with TCC, so we just undefine the redefinition
#ifdef __TINYC__
    #undef main
#endif

// Utility macros
#define CHECK_ERROR(test, message) \
    do { \
        if((test)) { \
            fprintf(stderr, "%s\n", (message)); \
            exit(1); \
        } \
    } while(0)

int randInt(int rmin, int rmax) {
    return rand() % rmax + rmin;
}

static const int width = 800;
static const int height = 600;

int palette[9] = {20, 20, 30, 255, 0, 0, 0, 255, 0};

typedef struct particle_t {
    float pX;
    float pY;
    float vX;
    float vY;
    float mass;
} particle_t;

typedef struct qtree_t {
    particle_t* particle;
    struct qtree_t* branch[4];
    SDL_Rect bounds;
    bool sub;
} qtree_t;

int pCount = 1000;
particle_t* particles;
float dt = 0.05;

void SetPalette(int* palette, int index, SDL_Renderer* renderer);
void drawTree(SDL_Renderer* renderer, qtree_t* tree);
int boundsCheck(particle_t* particle, SDL_Rect* rect);
void genTree(qtree_t* tree, particle_t** buffer, int count);

int main(int argc, char **argv) {
    //srand(time(NULL));
    // Initialize SDL
    CHECK_ERROR(SDL_Init(SDL_INIT_VIDEO) != 0, SDL_GetError());
    SDL_Window *window = SDL_CreateWindow("Hello, SDL2", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_OPENGL);
    CHECK_ERROR(window == NULL, SDL_GetError());
    // Create a renderer (accelerated and in sync with the display refresh rate)
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);    
    CHECK_ERROR(renderer == NULL, SDL_GetError());
    SetPalette(palette, 0, renderer);
    particles = malloc(pCount*sizeof(particle_t));
    particle_t** buffer = malloc(pCount*sizeof(particle_t*));
    for (int i = 0; i < pCount; i++){
        particles[i].pX = randInt(0, 800);
        particles[i].pY = randInt(0, 600);
        particles[i].vX = randInt(0, 100) - 50;
        particles[i].vY = randInt(0, 100) - 50;
        buffer[i] = &particles[i];
    }
    bool running = true;
    SDL_Event event;
    while(running) {
        while(SDL_PollEvent(&event)) {
            if(event.type == SDL_QUIT) {
                 running = false;
            }                    
        }
        qtree_t tree;
        tree.bounds.x = 0;
        tree.bounds.y = 0;
        tree.bounds.w = 800;
        tree.bounds.h = 600;
        tree.sub = 0;
        genTree(&tree, buffer, pCount);
        SetPalette(palette, 0, renderer);
        SDL_RenderClear(renderer);
        SetPalette(palette, 1, renderer);
        for (int i = 0; i < pCount; i++){
            SDL_RenderDrawPoint(renderer, particles[i].pX, particles[i].pY);
            particles[i].pX += particles[i].vX * dt;
            particles[i].pY += particles[i].vY * dt;
        }
        SetPalette(palette, 2, renderer);
        drawTree(renderer, &tree);
        SDL_RenderPresent(renderer);  
    }

    // Release resources
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

void SetPalette(int* palette, int index, SDL_Renderer* renderer){
    SDL_SetRenderDrawColor(renderer, palette[3*index], palette[(3*index)+1], palette[(3*index)+2], 255);
}

void drawTree(SDL_Renderer* renderer, qtree_t* tree){
    SDL_RenderDrawRect(renderer, &(tree->bounds));
    if (tree->sub){
        for (int i = 0; i < 4; i++){
            drawTree(renderer, (tree->branch)[i]);
        }
    }
}

void genTree(qtree_t* tree, particle_t** buffer, int count){
    particle_t** In = malloc(count*sizeof(particle_t*));
    int pNum = 0;
    for (int i = 0; i < count; i++){
        if (boundsCheck(buffer[i], &(tree->bounds))){
            In[pNum] = buffer[i];
            pNum++;
        }
    }
    if (pNum > 1){
        
        for (int i = 0; i < 4; i++) {
            (tree->branch)[i] = malloc(sizeof(qtree_t));
            (tree->branch)[i]->sub = 0;
            (tree->branch)[i]->bounds.w = tree->bounds.w/2;
            (tree->branch)[i]->bounds.h = tree->bounds.h/2;
        }
        tree->sub = 1;
        (tree->branch)[0]->bounds.x = tree->bounds.x;
        (tree->branch)[0]->bounds.y = tree->bounds.y;
        (tree->branch)[1]->bounds.x = tree->bounds.x + (tree->bounds.w/2);
        (tree->branch)[1]->bounds.y = tree->bounds.y; 
        (tree->branch)[2]->bounds.x = tree->bounds.x;
        (tree->branch)[2]->bounds.y = tree->bounds.y + (tree->bounds.h/2);
        (tree->branch)[3]->bounds.x = tree->bounds.x + (tree->bounds.w/2);
        (tree->branch)[3]->bounds.y = tree->bounds.y + (tree->bounds.h/2);
        for (int i = 0; i < 4; i++) {
            genTree((tree->branch)[i], In, pNum);
        }
    }
    else{
        tree->particle = In[0];
    }
}

int boundsCheck(particle_t* particle, SDL_Rect* rect){
    return ((particle->pX >= rect->x) && (particle->pX < (rect->x + rect->w)) && (particle->pY >= rect->y) && (particle->pY < (rect->y + rect->h)));
}

