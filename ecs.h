#ifndef _ECS_H_
#define _ECS_H_

#define ECS_API
#define ECS_VERSION "0.1.0"

#ifndef ECS_MALLOC
    #define ECS_MALLOC malloc
#endif

#ifndef ECS_FREE
    #define ECS_FREE free
#endif

#ifndef ECS_REALLOC
    #define ECS_REALLOC realloc
#endif

#define ECS_MASK(count, ...) \
count, (int[]){__VA_ARGS__}

typedef unsigned int ecs_entity_t;

typedef struct ecs_world_t ecs_world_t;

typedef struct {
    unsigned int mask;
    ecs_world_t* world;
    int entities_count;
    ecs_entity_t* entities;
} ecs_filter_t;

typedef void(*ecs_system_func_t)(ecs_filter_t*);

#if defined(__cplusplus)
extern "C" {
#endif

ECS_API ecs_world_t* ecs_create(int entities, int components, int systems);
ECS_API void ecs_destroy(ecs_world_t* w);

ECS_API void ecs_clear(ecs_world_t* w);
ECS_API void ecs_clear_entities(ecs_world_t* w);
ECS_API void ecs_clear_components(ecs_world_t* w);
ECS_API void ecs_clear_systems(ecs_world_t* w);

ECS_API void ecs_update(ecs_world_t* w);
// ECS_API void ecs_run_systems(ecs_world_t* w, int type);

ECS_API void ecs_register_component(ecs_world_t* w, int index, unsigned int size, unsigned int count);
ECS_API void ecs_unregister_component(ecs_world_t* w, int index);

ECS_API void ecs_register_system(ecs_world_t* w, ecs_system_func_t fn, int filter_count, int filters[]);
ECS_API void ecs_unregister_system(ecs_world_t* w, ecs_system_func_t fn);

ECS_API ecs_entity_t ecs_create_entity(ecs_world_t* w);
ECS_API void ecs_destroy_entity(ecs_world_t* w, ecs_entity_t e);

ECS_API void ecs_entity_set_component(ecs_world_t* w, ecs_entity_t e, int comp, void* data);
ECS_API void* ecs_entity_get_component(ecs_world_t* w, ecs_entity_t e, int comp);
ECS_API void ecs_entity_remove_component(ecs_world_t* w, ecs_entity_t e, int comp);

#if defined(__cplusplus)
}
#endif

#endif /* _ECS_H_ */

#if defined(ECS_IMPLEMENTATION)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char enabled;
    unsigned int mask;
    int* components;
} ecs_entity_internal_t;

typedef struct {
    void* data;
    int count;
    int size;

    int top;
    char* enabled;
} ecs_component_pool_t;

typedef struct {
    char enabled;
    unsigned int mask;
    ecs_system_func_t func;
    ecs_filter_t filter;
} ecs_system_t;

struct ecs_world_t {
    int max_entities;
    int max_components;
    int max_systems;

    int entity_top;
    int system_top;

    ecs_entity_internal_t* entities;
    ecs_component_pool_t* components;
    ecs_system_t* systems;

    ecs_filter_t filter;
};

static void update_filters(ecs_world_t* w, int comp) {
    for (int i = 0; i < w->system_top; i++) {
        ecs_system_t* sys = &(w->systems[i]);
        ecs_filter_t* filter = &(sys->filter);
        filter->world = w;
        if (!(sys->mask & (1 << comp))) continue;
        sys->filter.entities_count = 0;
        for (int e = 0; e < w->entity_top; e++) {
            ecs_entity_internal_t* ent = &(w->entities[e]);
            if (ent->enabled && ((ent->mask & sys->mask) == sys->mask)) {
                filter->entities[filter->entities_count++] = (e+1);
            }
        }
    }
}

ecs_world_t* ecs_create(int entities, int components, int systems) {
    ecs_world_t* world = ECS_MALLOC(sizeof(*world));
    if (!world) return world;
    memset(world, 0, sizeof(*world));
    world->max_entities = entities;
    world->max_components = components;
    world->max_systems = systems;

    world->entity_top = 0;
    world->system_top = 0;

    // world->filter.entities_count = 0;
    // world->filter.entities = ECS_MALLOC(sizeof(ecs_entity_t) * entities);

    world->entities = ECS_MALLOC(sizeof(ecs_entity_internal_t) * entities);
    world->components = ECS_MALLOC(sizeof(ecs_component_pool_t) * components);
    world->systems = ECS_MALLOC(sizeof(ecs_system_t) * systems);

    for (int i = 0; i < entities; i++) {
        world->entities[i].enabled = 0;
        world->entities[i].mask = 0;
        world->entities[i].components = ECS_MALLOC(sizeof(int) * world->max_components);
        for (int c = 0; c < components; c++) {
            world->entities[i].components[c] = -1;
        }
    }
    memset(world->components, 0, sizeof(ecs_component_pool_t) * components);
    memset(world->systems, 0, sizeof(ecs_system_t) * systems);

    return world;
}

void ecs_destroy(ecs_world_t* w) {
    if (!w) return;
    ECS_FREE(w->filter.entities);
    for (int i = 0; i < w->max_entities; i++) {
        if (w->entities[i].enabled) ECS_FREE(w->entities[i].components);
    }
    ECS_FREE(w->entities);
    for (int i = 0; i < w->max_components; i++) {
        ecs_component_pool_t* pool = &(w->components[i]);
        if (pool->size > 0) {
            ECS_FREE(pool->data);
            ECS_FREE(pool->enabled);
        }
    }
    ECS_FREE(w->components);
    for (int i = 0; i < w->max_systems; i++) {
        ecs_system_t* sys = &(w->systems[i]);
        if (sys->enabled) ECS_FREE(sys->filter.entities);
    }
    ECS_FREE(w->systems);
    ECS_FREE(w);
}

void ecs_clear_entities(ecs_world_t* w) {
    if (!w) return;
    for (int i = 0; i < w->entity_top; i++) {
        ecs_entity_internal_t* ent = &(w->entities[i]);
        if (ent->enabled) {
            ent->enabled = 0;
        }
    }
    for (int i = 0; i < w->max_components; i++) {
        ecs_component_pool_t* pool = &(w->components[i]);
        if (pool->count > 0) {
            memset(pool->enabled, 0, sizeof(char) * (pool->count / 8));
        }
    }
    for (int i = 0; i < w->system_top; i++) {
        ecs_system_t* sys = &(w->systems[i]);
        if (sys->enabled) sys->filter.entities_count = 0;
    }
}

void ecs_update(ecs_world_t* w) {
    if (!w) return;
    ecs_filter_t* filter = &(w->filter);
    // for (int i = 0; i < SYSTEM_TYPES; i++) ecs_run_systems(w, i);
    for (int i = 0; i < w->system_top; i++) {
        ecs_system_t* sys = &(w->systems[i]);
        if (sys->enabled) sys->func(&(sys->filter));
    }
}

void ecs_run_systems(ecs_world_t* w, int type) {
    if (!w) return;
    // ecs_filter_t* filter = &(w->filter);
    // ecs_system_pool_t* pool = &(w->systems[type]);
    for (int i = 0; i < w->system_top; i++) {
        // filter->entities_count = 0;
        ecs_system_t* sys = &(w->systems[i]);
        // ecs_filter_t* filter = &(sys->filter);
        // for (int j = 0; j < w->entity_top; j++) {
        //     if ((w->entities[j].mask & sys->mask) == sys->mask) {
        //         filter->entities[filter->entities_count++] = j+1;
        //     }
        // }
        if (sys->enabled) sys->func(&(sys->filter));
    }
}

ecs_entity_t ecs_create_entity(ecs_world_t* w) {
    ecs_entity_t e = 0;
    if (!w) return e;
    int top = w->entity_top;
    int i;
    for (i = 0; i < top; i++) {
        ecs_entity_internal_t* ent = &(w->entities[i]);
        if (!ent->enabled) {
            ent->enabled = 1;
            ent->mask = 0;
            break;
        }
    }
    if (i == top) {
        ecs_entity_internal_t* ent = &(w->entities[i]);
        if (!ent->enabled) {
            ent->enabled = 1;
            ent->mask = 0;
        }
        w->entity_top++;
    }
    e = w->entity_top;
    return e;
}

void ecs_destroy_entity(ecs_world_t* w, ecs_entity_t e) {
    if (!w) return;
    int index = e - 1;
    ecs_entity_internal_t* ent = &(w->entities[index]);
    ent->enabled = 0;
    for (int i = 0; i < w->max_components; i++) {
        ent->components[i] = -1;
    }
    ent->mask = 0;
}

static void enable_component(ecs_world_t* w, int comp, int index, int enable) {
    ecs_component_pool_t* pool = &(w->components[comp]);
    int pos = pool->count / 8;
    int i = index % 8;
    pool->enabled[pos] &= ~((1 << i) * enable);
}

static int get_free_component(ecs_world_t* w, int comp) {
    ecs_component_pool_t* pool = &(w->components[comp]);
    int top = pool->top;
    int i;
    for (i = 0; i < pool->count; i++) {
        int index = pool->count / 8;
        int ii = i % 8;
        char enabled = pool->enabled[index];
        if (!(enabled & (1 << ii))) {
            pool->enabled[index] |= (1 << ii);
            break;
        }
    }
    if (i == top) pool->top++;

    return i;
}

void ecs_register_component(ecs_world_t* w, int index, unsigned int size, unsigned int count) {
    if (!w) return;
    ecs_component_pool_t* comp = &(w->components[index]);
    comp->count = count;
    comp->size = size;
    comp->data = ECS_MALLOC(size * count);
    comp->top = 0;
    int sz = count / 8;
    comp->enabled = ECS_MALLOC(sizeof(char) * sz);
}

void ecs_unregister_component(ecs_world_t* w, int index) {
    if (!w) return;
    ecs_component_pool_t* comp = &(w->components[index]);
    ECS_FREE(comp->data);
    ECS_FREE(comp->enabled);
}

void ecs_register_system(ecs_world_t* w, ecs_system_func_t fn, int filter_count, int* filters) {
    if (!w) return;
    // ecs_system_pool_t* pool = &(w->systems[type]);
    ecs_system_t* sys = &(w->systems[w->system_top++]);
    sys->enabled = 1;
    sys->func = fn;
    sys->mask = 0;
    int max_count = 0;
    for (int i = 0; i < filter_count; i++) {
        sys->mask |= (1 << filters[i]);
        ecs_component_pool_t* comp = &(w->components[filters[i]]);
        if (comp->count > max_count) max_count = comp->count;
        // if (comp->)
    }
    sys->filter.entities_count = 0;
    sys->filter.entities = ECS_MALLOC(sizeof(ecs_entity_t) * max_count);
    memset(sys->filter.entities, 0, sizeof(ecs_entity_t) * max_count);
}

void ecs_unregister_system(ecs_world_t* w, ecs_system_func_t fn) {
    if (!w) return;
    // ecs_system_pool_t* pool = &(w->systems[type]);
    ecs_system_t* sys = NULL;
    for (int i = 0; i < w->system_top; i++) {
        ecs_system_t* s = &(w->systems[i]);
        if (s->enabled && s->func == fn) sys = s;
    }
    if (!sys) return;
    // ecs_system_t* sys = &(pool->systems[pool->top++]);
    sys->enabled = 0;
    sys->func = NULL;
    ECS_FREE(sys->filter.entities);
}

void ecs_entity_set_component(ecs_world_t* w, ecs_entity_t e, int comp, void* data) {
    if (!w) return;
    ecs_entity_internal_t* ent = &(w->entities[e-1]);
    int i = 0;
    if (ent->mask & (1 << comp)) {
        i = ent->components[comp];
    } else {
        i = get_free_component(w, comp);
        ent->components[comp] = i;
    }
    ecs_component_pool_t* pool = &(w->components[comp]);
    char* comp_data = ((char*)pool->data) + (pool->size * i);
    if (data) memcpy(comp_data, data, pool->size);
    ent->mask |= (1 << comp);
    update_filters(w, comp);
}

void* ecs_entity_get_component(ecs_world_t* w, ecs_entity_t e, int comp) {
    if (!w) return NULL;
    ecs_entity_internal_t* ee = &(w->entities[e-1]);
    if (!(ee->mask & (1 << comp))) return NULL;
    int index = ee->components[comp];
    if (index < 0) return NULL;
    ecs_component_pool_t* pool = &(w->components[comp]);
    void* data = ((char*)pool->data) + (index * pool->size);
    return data;
}

void ecs_entity_remove_component(ecs_world_t* w, ecs_entity_t e, int comp) {
    if (!w) return;
    ecs_entity_internal_t* ee = &(w->entities[e-1]);
    if (!(ee->mask & (1 << comp))) return;
    ee->mask &= ~(1 << comp);
    enable_component(w, comp, ee->components[comp], 0);
    ee->components[comp] = -1;
    update_filters(w, comp);
}

#endif /* ECS_IMPLEMENTATION */