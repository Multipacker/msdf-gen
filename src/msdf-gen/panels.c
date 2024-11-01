// NOTE(simon): Tab functions

internal Tab *tab_create(State *state, Str8 name) {
    Tab *tab = state->tab_freelist;
    if (tab) {
        sll_stack_pop(state->tab_freelist);
    } else {
        tab = arena_push_struct_zero(state->arena, Tab);
    }

    memory_zero_struct(tab);

    tab->arena = arena_create();
    tab->name  = str8_copy(tab->arena, name);

    return tab;
}

internal Void tab_free(State *state, Tab *tab) {
    arena_destroy(tab->arena);
    sll_stack_push(state->tab_freelist, tab);
}

internal Void *tab_get_state(Tab *tab, U64 size) {
    Void *state = tab->view_state;

    if (!state) {
        state = arena_push_zero(tab->arena, size, 16);
        tab->view_state = state;
    }

    return state;
}

// NOTE(simon): Panel functions

internal Panel *panel_create(State *state) {
    Panel *panel = state->panel_freelist;
    if (panel) {
        sll_stack_pop(state->panel_freelist);
    } else {
        panel = arena_push_struct_zero(state->arena, Panel);
    }

    memory_zero_struct(panel);

    return panel;
}

internal PanelIterator panel_iterator_depth_first_pre_order(Panel *panel) {
    PanelIterator iterator = { 0 };

    if (panel->first) {
        iterator.next = panel->first;
        iterator.push_count = 1;
    } else {
        for (Panel *parent = panel; parent; parent = parent->parent) {
            if (parent->next) {
                iterator.next = parent->next;
                break;
            }
            ++iterator.pop_count;
        }
    }

    return iterator;
}

internal R2F32 rectangle_from_child_panel_parent_rectangle(Panel *child, R2F32 parent_rectangle) {
    R2F32 result = parent_rectangle;

    Panel *parent = child->parent;
    if (parent) {
        V2F32 parent_size = r2f32_size(parent_rectangle);
        for (Panel *panel = parent->first; panel != child; panel = panel->next) {
            result.min.values[parent->split_axis] += panel->percentage_of_parent * parent_size.values[parent->split_axis];
        }
        result.max.values[parent->split_axis] = result.min.values[parent->split_axis] + child->percentage_of_parent * parent_size.values[parent->split_axis];
    }

    return result;
}

internal R2F32 rectangle_from_panel(Panel *panel, R2F32 root_rectangle) {
    Arena_Temporary scratch = arena_get_scratch(0, 0);

    typedef struct WalkNode WalkNode; 
    struct WalkNode {
        WalkNode *next;
        Panel    *child;
    };
    WalkNode *first_walk_node = 0;
    for (Panel *p = panel; p->parent; p = p->parent) {
        WalkNode *node = arena_push_struct_zero(scratch.arena, WalkNode);
        node->child = p;
        sll_stack_push(first_walk_node, node);
    }

    R2F32 result = root_rectangle;
    for (WalkNode *node = first_walk_node; node; node = node->next) {
        result = rectangle_from_child_panel_parent_rectangle(node->child, result);
    }

    arena_end_temporary(scratch);
    return result;
}

// NOTE(simon): Panel-tab functions

internal Void panel_remove_tab(State *state, Panel *panel, Tab *tab) {
    if (panel->active_tab == tab) {
        if (tab->next) {
            panel->active_tab = tab->next;
        } else {
            panel->active_tab = tab->previous;
        }
    }
    dll_remove(panel->tab_first, panel->tab_last, tab);
}
