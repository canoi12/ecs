#define ECS_IMPLEMENTATION
#include "ecs.h"

#include <SDL2/SDL.h>

SDL_Window* window;
SDL_Renderer* render;
SDL_Event ev;
Uint8* keys;
float delta;

enum {
    TRANSFORM_COMPONENT = 0,
    KINEMATIC_COMPONENT,
    RENDER_COMPONENT,
    INPUT_COMPONENT,
    TAG_COMPONENT,
    SPRITE_COMPONENT,
    ANIMATED_SPRITE_COMPONENT,
    CANVAS_COMPONENT,

    COMPONENTS_COUNT
};

enum {
    TAG_NONE = 0,
    TAG_PLAYER,
    TAG_ENEMY
};

struct Transform {
    struct { float x, y; } position;
    struct { float x, y; } scale;
    float angle;
};

struct Kinematic {
    float speed;
    struct {
        float x, y;
    } velocity;
};

struct Render {
    SDL_Color color;
    float width, height;
};

struct Sprite {
    SDL_Texture* texture;
    SDL_Rect source;
    int width, height;
    int tilew, tileh;
};

struct AnimatedSprite {
    SDL_Texture* texture;
    SDL_Rect* frames;
    int width, height;
    int tilew, tileh;
};

struct Canvas {
    SDL_Texture* target;
};

void set_canvas_system(ecs_filter_t* filter) {
    ecs_world_t* w = filter->world;
    for (int i = 0; i < filter->entities_count; i++) {
        struct Canvas* canvas = ecs_entity_get_component(w, filter->entities[i], CANVAS_COMPONENT);
        SDL_SetRenderTarget(render, canvas->target);
        SDL_SetRenderDrawColor(render, 75, 125, 125, 255);
        SDL_RenderClear(render);
    }
}

void unset_canvas_system(ecs_filter_t* filter) {
    ecs_world_t* w = filter->world;
    for (int i = 0; i < filter->entities_count; i++) {
        struct Canvas* canvas = ecs_entity_get_component(w, filter->entities[i], CANVAS_COMPONENT);
        SDL_SetRenderTarget(render, NULL);
    }
}

void draw_canvas_system(ecs_filter_t* filter) {
    ecs_world_t* w = filter->world;
    for (int i = 0; i < filter->entities_count; i++) {
        struct Canvas* canvas = ecs_entity_get_component(w, filter->entities[i], CANVAS_COMPONENT);
        SDL_RenderCopy(render, canvas->target, NULL, NULL);
    }
}

void input_system(ecs_filter_t* filter) {
    ecs_world_t* w = filter->world;
    for (int i = 0; i < filter->entities_count; i++) {
        struct Kinematic* k = ecs_entity_get_component(w, filter->entities[i], KINEMATIC_COMPONENT);
        char* input = ecs_entity_get_component(w, filter->entities[i], INPUT_COMPONENT);
        char* tag = ecs_entity_get_component(w, filter->entities[i], TAG_COMPONENT);

        if (*tag == TAG_PLAYER) {
            if (keys[SDL_SCANCODE_LEFT]) k->velocity.x = -1;
            else if (keys[SDL_SCANCODE_RIGHT]) k->velocity.x = 1;
            else k->velocity.x = 0;
        }
    }
}

void move_system(ecs_filter_t* filter) {
    ecs_world_t* w = filter->world;
    for (int i = 0; i < filter->entities_count; i++) {
        struct Transform* t = ecs_entity_get_component(w, filter->entities[i], TRANSFORM_COMPONENT);
        struct Kinematic* k = ecs_entity_get_component(w, filter->entities[i], KINEMATIC_COMPONENT);

        t->position.x += k->velocity.x * (delta * k->speed);
        t->position.y += k->velocity.y * (delta * k->speed);
    }
}

void render_system(ecs_filter_t* filter) {
    ecs_world_t* w = filter->world;
    for (int i = 0; i < filter->entities_count; i++) {
        ecs_entity_t e = filter->entities[i];
        struct Transform* t = ecs_entity_get_component(w, e, TRANSFORM_COMPONENT);
        struct Render* r = ecs_entity_get_component(w, e, RENDER_COMPONENT);
        SDL_SetRenderDrawColor(render, 255, 255, 255, 255);
        SDL_Rect rect;
        rect.x = t->position.x;
        rect.y = t->position.y;
        rect.w = r->width;
        rect.h = r->height;
        SDL_RenderDrawRect(render, &rect);
    }
}

#define INPUT_SYSTEM_MASK \
ECS_MASK(3, INPUT_COMPONENT, KINEMATIC_COMPONENT, TAG_COMPONENT)

#define MOVE_SYSTEM_MASK \
ECS_MASK(2, TRANSFORM_COMPONENT, KINEMATIC_COMPONENT)

#define RENDER_SYSTEM_MASK \
ECS_MASK(2, TRANSFORM_COMPONENT, RENDER_COMPONENT)

#define CANVAS_SYSTEM_MASK \
ECS_MASK(1, CANVAS_COMPONENT)

int main(int argc, char** argv) {

    if (SDL_Init(SDL_INIT_VIDEO)) return 0;

    window = SDL_CreateWindow("ECS", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 380, SDL_WINDOW_SHOWN);
    render = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    keys = (Uint8*)SDL_GetKeyboardState(NULL);

    ecs_world_t* w = ecs_create(1000, COMPONENTS_COUNT, 32);
    ecs_register_component(w, TRANSFORM_COMPONENT, sizeof(struct Transform), 128);
    ecs_register_component(w, KINEMATIC_COMPONENT, sizeof(struct Kinematic), 128);
    ecs_register_component(w, RENDER_COMPONENT, sizeof(struct Render), 256);
    ecs_register_component(w, INPUT_COMPONENT, sizeof(char), 24);
    ecs_register_component(w, TAG_COMPONENT, sizeof(char), 128);
    ecs_register_component(w, SPRITE_COMPONENT, sizeof(struct Sprite), 256);
    ecs_register_component(w, CANVAS_COMPONENT, sizeof(struct Canvas), 2);

    ecs_register_system(w, input_system, INPUT_SYSTEM_MASK);
    ecs_register_system(w, move_system, MOVE_SYSTEM_MASK);
    ecs_register_system(w, set_canvas_system, CANVAS_SYSTEM_MASK);
    ecs_register_system(w, render_system, RENDER_SYSTEM_MASK);
    ecs_register_system(w, unset_canvas_system, CANVAS_SYSTEM_MASK);
    ecs_register_system(w, draw_canvas_system, CANVAS_SYSTEM_MASK);

    ecs_entity_t e = ecs_create_entity(w);
    struct Transform t;
    t.position.x = 256;
    t.position.y = 32;
    struct Render r;
    r.width = 32;
    r.height = 64;
    struct Kinematic k = {
        .speed = 120,
        .velocity = { 0, 0 }
    };

    ecs_entity_set_component(w, e, TRANSFORM_COMPONENT, &t);
    ecs_entity_set_component(w, e, RENDER_COMPONENT, &r);
    ecs_entity_set_component(w, e, KINEMATIC_COMPONENT, &k);
    ecs_entity_set_component(w, e, INPUT_COMPONENT, NULL);
    char tag = TAG_PLAYER;
    ecs_entity_set_component(w, e, TAG_COMPONENT, &tag);

    e = ecs_create_entity(w);
    t.position.x = 64;
    t.position.y = 64;
    ecs_entity_set_component(w, e, TRANSFORM_COMPONENT, &t);
    ecs_entity_set_component(w, e, RENDER_COMPONENT, &r);
    ecs_entity_set_component(w, e, KINEMATIC_COMPONENT, &k);
    tag = TAG_ENEMY;
    ecs_entity_set_component(w, e, TAG_COMPONENT, &tag);

    e = ecs_create_entity(w);
    struct Canvas canvas;
    canvas.target = SDL_CreateTexture(render, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, 320, 190);
    ecs_entity_set_component(w, e, CANVAS_COMPONENT, &canvas);

    double last = SDL_GetTicks();

    while (ev.type != SDL_QUIT) {
        while (SDL_PollEvent(&ev));
        double current = SDL_GetTicks();
        delta = (current - last) / 1000.f;
        last = current;
        SDL_SetRenderDrawColor(render, 0, 0, 0, 0);
        SDL_RenderClear(render);

        ecs_update(w);

        SDL_RenderPresent(render);
    }

    ecs_destroy(w);
    SDL_DestroyTexture(canvas.target);

    SDL_DestroyRenderer(render);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}