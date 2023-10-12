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

#define ECS_STATE_ENABLED 0x1
#define ECS_STATE_LOADED 0x2

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
    int top;
    int size;
    int* data;
} stack_t;

static void stack_init(stack_t* s, int size) {
    s->top = 0;
    s->size = size;
    s->data = ECS_MALLOC(sizeof(int) * size);
}

static void stack_deinit(stack_t* s) {
    ECS_FREE(s->data);
}

static int stack_top(stack_t* s) {
    return s->data[s->top-1];
}

static void stack_push(stack_t* s, int i) {
    s->data[s->top] = i;
    s->top++;
}

static int stack_pop(stack_t* s) {
    int res = s->data[s->top-1];
    s->top--;
    return res;
}


typedef struct {
    char enabled;
    unsigned int mask;
    int* components;
} ecs_entity_internal_t;

typedef struct {
    int count;
    ecs_entity_internal_t* entities;
    stack_t available;
} ecs_entity_manager_t;

typedef struct {
    char state;
    int size;
    int count;
    stack_t available;
    void* data;
} ecs_component_pool_t;

typedef struct {
    int count;
    ecs_component_pool_t* pools;
    stack_t available;
} ecs_component_manager_t;

typedef struct {
    char state;
    char enabled;
    unsigned int mask;
    ecs_system_func_t func;
    stack_t entities;
} ecs_system_t;

typedef struct {
    int count;
    ecs_system_t* systems;
    stack_t available_systems;
} ecs_system_manager_t;

struct ecs_world_t {
    ecs_entity_manager_t entity_manager;
    ecs_component_manager_t component_manager;
    ecs_system_manager_t system_manager;

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

    ecs_entity_manager_t* em = &(world->entity_manager);
    ecs_component_manager_t* cm = &(world->component_manager);
    ecs_system_manager_t* sm = &(world->system_manager);

    // Entity Manager
    em->count = entities;
    int size = sizeof(ecs_entity_internal_t) * entities; 
    em->entities = malloc(size);
    memset(em->entities, 0, size);
    stack_init(&(ee->available), entities);
    ee->available.top = entities;

    for (int i = 0; i < entities; i++) {
        ecs_entity_internal_t* ee = &(em->entities[i]);
        ee->enabled = 0;
        ee->mask = 0;
        ee->components = ECS_MALLOC(sizeof(int) * components);
        ee->available.data[i] = entities - i;
    }

    // Component Manager
    cm->count = components;
    size = sizeof(ecs_component_pool_t) * components;
    cm->pools = ECS_MALLOC(size);
    stack_init(&(cm->available), components);
    cm->available.top = components;

    for (int i = 0; i < components; i++) {
        cm->available.data[i] = components - i - 1;
    }

    // System Manager
    sm->count = systems;
    size = sizeof(ecs_system_t) * systems;
    sm->systems = ECS_MALLOC(size);
    memset(sm->systems, 0, size);
    stack_init(&(sm->available), systems);
    sm->available.top = systems;

    for (int i = 0; i < systems; i++) {
        ecs_system_t* sys = &(sm->systems[i]);
        sys->enabled = 0;
        sys->mask = 0;
        sys->func = NULL;
        sm->available.data[i] = systems - i - 1;
    }

    world->entity_top = 0;
    world->system_top = 0;

    // world->filter.entities_count = 0;
    // world->filter.entities = ECS_MALLOC(sizeof(ecs_entity_t) * entities);

    world->entities = ECS_MALLOC(sizeof(ecs_entity_internal_t) * entities);
    world->components = ECS_MALLOC(sizeof(ecs_component_pool_t) * components);
    world->systems = ECS_MALLOC(sizeof(ecs_system_t) * systems);

    em->available.top = entities;
    em->available.size = entities;
    em->available.data = ECS_MALLOC(sizeof(int) * entities);

    for (int i = 0; i < entities; i++) {
        em->available.data[i] = entities - i;
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
    ecs_entity_manager_t* em = &(world->entity_manager);
    ecs_component_manager_t* cm = &(world->component_manager);
    ecs_system_manager_t* sm = &(world->system_manager);

    for (int i = 0; i < em->count; i++) {
        ECS_FREE(em->entities.components);
    }
    ECS_FREE(em->entities);
    ECS_FREE(em->available.data);

    for (int i = 0; i < cm->count; i++) {
        ecs_component_pool_t* pool = &(cm->pools[i]);
        ECS_FREE(pool->available.data);
        ECS_FREE(pool->data);
    }
    ECS_FREE(cm->pools);
    ECS_FREE(cm->available_pools.data);

    ECS_FREE(sm->systems);
    //

    ECS_FREE(w->filter.entities);
    for (int i = 0; i < w->max_entities; i++) {
        if (w->entities[i].enabled) ECS_FREE(w->entities[i].components);
    }
    ECS_FREE(w->entities);
    ECS_FREE(w->entitity_manager.
    for (int i = 0; i < w->max_components; i++) {
        ecs_component_pool_t* pool = &(w->components[i]);
        if (pool->size > 0) {
            ECS_FREE(pool->data);
            ECS_FREE(pool->available.data);
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
    ecs_entity_manager_t* em = &(w->entity_manager);
    ecs_component_manager_t* cm = &(w->component_manager);
    for (int i = 0; i < em->count; i++) {
        em->entities[i].state = 0;
        em->entities[i].mask = 0;
        em->available[i] = em->count - i;
    }


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
    ecs_entity_manager_t* em = &(w->entity_manager);
    if (em->available.top >= em->available.size) return e;
    int avail = stack_pop(&(em->available));
    ecs_entity_internal_t* ee = &(em->entities[avail-1]);
    ee->enabled = 1;
    ee->mask = 0;
    return avail;

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
    ecs_entity_manager_t* em = &(w->entity_manager);
    int index = e - 1;
    ecs_entity_internal_t* ent = &(em->entities[index]);
    ent->enabled = 0;
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
    comp->state = ECS_COMPONENT_ENABLED & ECS_COMPONENT_LOADED;
    comp->count = count;
    comp->size = size;
    if (comp->data)
        comp->data = ECS_REALLOC(size * count);
    else 
        comp->data = ECS_MALLOC(size * count);

    stack_t* stack = &(comp->available);
    stack->top = count;
    stack->size = count;
    if (stack->data)
        stack->data = (int*)ECS_REALLOC(sizeof(int) * count);
    else
        stack->data = (int*)ECS_MALLOC(sizeof(int) * count);

    for (int i = (count-1); i >= 0; i--) stack->data[i] = i;
}

void ecs_unregister_component(ecs_world_t* w, int index) {
    if (!w) return;
    ecs_component_pool_t* comp = &(w->components[index]);
    comp->state = 0;
}

void ecs_register_system(ecs_world_t* w, ecs_system_func_t fn, int filter_count, int* filters) {
    if (!w) return;
    ecs_entity_manager_t* em = &(w->entity_manager);
    ecs_component_manager_t* cm = &(w->component_manager);
    ecs_system_manager_t* sm = &(w->system_manager);
    stack_t* stack = &(sm->available_systems);
    if (stack->top >= stack->size) return;

    int index = stack_pop(stack);

    ecs_system_t* sys = &(sm->systems[index]);
    sys->enabled = 1;
    sys->func = fn;
    sys->mask = 0;
    int max_count = 0;

    for (int i = 0; i < filter_count; i++) {
        sys->mask |= (1 << filters[i]);
        ecs_component_pool_t* comp = &(em->pools);
        if (comp->count > max_count) max_count = comp->count;
    }
    sys->filter.entities_count = 0;
    sys->filter.entities = ECS_MALLOC(sizeof(ecs_entity_t) * max_count);

    for (int i = 0; i < em->count; i++) {
        ecs_entity_internal_t* ee = em->entities + i;
        if (ee->mask == sys->mask)
    }

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
