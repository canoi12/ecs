# ecs
Single header ECS lib

```c

#define ECS_IMPLEMENTATION
#include "ecs.h"

enum {
    TRANSFORM_COMPONENT = 0,
    KINEMATIC_COMPONENT,

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

void move_system(ecs_filter_t* filter) {
    ecs_world_t* w = filter->world;
    for (int i = 0; i < filter->entities_count; i++) {
        ecs_entity_t e = filter->entities[i];
        struct Transform* t = ecs_entity_get_component(w, e, TRANSFORM_COMPONENT);
        struct Kinematic* k = ecs_entity_get_component(w, e, KINEMATIC_COMPONENT);

        t->position.x += k->velocity.x * (0.001 * k->speed);
        t->position.y += k->velocity.y * (0.001 * k->speed);
    }
}

#define MOVE_SYSTEM_MASK \
ECS_MASK(2, TRANSFORM_COMPONENT, KINEMATIC_COMPONENT)

int main(int argc, char** argv) {
    ecs_world_t* w = ecs_create(256, COMPONENTS_COUNT, 16);
    ecs_register_component(w, TRANSFORM_COMPONENT, sizeof(struct Transform), 20);
    ecs_register_component(w, KINEMATIC_COMPONENT, sizeof(struct Kinematic), 20);

    ecs_register_system(w, move_system, MOVE_SYSTEM_MASK);

    ecs_entity_t e = ecs_create_entity(w);
    struct Transform t = {
        {64, 32},
        {1, 1},
        0
    };
    struct Kinematic k = {
        120,
        {0, 0}
    };
    ecs_entity_set_component(w, e, TRANSFORM_COMPONENT, &t);
    ecs_entity_set_component(w, e, KINEMATIC_COMPONENT, &k);


    while (1) ecs_update(w);

    ecs_destroy(w);
    return 0;
}

```


I'm using other libs as reference, so it's valid to check out if you want a more stable code in your project:

- [ecs](https://github.com/soulfoam/ecs)
- [flecs](https://github.com/SanderMertens/flecs)
- [entt](https://github.com/skypjack/entt)
