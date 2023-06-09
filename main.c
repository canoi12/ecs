#include <stdio.h>

#define ECS_IMPLEMENTATION
#include "ecs.h"

enum {
    TRANSFORM_COMPONENT,
    KINEMATIC_COMPONENT,
    SPRITE_COMPONENT,

    COMPONENTS_COUNT
};

struct Transform {
    struct { float x, y; } position;
    struct { float x, y; } scale;
    float angle;
};

struct Kinematic {
    float speed;
    struct { float x, y; } velocity;
};

void print_system(ecs_world_t* w) {
    for (int i = 0; i < w->max_entities; i++) {
        struct Transform* t = ecs_get_component(w, i+1, TRANSFORM_COMPONENT);
        if (t) {
            printf("(%i) Transform(%p): %f %f\n", i+1, t, t->position.x, t->position.y);
            // printf("Testandow\n");
        }
    }
}

int main(int argc, char** argv) {
    ecs_world_t* w = ecs_create(1000, COMPONENTS_COUNT, 4);
    ecs_register_component(w, TRANSFORM_COMPONENT, sizeof(struct Transform), 1000);
    ecs_register_component(w, KINEMATIC_COMPONENT, sizeof(struct Kinematic), 1000);
    ecs_entity_t e = ecs_create_entity(w);
    struct Transform t = {0};
    t.position.x = 120;
    t.position.y = 130;
    ecs_set_component(w, e, TRANSFORM_COMPONENT, &t);
    e = ecs_create_entity(w);
    t.position.x = 256;
    ecs_set_component(w, e, TRANSFORM_COMPONENT, &t);
    e = ecs_create_entity(w);
    t.position.y = 135;
    ecs_set_component(w, e, TRANSFORM_COMPONENT, &t);
    ecs_set_component(w, e, KINEMATIC_COMPONENT, NULL);

    // ecs_update(w);

    ecs_destroy_entity(w, e);
    ecs_destroy(w);
    return 0;
}