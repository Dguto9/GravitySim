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
    return (rand() % (rmax-rmin)) + rmin;
}

static const int width = 800;
static const int height = 600;

int palette[9] = {20, 20, 30, 255, 255, 255, 255, 100, 100};

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
    bool leaf;
    float mX;
    float mY;
    float mass;
} qtree_t;

int pCount = 10000;
particle_t* particles;
float dt = 0.00005;
float G = 10;
float damping = 1;
float theta = 0.6;
int drawOn = 1;

void SetPalette(int* palette, int index, SDL_Renderer* renderer);
void drawTree(SDL_Renderer* renderer, qtree_t* tree);
void drawTreeNoRecurse(SDL_Renderer* renderer, qtree_t* tree);
void drawTreeCM(SDL_Renderer* renderer, qtree_t* tree);
int boundsCheck(particle_t* particle, SDL_Rect* rect);
int lerp(int start, int end, float a);
void genTree(qtree_t* tree, particle_t** buffer, int count);
void updateParticle(particle_t* particle, qtree_t* tree, float theta, SDL_Renderer* renderer, bool dispTree);

int main(int argc, char **argv) {
    //srand(time(NULL));
    // Initialize SDL
    CHECK_ERROR(SDL_Init(SDL_INIT_VIDEO) != 0, SDL_GetError());
    SDL_Window *window = SDL_CreateWindow("Gravity", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_OPENGL);
    CHECK_ERROR(window == NULL, SDL_GetError());
    // Create a renderer (accelerated and in sync with the display refresh rate)
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);    
    CHECK_ERROR(renderer == NULL, SDL_GetError());
    SetPalette(palette, 0, renderer);
    particles = malloc(pCount*sizeof(particle_t));
    particle_t** buffer = malloc(pCount*sizeof(particle_t*));
    for (int i = 0; i < pCount; i++){
        int r = randInt(20, 200);
        float angle = ((double)rand() / RAND_MAX) * 2 * M_PI;
        particles[i].pX = (r * cos(angle)) + 400;
        particles[i].pY = (r * sin(angle)) + 300;
        particles[i].vX = (particles[i].pY - 300)*10000/r;//randInt(0, 100) - 50;
        particles[i].vY = -(particles[i].pX - 400)*10000/r;//randInt(0, 100) - 50;
        particles[i].mass = randInt(10000, 10000000);
        buffer[i] = &particles[i];
    }
    particles[0].pX = 400;
    particles[0].pY = 300;
    particles[0].vX = 0;
    particles[0].vY = 0;
    particles[0].mass = 2000000000;
    /*
    buffer[0] = &particles[0];
    particles[1].pX = 600;
    particles[1].pY = 300;
    particles[1].vX = 0;
    particles[1].vY = 2;
    particles[1].mass = 1000000;
    buffer[1] = &particles[1];
    particles[2].pX = 400;
    particles[2].pY = 300;
    particles[2].vX = 0;
    particles[2].vY = -100;
    particles[2].mass = 1000000;
    buffer[2] = &particles[2];
    */
    bool running = true;
    int viewInd = 0;
    int steps = 0;
    SDL_Event event;
    while(running) {
        while(SDL_PollEvent(&event)) {
            if(event.type == SDL_QUIT) {
                 running = false;
            }  
            else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                case SDLK_LEFT:
                    viewInd -= 1;
                    break;
                case SDLK_RIGHT:
                    viewInd += 1;
                    break;
                default:
                    break;
                }
            }
        }
        qtree_t tree;
        tree.bounds.x = 0;
        tree.bounds.y = 0;
        tree.bounds.w = 800;
        tree.bounds.h = 600;
        tree.sub = 0;
        tree.leaf = 0;
        tree.mass = 0;
        tree.mX = 0;
        tree.mY = 0;
        genTree(&tree, buffer, pCount);
        if (steps % drawOn == 0) {
            SetPalette(palette, 0, renderer);
            SDL_RenderClear(renderer);
        }
        for (int i = 0; i < pCount; i++){
            SetPalette(palette, 2, renderer);
            if (i == viewInd && steps % drawOn == 0) {
                updateParticle(particles + i, &tree, theta, renderer, 1);
            }
            else {
                updateParticle(particles + i, &tree, theta, renderer, 0);
            }
            particles[i].pX += particles[i].vX * dt;
            particles[i].pY += particles[i].vY * dt;
            if (steps % drawOn == 0) {
                SDL_SetRenderDrawColor(renderer, 255, lerp(100, 255, (float)(abs(particles[i].vX) + abs(particles[i].vY)) / 50000.0f), lerp(100, 255, (float)(abs(particles[i].vX) + abs(particles[i].vY)) / 50000.0f), 255);
                SDL_RenderDrawPoint(renderer, particles[i].pX, particles[i].pY);
            }
        }
        //SetPalette(palette, 2, renderer);
        //drawTree(renderer, &tree);
        SDL_RenderPresent(renderer);  
        steps++;
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
    if (tree->sub){
        SDL_RenderDrawRect(renderer, &(tree->bounds));
        //SDL_RenderDrawPoint(renderer, tree->mX, tree->mY);
        for (int i = 0; i < 4; i++){
            drawTree(renderer, (tree->branch)[i]);
        }
    }
    else if (tree->leaf) {
        SDL_RenderDrawRect(renderer, &(tree->bounds));
    }
}

void drawTreeNoRecurse(SDL_Renderer* renderer, qtree_t* tree) {
    SDL_RenderDrawRect(renderer, &(tree->bounds));
}

void drawTreeCM(SDL_Renderer* renderer, qtree_t* tree) {
    SDL_RenderDrawPoint(renderer, tree->mX, tree->mY);
}

void genTree(qtree_t* tree, particle_t** buffer, int count){
    particle_t** In = malloc(count*sizeof(particle_t*));
    int pNum = 0;
    for (int i = 0; i < count; i++){
        if (boundsCheck(buffer[i], &(tree->bounds))){
            tree->mass += buffer[i]->mass;
            tree->mX += (buffer[i]->pX * buffer[i]->mass);
            tree->mY += (buffer[i]->pY * buffer[i]->mass);
            In[pNum] = buffer[i];
            pNum++;
        }
    }
    tree->mX /= (tree->mass);
    tree->mY /= (tree->mass);
    if (pNum > 1){
        
        for (int i = 0; i < 4; i++) {
            (tree->branch)[i] = malloc(sizeof(qtree_t));
            (tree->branch)[i]->sub = 0;
            (tree->branch)[i]->leaf = 0;
            (tree->branch)[i]->mass = 0;
            (tree->branch)[i]->mX = 0;
            (tree->branch)[i]->mY = 0;
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
    else if (pNum == 1){
        tree->leaf = 1;
        tree->particle = In[0];
    }
}

void updateParticle(particle_t* particle, qtree_t* tree, float theta, SDL_Renderer* renderer, bool dispTree) {
    float distApprox = abs(tree->mX - particle->pX) + abs(tree->mY - particle->pY);
    if (!distApprox) {
        return;
    }
    if (((tree->bounds.w / distApprox) > theta) && tree->sub) {
        for (int i = 0; i < 4; i++) {
            updateParticle(particle, (tree->branch)[i], theta, renderer, dispTree);
        }
    }
    else {
        if (dispTree) {
            drawTreeNoRecurse(renderer, tree);
            //drawTreeCM(renderer, tree);
        }
        float distSquared = pow(tree->mX - particle->pX, 2) + pow(tree->mY - particle->pY, 2);
        float normalize = 1.0f/sqrt(distSquared);
        particle->vX += ((tree->mX - particle->pX) * normalize) * ((G * tree->mass) / (distSquared + damping)) * dt;
        particle->vY += ((tree->mY - particle->pY) * normalize) * ((G * tree->mass) / (distSquared + damping)) * dt;
    }
}

int boundsCheck(particle_t* particle, SDL_Rect* rect){
    return ((particle->pX >= rect->x) && (particle->pX < (rect->x + rect->w)) && (particle->pY >= rect->y) && (particle->pY < (rect->y + rect->h)));
}

int lerp(int start, int end, float a) {
    return start + ((float)(end - start) * (a > 1 ? 1 : a < 0 ? 0 : a));
}

