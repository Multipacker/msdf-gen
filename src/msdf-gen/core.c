internal Tab *tab_create(State *state, Str8 name) {
    U64 generation = 0;

    Tab *tab = state->tab_freelist;
    if (tab) {
        sll_stack_pop(state->tab_freelist);
        generation = tab->generation;
    } else {
        tab = arena_push_struct_zero(state->arena, Tab);
    }

    memory_zero_struct(tab);

    tab->generation = generation;

    tab->arena = arena_create();
    tab->name  = str8_copy(tab->arena, name);

    return tab;
}

internal Void tab_free(State *state, Tab *tab) {
    arena_destroy(tab->arena);
    ++tab->generation;
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

internal Tab *tab_from_handle(Handle handle) {
    Tab *result = (Tab *) handle.data;

    if (result && handle.generation != result->generation) {
        result = 0;
    }

    return result;
}

internal Handle handle_from_tab(Tab *tab) {
    Handle result = { 0 };

    if (tab) {
        result.data = tab;
        result.generation = tab->generation;
    }

    return result;
}



internal Panel *panel_create(State *state) {
    U64 generation = 0;

    Panel *panel = state->panel_freelist;
    if (panel) {
        sll_stack_pop(state->panel_freelist);
        generation = panel->generation;
    } else {
        panel = arena_push_struct_zero(state->arena, Panel);
    }

    memory_zero_struct(panel);
    panel->generation = generation;

    return panel;
}

internal Void panel_free(State *state, Panel *panel) {
    for (Tab *tab = panel->tab_first, *next = 0; tab; tab = next) {
        next = tab->next;
        panel_remove_tab(panel, tab);
        tab_free(state, tab);
    }

    ++panel->generation;
    sll_stack_push(state->panel_freelist, panel);
}

internal PanelIterator panel_iterator_depth_first_pre_order(Panel *panel, Panel *root) {
    PanelIterator iterator = { 0 };

    if (panel->first) {
        iterator.next = panel->first;
        iterator.push_count = 1;
    } else {
        for (Panel *parent = panel; parent && parent != root; parent = parent->parent) {
            if (parent->next) {
                iterator.next = parent->next;
                break;
            }
            ++iterator.pop_count;
        }
    }

    return iterator;
}

internal R2F32 rectangle_from_child_panel_parent_rectangle(Panel *parent, Panel *child, R2F32 parent_rectangle) {
    R2F32 result = parent_rectangle;

    if (parent) {
        V2F32 parent_size = r2f32_size(parent_rectangle);
        result.max.values[parent->split_axis] = result.min.values[parent->split_axis];
        for (Panel *panel = parent->first; panel; panel = panel->next) {
            result.max.values[parent->split_axis] += panel->percentage_of_parent * parent_size.values[parent->split_axis];
            if (panel == child) {
                break;
            }
            result.min.values[parent->split_axis] = result.max.values[parent->split_axis];
        }
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
        result = rectangle_from_child_panel_parent_rectangle(node->child->parent, node->child, result);
    }

    arena_end_temporary(scratch);
    return result;
}

internal Void panel_insert(Panel *parent, Panel *previous, Panel *child) {
    dll_insert_next_previous_zero(parent->first, parent->last, previous, child, next, previous, 0);
    ++parent->child_count;
    child->parent = parent;
}

internal Void panel_remove(Panel *parent, Panel *child) {
    dll_remove(parent->first, parent->last, child);
    child->next = child->previous = child->parent = 0;
    --parent->child_count;
}

internal Void panel_remove_tab(Panel *panel, Tab *tab) {
    if (tab_from_handle(panel->active_tab) == tab) {
        if (tab->next) {
            panel->active_tab = handle_from_tab(tab->next);
        } else if (tab->previous) {
            panel->active_tab = handle_from_tab(tab->previous);
        } else {
            panel->active_tab = handle_from_tab(0);
        }
    }
    dll_remove(panel->tab_first, panel->tab_last, tab);
}

internal Void panel_insert_tab(Panel *panel, Tab *previous_tab, Tab *tab) {
    dll_insert_next_previous_zero(panel->tab_first, panel->tab_last, previous_tab, tab, next, previous, 0);
    panel->active_tab = handle_from_tab(tab);
    ++panel->child_count;
}

internal Panel *panel_from_handle(Handle handle) {
    Panel *result = (Panel *) handle.data;

    if (result && handle.generation != result->generation) {
        result = 0;
    }
    return result;
}

internal Handle handle_from_panel(Panel *panel) {
    Handle result = { 0 };

    if (panel) {
        result.data = panel;
        result.generation = panel->generation;
    }

    return result;
}



internal Context *copy_context(Arena *arena, Context *context) {
    Context *result = arena_push_struct_zero(arena, Context);
    memory_copy(result, context, sizeof(*result));
    result->next = 0;
    return result;
}

internal Void push_context_internal(Context *context) {
    State *state = global_state;
    Context *copy = copy_context(frame_arena(), context);
    sll_stack_push(state->context_stack, copy);
}

internal Void pop_context(Void) {
    State *state = global_state;
    sll_stack_pop(state->context_stack);
}

internal Context *top_context(Void) {
    State *state = global_state;
    Context *result = state->context_stack;
    return result;
}



internal Void push_command_internal(CommandKind kind, Context *context) {
    State *state = global_state;

    CommandNode *node = arena_push_struct_zero(state->command_arena, CommandNode);
    node->command.kind = kind;
    node->command.context = copy_context(state->command_arena, context);
    sll_queue_push(state->commands.first, state->commands.last, node);
}



internal B32 drag_is_active(Void) {
    State *state = global_state;
    B32 result = (state->drag_state == DragState_Dragging || state->drag_state == DragState_Dropping);
    return result;
}

internal Void drag_begin(Void) {
    State *state = global_state;

    if (!drag_is_active()) {
        state->drag_state = DragState_Dragging;
    }
}

internal B32 drag_drop(Void) {
    State *state = global_state;

    B32 result = false;
    if (state->drag_state == DragState_Dropping) {
        result = true;
        state->drag_state = DragState_None;
    }

    return result;
}

internal Void drag_cancel(Void) {
    State *state = global_state;

    if (drag_is_active()) {
        state->drag_state = DragState_None;
    }
}



internal V4F32 color_from_theme(ThemeColor color) {
    State *state = global_state;
    V4F32 result = state->theme.colors[color];
    return result;
}

internal UI_Palette palette_from_code(PaletteCode code) {
    State *state = global_state;
    UI_Palette result = state->palettes[code];
    return result;
}



internal Void request_frame(Void) {
    State *state = global_state;
    state->frames_to_render = 4;
}

internal Arena *frame_arena(Void) {
    State *state = global_state;
    Arena *result = state->frame_arenas[state->frame_index % array_count(state->frame_arenas)];
    return result;
}

typedef struct CommandItem CommandItem;
struct CommandItem {
    CommandKind command;
    Str8 name;
    Str8 description;
    FuzzyMatchList fuzzy_matches;
};

internal S64 command_item_compare(CommandItem a, CommandItem b) {
    S64 result = 0;

    // NOTE(simon): More matches mean make items appear earlier.
    if (result == 0) {
        if (a.fuzzy_matches.count > b.fuzzy_matches.count) {
            result = -1;
        } else if (a.fuzzy_matches.count < b.fuzzy_matches.count) {
            result = 1;
        }
    }

    // NOTE(simon): Earlier first matches make items appear earlier.
    if (result == 0) {
        U64 a_first_fuzzy = U64_MAX;
        for (FuzzyMatch *match = a.fuzzy_matches.first; match; match = match->next) {
            a_first_fuzzy = u64_min(match->min, a_first_fuzzy);
        }

        U64 b_first_fuzzy = U64_MAX;
        for (FuzzyMatch *match = b.fuzzy_matches.first; match; match = match->next) {
            b_first_fuzzy = u64_min(match->min, b_first_fuzzy);
        }

        if (a_first_fuzzy < b_first_fuzzy) {
            result = -1;
        } else if (a_first_fuzzy > b_first_fuzzy) {
            result = 1;
        }
    }

    // NOTE(simon): Fallback on lexigraphical ordering.
    if (result == 0) {
        result = str8_compare_ascii(a.name, b.name);
    }

    return result;
}

internal Void quicksort(CommandItem *commands, U64 command_count) {
    if (command_count <= 1) {
        return;
    }

    // NOTE(simon): Choose pivot by median of three.
    // NOTE(simon): This makes the sort unstable!!!
    U64 pivot_index  = 0;
    U64 first_index  = 0;
    U64 middle_index = command_count / 2;
    U64 last_index   = command_count - 1;
    if (command_item_compare(commands[first_index], commands[middle_index]) > 0 ^ command_item_compare(commands[first_index], commands[last_index]) > 0) {
        pivot_index = first_index;
    } else if (command_item_compare(commands[middle_index], commands[first_index]) < 0 ^ command_item_compare(commands[middle_index], commands[last_index]) < 0) {
        pivot_index = middle_index;
    } else {
        pivot_index = last_index;
    }

    // NOTE(simon): Swap pivot to start of list
    swap(commands[0], commands[pivot_index], CommandItem);
    pivot_index = 0;

    // NOTE(simon): Partition
    U64 low_index = 1;
    U64 high_index = command_count;
    for (;;) {
        while (low_index < high_index && command_item_compare(commands[low_index], commands[pivot_index]) <= 0) {
            ++low_index;
        }

        while (low_index < high_index && command_item_compare(commands[pivot_index], commands[high_index - 1]) <= 0) {
            --high_index;
        }

        if (low_index < high_index) {
            swap(commands[low_index], commands[high_index - 1], CommandItem);
        } else {
            break;
        }
    }

    // NOTE(simon): Swap pivot to the middle.
    pivot_index = low_index - 1;
    swap(commands[0], commands[pivot_index], CommandItem);

    // NOTE(simon): Recurse
    quicksort(commands, pivot_index);
    quicksort(&commands[pivot_index + 1], command_count - pivot_index - 1);
}

internal Void update(Void) {
    State *state = global_state;

    // NOTE(simon): Set up base context
    {
        Handle active_panel = state->active_panel;
        if (panel_from_handle(active_panel)) {
            state->base_context.tab = panel_from_handle(active_panel)->active_tab;
        }
        state->base_context.panel = active_panel;
        state->base_context.codepoint = state->selected_codepoint;
        state->context_stack->next = 0;
        state->context_stack = &state->base_context;
    }

    arena_reset(frame_arena());

    // NOTE(simon): Trigger an auto save once every 5 seconds.
    if (os_now_nanoseconds() - state->previous_auto_save > (U64) (5 * 1e9)) {
        state->previous_auto_save = os_now_nanoseconds();
        push_command(Command_SaveProject);
    }

    Gfx_EventList events = { 0 };

    local U32 depth = 0;
    if (depth == 0) {
        ++depth;
        events = gfx_get_events(frame_arena(), state->frames_to_render == 0);
        --depth;
    }

    UI_EventList ui_events = { 0 };

    typedef struct Binding Binding;
    struct Binding {
        Gfx_Key         key;
        Gfx_KeyModifier modifiers;
        CommandKind     command;
    };

    Binding bindings[] = {
            { Gfx_Key_W,         Gfx_KeyModifier_Control,                         Command_CloseTab,             },
            { Gfx_Key_Tab,       Gfx_KeyModifier_Shift | Gfx_KeyModifier_Control, Command_PreviousTab,          },
            { Gfx_Key_Tab,       Gfx_KeyModifier_Control,                         Command_NextTab,              },
            { Gfx_Key_N,         Gfx_KeyModifier_Control,                         Command_NextTheme,            },
            { Gfx_Key_P,         Gfx_KeyModifier_Control,                         Command_PreviousTheme,        },
            { Gfx_Key_Left,      Gfx_KeyModifier_Shift | Gfx_KeyModifier_Control, Command_SelectWordLeft,       },
            { Gfx_Key_Up,        Gfx_KeyModifier_Shift | Gfx_KeyModifier_Control, Command_SelectWordUp,         },
            { Gfx_Key_Right,     Gfx_KeyModifier_Shift | Gfx_KeyModifier_Control, Command_SelectWordRight,      },
            { Gfx_Key_Down,      Gfx_KeyModifier_Shift | Gfx_KeyModifier_Control, Command_SelectWordDown,       },
            { Gfx_Key_Left,      Gfx_KeyModifier_Shift,                           Command_SelectCharacterLeft,  },
            { Gfx_Key_Up,        Gfx_KeyModifier_Shift,                           Command_SelectCharacterUp,    },
            { Gfx_Key_Right,     Gfx_KeyModifier_Shift,                           Command_SelectCharacterRight, },
            { Gfx_Key_Down,      Gfx_KeyModifier_Shift,                           Command_SelectCharacterDown,  },
            { Gfx_Key_Left,      Gfx_KeyModifier_Control,                         Command_MoveWordLeft,         },
            { Gfx_Key_Up,        Gfx_KeyModifier_Control,                         Command_MoveWordUp,           },
            { Gfx_Key_Right,     Gfx_KeyModifier_Control,                         Command_MoveWordRight,        },
            { Gfx_Key_Down,      Gfx_KeyModifier_Control,                         Command_MoveWordDown,         },
            { Gfx_Key_Left,      0,                                               Command_MoveCharacterLeft,    },
            { Gfx_Key_Up,        0,                                               Command_MoveCharacterUp,      },
            { Gfx_Key_Right,     0,                                               Command_MoveCharacterRight,   },
            { Gfx_Key_Down,      0,                                               Command_MoveCharacterDown,    },
            { Gfx_Key_Home,      Gfx_KeyModifier_Shift,                           Command_SelectHome,           },
            { Gfx_Key_End,       Gfx_KeyModifier_Shift,                           Command_SelectEnd,            },
            { Gfx_Key_Home,      0,                                               Command_MoveHome,             },
            { Gfx_Key_End,       0,                                               Command_MoveEnd,              },
            { Gfx_Key_PageUp,    Gfx_KeyModifier_Shift,                           Command_SelectPageUp,         },
            { Gfx_Key_PageDown,  Gfx_KeyModifier_Shift,                           Command_SelectPageDown,       },
            { Gfx_Key_PageUp,    0,                                               Command_MovePageUp,           },
            { Gfx_Key_PageDown,  0,                                               Command_MovePageDown,         },
            { Gfx_Key_Home,      Gfx_KeyModifier_Shift | Gfx_KeyModifier_Control, Command_SelectWholeUp,        },
            { Gfx_Key_End,       Gfx_KeyModifier_Shift | Gfx_KeyModifier_Control, Command_SelectWholeDown,      },
            { Gfx_Key_Home,      Gfx_KeyModifier_Control,                         Command_MoveWholeUp,          },
            { Gfx_Key_End,       Gfx_KeyModifier_Control,                         Command_MoveWholeDown,        },
            { Gfx_Key_Backspace, Gfx_KeyModifier_Control,                         Command_RemoveWord,           },
            { Gfx_Key_Delete,    Gfx_KeyModifier_Control,                         Command_DeleteWord,           },
            { Gfx_Key_Backspace, 0,                                               Command_RemoveCharacter,      },
            { Gfx_Key_Delete,    0,                                               Command_DeleteCharacter,      },
            { Gfx_Key_A,         Gfx_KeyModifier_Control,                         Command_SelectAll,            },
            { Gfx_Key_C,         Gfx_KeyModifier_Control,                         Command_Copy,                 },
            { Gfx_Key_V,         Gfx_KeyModifier_Control,                         Command_Paste,                },
            { Gfx_Key_X,         Gfx_KeyModifier_Control,                         Command_Cut,                  },
            { Gfx_Key_T,         Gfx_KeyModifier_Control,                         Command_ToggleListView,       },
            { Gfx_Key_F1,        0                      ,                         Command_OpenCommandLister,    },
            { Gfx_Key_Return,    0                      ,                         Command_Accept,               },
            { Gfx_Key_Escape,    0                      ,                         Command_Cancel,               },
            { Gfx_Key_H,         Gfx_KeyModifier_Control,                         Command_FocusPanelLeft,       },
            { Gfx_Key_J,         Gfx_KeyModifier_Control,                         Command_FocusPanelDown,       },
            { Gfx_Key_K,         Gfx_KeyModifier_Control,                         Command_FocusPanelUp,         },
            { Gfx_Key_L,         Gfx_KeyModifier_Control,                         Command_FocusPanelRight,      },
    };

    // NOTE(simon): Process key bindings.
    for (Gfx_Event *event = events.first, *next; event; event = next) {
        next = event->next;

        if (event->kind != Gfx_EventKind_KeyPress) {
            continue;
        }

        for (U64 i = 0; i < array_count(bindings); ++i) {
            Binding binding = bindings[i];
            if (event->key == binding.key && (event->key_modifiers & binding.modifiers) == binding.modifiers && (~event->key_modifiers & ~binding.modifiers) == ~binding.modifiers) {
                push_command(binding.command);
                dll_remove(events.first, events.last, event);
                break;
            }
        }
    }

    // NOTE(simon): Consume events.
    // TODO(simon): This should be done per window.
    for (Gfx_Event *event = events.first, *next; event; event = next) {
        next = event->next;
        B32 consume = false;
        UI_Event *ui_event = 0;

        if (event->kind == Gfx_EventKind_Quit) {
            consume = true;
            state->running = false;
        } else if (event->kind == Gfx_EventKind_FileDrop) {
            consume = true;
            push_command(Command_LoadFont, .path = event->path);
        } else if (event->kind == Gfx_EventKind_KeyPress || event->kind == Gfx_EventKind_KeyRelease || event->kind == Gfx_EventKind_Text || event->kind == Gfx_EventKind_Scroll) {
            consume = true;
            UI_EventKind kind = UI_EventKind_Null;
            switch (event->kind) {
                case Gfx_EventKind_Null:       kind = UI_EventKind_Null;       break;
                case Gfx_EventKind_Quit:       kind = UI_EventKind_Null;       break;
                case Gfx_EventKind_KeyPress:   kind = UI_EventKind_KeyPress;   break;
                case Gfx_EventKind_KeyRelease: kind = UI_EventKind_KeyRelease; break;
                case Gfx_EventKind_MouseMove:  kind = UI_EventKind_Null;       break;
                case Gfx_EventKind_Text:       kind = UI_EventKind_Text;       break;
                case Gfx_EventKind_Scroll:     kind = UI_EventKind_Scroll;     break;
                case Gfx_EventKind_Resize:     kind = UI_EventKind_Null;       break;
                case Gfx_EventKind_FileDrop:   kind = UI_EventKind_Null;       break;
                case Gfx_EventKind_Wakeup:     kind = UI_EventKind_Null;       break;
                case Gfx_EventKind_COUNT:      kind = UI_EventKind_Null;       break;
            }
            ui_event = arena_push_struct_zero(frame_arena(), UI_Event);
            ui_event->kind      = kind;
            ui_event->text      = event->text;
            ui_event->position  = event->position;
            ui_event->scroll    = event->scroll;
            ui_event->key       = event->key;
            ui_event->modifiers = event->key_modifiers;
        }

        if (drag_is_active() && event->kind == Gfx_EventKind_KeyRelease && event->key == Gfx_Key_MouseLeft) {
            state->drag_state = DragState_Dropping;
        }

        if (ui_event && ui_event->kind != UI_EventKind_Null) {
            ui_event_list_push_event(&ui_events, ui_event);
        }

        if (consume) {
            dll_remove(events.first, events.last, event);
        }
    }

    // NOTE(simon): If there are UI events, they will potentially trigger UI
    // changes, so render more frames.
    if (ui_events.first) {
        request_frame();
    }

    // NOTE(simon): Execute commands
    if (depth == 0) {
        for (CommandNode *node = state->commands.first; node; node = node->next) {
            request_frame();
            UI_Event *ui_event = 0;
            Context *command_context = node->command.context;
            switch (node->command.kind) {
                case Command_FocusPanel: {
                    state->active_panel = command_context->panel;
                } break;
                case Command_FocusPanelLeft:
                case Command_FocusPanelUp:
                case Command_FocusPanelRight:
                case Command_FocusPanelDown: {
                    // NOTE(simon): Extract direction and side from direction.
                    Axis2 movement_axis = Axis2_Invalid;
                    Side  movement_side = Side_Invalid;
                    if (node->command.kind == Command_FocusPanelLeft || node->command.kind == Command_FocusPanelRight) {
                        movement_axis = Axis2_X;
                    }
                    if (node->command.kind == Command_FocusPanelUp || node->command.kind == Command_FocusPanelDown) {
                        movement_axis = Axis2_Y;
                    }
                    if (node->command.kind == Command_FocusPanelLeft || node->command.kind == Command_FocusPanelUp) {
                        movement_side = Side_Min;
                    }
                    if (node->command.kind == Command_FocusPanelRight || node->command.kind == Command_FocusPanelDown) {
                        movement_side = Side_Max;
                    }

                    Panel *panel = panel_from_handle(command_context->panel);
                    V2F32 panel_center = { 0 };
                    Panel *sibling = 0;

                    // TODO(simon): This should not be based on animation state
                    // as that is less predictable and not consistent unless
                    // you wait out the animations.

                    // NOTE(simon): Find closest sibling along our movement axis.
                    if (panel) {
                        panel_center = r2f32_center(panel->animated_rectangle_percentage);
                        sibling = panel;

                        if (movement_side == Side_Min) {
                            while (sibling->parent && !(sibling->parent->split_axis == movement_axis && sibling->previous)) {
                                sibling = sibling->parent;
                            }

                            if (sibling && sibling->previous) {
                                sibling = sibling->previous;
                            }
                        } else if (movement_side == Side_Max) {
                            while (sibling->parent && !(sibling->parent->split_axis == movement_axis && sibling->next)) {
                                sibling = sibling->parent;
                            }

                            if (sibling && sibling->next) {
                                sibling = sibling->next;
                            }
                        }
                    }

                    // NOTE(simon): Find the closest child in the selected sibling.
                    Panel *best_child = 0;
                    F32 best_distance = f32_infinity();
                    if (sibling->first) {
                        for (Panel *child = sibling; child; child = panel_iterator_depth_first_pre_order(child, sibling).next) {
                            if (child->first) {
                                continue;
                            }

                            V2F32 child_center = r2f32_center(child->animated_rectangle_percentage);
                            F32 distance = f32_abs(panel_center.x - child_center.x) + f32_abs(panel_center.y - child_center.y);

                            if (distance < best_distance) {
                                best_distance = distance;
                                best_child = child;
                            }
                        }
                    } else {
                        best_child = sibling;
                    }

                    if (best_child) {
                        state->active_panel = handle_from_panel(best_child);
                    }
                } break;
                case Command_ClosePanel: {
                    Panel *panel = panel_from_handle(command_context->panel);

                    if (panel && panel->parent) {
                        Panel *parent = panel->parent;
                        if (parent->child_count == 2) {
                            // NOTE(simon): Merge the panel that we keep with our grandparent.
                            Panel *discard_child = panel;
                            Panel *keep_child    = parent->first == discard_child ? parent->last : parent->first;
                            Panel *grandparent   = parent->parent;
                            Panel *previous      = parent->previous;
                            F32 parent_percentage = parent->percentage_of_parent;

                            panel_remove(parent, keep_child);

                            // NOTE(simon): Insert the panel we are keeping into the tree.
                            keep_child->percentage_of_parent = parent->percentage_of_parent;
                            if (grandparent) {
                                panel_remove(grandparent, parent);
                                panel_insert(grandparent, previous, keep_child);
                            } else {
                                state->panel_root = keep_child;
                            }

                            // NOTE(simon): Update active panel, recursing into children if needed.
                            if (panel_from_handle(state->active_panel) == discard_child) {
                                Panel *next_panel = keep_child;
                                while (next_panel->first) {
                                    next_panel = next_panel->first;
                                }
                                state->active_panel = handle_from_panel(next_panel);
                            }

                            panel_free(state, discard_child);
                            panel_free(state, parent);

                            // NOTE(simon): If the split axis of keep child and grandparent are the same, merge their children.
                            if (grandparent && keep_child->first && grandparent->split_axis == keep_child->split_axis) {
                                Panel *child_previous = keep_child->previous;
                                panel_remove(grandparent, keep_child);

                                for (Panel *child = keep_child->first, *next; child; child = next) {
                                    next = child->next;

                                    panel_remove(keep_child, child);
                                    panel_insert(grandparent, child_previous, child);
                                    child_previous = child;
                                    child->percentage_of_parent *= keep_child->percentage_of_parent;
                                }

                                panel_free(state, keep_child);
                            }
                        } else {
                            // NOTE(simon): Remove panel and adjust children to fill the empty space.
                            Panel *next = 0;
                            if (panel->next) {
                                next = panel->next;
                            } else if (panel->previous) {
                                next = panel->previous;
                            }
                            panel_remove(parent, panel);

                            for (Panel *child = parent->first; child; child = child->next) {
                                child->percentage_of_parent /= 1.0f - panel->percentage_of_parent;
                            }

                            // NOTE(simon): Update active panel, recursing into children if needed.
                            if (panel_from_handle(state->active_panel) == panel) {
                                Panel *next_panel = next;
                                while (next_panel->first) {
                                    next_panel = next_panel->first;
                                }
                                state->active_panel = handle_from_panel(next_panel);
                            }

                            panel_free(state, panel);
                        }
                    }
                } break;
                case Command_SplitPanel: {
                    Side side = side_from_direction2(command_context->direction);
                    Axis2 axis = axis2_from_direction2(command_context->direction);

                    Panel *split_panel = panel_from_handle(command_context->destination_panel);
                    if (split_panel && command_context->direction != Direction2_Invalid) {
                        Panel *parent = split_panel->parent;

                        Panel *new_panel = 0;
                        if (parent && axis == parent->split_axis) {
                            Panel *next = panel_create(state);
                            panel_insert(parent, side == Side_Max ? split_panel : split_panel->previous, next);
                            next->percentage_of_parent = 1.0f / (F32) parent->child_count;
                            for (Panel *child = parent->first; child; child = child->next) {
                                if (child != next) {
                                    child->percentage_of_parent *= (F32) (parent->child_count - 1) / (F32) parent->child_count;
                                }
                            }
                            state->active_panel = handle_from_panel(next);
                            new_panel = next;
                        } else {
                            Panel *previous_previous = split_panel->previous;
                            Panel *previous_parent = parent;
                            Panel *new_parent = panel_create(state);
                            new_parent->percentage_of_parent = split_panel->percentage_of_parent;
                            if (previous_parent) {
                                panel_remove(previous_parent, split_panel);
                                panel_insert(previous_parent, previous_previous, new_parent);
                            } else {
                                state->panel_root = new_parent;
                            }
                            Panel *left = split_panel;
                            Panel *right = panel_create(state);
                            new_panel = right;
                            if (side == Side_Min) {
                                swap(left, right, Panel *);
                            }

                            panel_insert(new_parent, 0, left);
                            panel_insert(new_parent, left, right);
                            new_parent->split_axis = axis;
                            left->percentage_of_parent = 0.5f;
                            right->percentage_of_parent = 0.5f;
                            state->active_panel = handle_from_panel(new_panel);
                        }

                        if (new_panel->previous) {
                            new_panel->animated_rectangle_percentage = new_panel->previous->animated_rectangle_percentage;
                            new_panel->animated_rectangle_percentage.min.values[axis] = new_panel->animated_rectangle_percentage.max.values[axis];
                        }
                        if (new_panel->next) {
                            new_panel->animated_rectangle_percentage = new_panel->next->animated_rectangle_percentage;
                            new_panel->animated_rectangle_percentage.max.values[axis] = new_panel->animated_rectangle_percentage.min.values[axis];
                        }

                        Panel *move_panel = panel_from_handle(command_context->panel);
                        Tab *move_tab = tab_from_handle(command_context->tab);

                        if (new_panel && move_panel && move_tab) {
                            panel_remove_tab(move_panel, move_tab);
                            panel_insert_tab(new_panel, new_panel->tab_last, move_tab);
                            new_panel->active_tab = handle_from_tab(move_tab);

                            if (!move_panel->tab_first && move_panel != state->panel_root && move_panel != new_panel->next && move_panel != new_panel->previous) {
                                push_command(Command_ClosePanel, .panel = handle_from_panel(move_panel));
                            }
                        }
                    }
                } break;
                case Command_OpenTab: {
                    Panel *panel = panel_from_handle(command_context->panel);
                    if (panel) {
                        TabSpecification *tab_spec = tab_specification_from_string(command_context->tab_specification);
                        Tab *tab = tab_create(state, tab_spec->display_name);
                        tab->build_view = tab_spec->build;
                        panel_insert_tab(panel, panel->tab_last, tab);
                    }
                } break;
                case Command_CloseTab: {
                    Panel *panel = panel_from_handle(command_context->panel);
                    Tab *tab = tab_from_handle(command_context->tab);
                    if (panel && tab) {
                        panel_remove_tab(panel, tab);
                        tab_free(state, tab);
                    }
                } break;
                case Command_PreviousTab: {
                    Panel *panel = panel_from_handle(command_context->panel);
                    if (panel) {
                        Tab *next_tab = tab_from_handle(panel->active_tab);
                        if (next_tab->previous) {
                            next_tab = next_tab->previous;
                        } else if (panel->tab_last) {
                            next_tab = panel->tab_last;
                        }

                        panel->active_tab = handle_from_tab(next_tab);
                    }
                } break;
                case Command_NextTab: {
                    Panel *panel = panel_from_handle(command_context->panel);
                    if (panel) {
                        Tab *next_tab = tab_from_handle(panel->active_tab);
                        if (next_tab->next) {
                            next_tab = next_tab->next;
                        } else if (panel->tab_first) {
                            next_tab = panel->tab_first;
                        }

                        panel->active_tab = handle_from_tab(next_tab);
                    }
                } break;
                case Command_MoveTab: {
                    Panel *panel = panel_from_handle(command_context->panel);
                    Tab   *tab   = tab_from_handle(command_context->tab);
                    Panel *destination_panel = panel_from_handle(command_context->destination_panel);
                    Tab   *previous_tab      = tab_from_handle(command_context->previous_tab);

                    if (panel && destination_panel && tab && tab != previous_tab) {
                        panel_remove_tab(panel, tab);
                        panel_insert_tab(destination_panel, previous_tab, tab);
                        state->active_panel = handle_from_panel(destination_panel);

                        if (!panel->tab_first && panel != state->panel_root) {
                            push_command(Command_ClosePanel, .panel = command_context->panel);
                        }
                    }

                } break;
                case Command_SaveProject: {
                    // TODO(simon): Replace this with a proper structured text format
                    Arena_Temporary scratch = arena_get_scratch(0, 0);
                    Str8List config = { 0 };
                    str8_list_push(scratch.arena, &config, str8_format(scratch.arena, "codepoint: %lu\n", state->selected_codepoint));
                    os_file_write(str8_literal("msdf.config"), config);
                    arena_end_temporary(scratch);
                } break;
                case Command_SelectCodepoint: {
                    U32 codepoint = command_context->codepoint;
                    if (codepoint <= 0x10FFFF) {
                        state->selected_codepoint = codepoint;
                    }
                } break;
                case Command_NextTheme: {
                    state->theme_index = (state->theme_index + 1) % array_count(global_themes);
                    state->target_theme = global_themes[state->theme_index];
                } break;
                case Command_PreviousTheme: {
                    state->theme_index = (state->theme_index + array_count(global_themes) - 1) % array_count(global_themes);
                    state->target_theme = global_themes[state->theme_index];
                } break;
                case Command_SelectWordLeft: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Navigation;
                    ui_event->delta.x = -1;
                    ui_event->unit = UI_EventDeltaUnit_Word;
                    ui_event->flags = UI_EventFlag_KeepMark;
                } break;
                case Command_SelectWordUp: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Navigation;
                    ui_event->delta.y = -1;
                    ui_event->unit = UI_EventDeltaUnit_Word;
                    ui_event->flags = UI_EventFlag_KeepMark;
                } break;
                case Command_SelectWordRight: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Navigation;
                    ui_event->delta.x = 1;
                    ui_event->unit = UI_EventDeltaUnit_Word;
                    ui_event->flags = UI_EventFlag_KeepMark;
                } break;
                case Command_SelectWordDown: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Navigation;
                    ui_event->delta.y = 1;
                    ui_event->unit = UI_EventDeltaUnit_Word;
                    ui_event->flags = UI_EventFlag_KeepMark;
                } break;
                case Command_SelectCharacterLeft: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Navigation;
                    ui_event->delta.x = -1;
                    ui_event->unit = UI_EventDeltaUnit_Character;
                    ui_event->flags = UI_EventFlag_KeepMark;
                } break;
                case Command_SelectCharacterUp: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Navigation;
                    ui_event->delta.y = -1;
                    ui_event->unit = UI_EventDeltaUnit_Character;
                    ui_event->flags = UI_EventFlag_KeepMark;
                } break;
                case Command_SelectCharacterRight: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Navigation;
                    ui_event->delta.x = 1;
                    ui_event->unit = UI_EventDeltaUnit_Character;
                    ui_event->flags = UI_EventFlag_KeepMark;
                } break;
                case Command_SelectCharacterDown: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Navigation;
                    ui_event->delta.y = 1;
                    ui_event->unit = UI_EventDeltaUnit_Character;
                    ui_event->flags = UI_EventFlag_KeepMark;
                } break;
                case Command_MoveWordLeft: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Navigation;
                    ui_event->delta.x = -1;
                    ui_event->unit = UI_EventDeltaUnit_Word;
                    ui_event->flags = UI_EventFlag_PickSelectSide | UI_EventFlag_ZeroDeltaOnSelection;
                } break;
                case Command_MoveWordUp: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Navigation;
                    ui_event->delta.y = -1;
                    ui_event->unit = UI_EventDeltaUnit_Word;
                    ui_event->flags = UI_EventFlag_PickSelectSide | UI_EventFlag_ZeroDeltaOnSelection;
                } break;
                case Command_MoveWordRight: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Navigation;
                    ui_event->delta.x = 1;
                    ui_event->unit = UI_EventDeltaUnit_Word;
                    ui_event->flags = UI_EventFlag_PickSelectSide | UI_EventFlag_ZeroDeltaOnSelection;
                } break;
                case Command_MoveWordDown: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Navigation;
                    ui_event->delta.y = 1;
                    ui_event->unit = UI_EventDeltaUnit_Word;
                    ui_event->flags = UI_EventFlag_PickSelectSide | UI_EventFlag_ZeroDeltaOnSelection;
                } break;
                case Command_MoveCharacterLeft: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Navigation;
                    ui_event->delta.x = -1;
                    ui_event->unit = UI_EventDeltaUnit_Character;
                    ui_event->flags = UI_EventFlag_PickSelectSide | UI_EventFlag_ZeroDeltaOnSelection;
                } break;
                case Command_MoveCharacterUp: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Navigation;
                    ui_event->delta.y = -1;
                    ui_event->unit = UI_EventDeltaUnit_Character;
                    ui_event->flags = UI_EventFlag_PickSelectSide | UI_EventFlag_ZeroDeltaOnSelection;
                } break;
                case Command_MoveCharacterRight: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Navigation;
                    ui_event->delta.x = 1;
                    ui_event->unit = UI_EventDeltaUnit_Character;
                    ui_event->flags = UI_EventFlag_PickSelectSide | UI_EventFlag_ZeroDeltaOnSelection;
                } break;
                case Command_MoveCharacterDown: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Navigation;
                    ui_event->delta.y = 1;
                    ui_event->unit = UI_EventDeltaUnit_Character;
                    ui_event->flags = UI_EventFlag_PickSelectSide | UI_EventFlag_ZeroDeltaOnSelection;
                } break;
                case Command_SelectHome: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Navigation;
                    ui_event->delta.x = -1;
                    ui_event->unit = UI_EventDeltaUnit_Line;
                    ui_event->flags = UI_EventFlag_KeepMark;
                } break;
                case Command_SelectEnd: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Navigation;
                    ui_event->delta.x = 1;
                    ui_event->unit = UI_EventDeltaUnit_Line;
                    ui_event->flags = UI_EventFlag_KeepMark;
                } break;
                case Command_MoveHome: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Navigation;
                    ui_event->delta.x = -1;
                    ui_event->unit = UI_EventDeltaUnit_Line;
                    ui_event->flags = UI_EventFlag_PickSelectSide | UI_EventFlag_ZeroDeltaOnSelection;
                } break;
                case Command_MoveEnd: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Navigation;
                    ui_event->delta.x = 1;
                    ui_event->unit = UI_EventDeltaUnit_Line;
                    ui_event->flags = UI_EventFlag_PickSelectSide | UI_EventFlag_ZeroDeltaOnSelection;
                } break;
                case Command_SelectPageUp: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Navigation;
                    ui_event->delta.y = -1;
                    ui_event->unit = UI_EventDeltaUnit_Page;
                    ui_event->flags = UI_EventFlag_KeepMark;
                } break;
                case Command_SelectPageDown: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Navigation;
                    ui_event->delta.y = 1;
                    ui_event->unit = UI_EventDeltaUnit_Page;
                    ui_event->flags = UI_EventFlag_KeepMark;
                } break;
                case Command_MovePageUp: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Navigation;
                    ui_event->delta.y = -1;
                    ui_event->unit = UI_EventDeltaUnit_Page;
                    ui_event->flags = UI_EventFlag_PickSelectSide | UI_EventFlag_ZeroDeltaOnSelection;
                } break;
                case Command_MovePageDown: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Navigation;
                    ui_event->delta.y = 1;
                    ui_event->unit = UI_EventDeltaUnit_Page;
                    ui_event->flags = UI_EventFlag_PickSelectSide | UI_EventFlag_ZeroDeltaOnSelection;
                } break;
                case Command_SelectWholeUp: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Navigation;
                    ui_event->delta.x = -1;
                    ui_event->unit = UI_EventDeltaUnit_Whole;
                    ui_event->flags = UI_EventFlag_KeepMark;
                } break;
                case Command_SelectWholeDown: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Navigation;
                    ui_event->delta.x = 1;
                    ui_event->unit = UI_EventDeltaUnit_Whole;
                    ui_event->flags = UI_EventFlag_KeepMark;
                } break;
                case Command_MoveWholeUp: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Navigation;
                    ui_event->delta.x = -1;
                    ui_event->unit = UI_EventDeltaUnit_Whole;
                    ui_event->flags = UI_EventFlag_PickSelectSide | UI_EventFlag_ZeroDeltaOnSelection;
                } break;
                case Command_MoveWholeDown: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Navigation;
                    ui_event->delta.x = 1;
                    ui_event->unit = UI_EventDeltaUnit_Whole;
                    ui_event->flags = UI_EventFlag_PickSelectSide | UI_EventFlag_ZeroDeltaOnSelection;
                } break;
                case Command_RemoveWord: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Edit;
                    ui_event->delta.x = -1;
                    ui_event->unit = UI_EventDeltaUnit_Word;
                    ui_event->flags = UI_EventFlag_ZeroDeltaOnSelection | UI_EventFlag_Delete;
                } break;
                case Command_DeleteWord: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Edit;
                    ui_event->delta.x = 1;
                    ui_event->unit = UI_EventDeltaUnit_Word;
                    ui_event->flags = UI_EventFlag_ZeroDeltaOnSelection | UI_EventFlag_Delete;
                } break;
                case Command_RemoveCharacter: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Edit;
                    ui_event->delta.x = -1;
                    ui_event->unit = UI_EventDeltaUnit_Character;
                    ui_event->flags = UI_EventFlag_ZeroDeltaOnSelection | UI_EventFlag_Delete;
                } break;
                case Command_DeleteCharacter: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Edit;
                    ui_event->delta.x = 1;
                    ui_event->unit = UI_EventDeltaUnit_Character;
                    ui_event->flags = UI_EventFlag_ZeroDeltaOnSelection | UI_EventFlag_Delete;
                } break;
                case Command_SelectAll: {
                    UI_Event *move_start = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    move_start->kind = UI_EventKind_Navigation;
                    move_start->unit = UI_EventDeltaUnit_Whole;
                    move_start->delta.x = -1;
                    ui_event_list_push_event(&ui_events, move_start);

                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Navigation;
                    ui_event->flags = UI_EventFlag_KeepMark;
                    ui_event->unit = UI_EventDeltaUnit_Whole;
                    ui_event->delta.x = 1;
                } break;
                case Command_Copy: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Edit;
                    ui_event->flags = UI_EventFlag_Copy | UI_EventFlag_KeepMark;
                } break;
                case Command_Paste: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Text;
                    ui_event->text = gfx_get_clipboard_text(ui_frame_arena());
                } break;
                case Command_Cut: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Edit;
                    ui_event->flags = UI_EventFlag_Copy | UI_EventFlag_Delete;
                } break;
                case Command_ToggleListView: {
                    state->all_of_unicode = !state->all_of_unicode;
                } break;
                case Command_OpenCommandLister: {
                    state->show_command_lister = true;
                } break;
                case Command_OpenGlyphListView: {
                    push_command(Command_OpenTab, .tab_specification = str8_literal("GlyphList"));
                } break;
                case Command_OpenGlyphViewView: {
                    push_command(Command_OpenTab, .tab_specification = str8_literal("GlyphView"));
                } break;
                case Command_OpenRenderStatsView: {
                    push_command(Command_OpenTab, .tab_specification = str8_literal("RenderStats"));
                } break;
                case Command_OpenThemeView: {
                    push_command(Command_OpenTab, .tab_specification = str8_literal("Theme"));
                } break;
                case Command_OpenTestView: {
                    push_command(Command_OpenTab, .tab_specification = str8_literal("Test"));
                } break;
                case Command_Accept: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Accept;
                } break;
                case Command_Cancel: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Cancel;
                } break;
                case Command_UnloadFont: {
                    msdf_cache_clear();
                    arena_reset(state->ttf_arena);
                    state->ttf_font = &ttf_font_nil;
                } break;
                case Command_LoadFont: {
                    // NOTE(simon): Unload any previous font
                    msdf_cache_clear();
                    arena_reset(state->ttf_arena);
                    state->ttf_font = &ttf_font_nil;

                    state->ttf_font = ttf_load(state->ttf_arena, command_context->path);

                    for (Str8Node *error = state->ttf_font->errors.first; error; error = error->next) {
                        os_console_print(error->string);
                    }
                } break;
                case Command_COUNT: {
                } break;
            }

            if (ui_event && ui_event->kind != UI_EventKind_Null) {
                ui_event_list_push_event(&ui_events, ui_event);
            }
        }
        arena_reset(state->command_arena);
        state->commands.first = 0;
        state->commands.last  = 0;
    }

    // NOTE(simon): Build palettes
    for (PaletteCode code = 0; code < PaletteCode_COUNT; ++code) {
        state->palettes[code].cursor    = state->theme.cursor;
        state->palettes[code].selection = state->theme.selection;
    }
    state->palettes[PaletteCode_Base].background = state->theme.base_background;
    state->palettes[PaletteCode_Base].border     = state->theme.base_border;
    state->palettes[PaletteCode_Base].text       = state->theme.text;
    state->palettes[PaletteCode_Button].background = state->theme.button_background;
    state->palettes[PaletteCode_Button].border     = state->theme.button_border;
    state->palettes[PaletteCode_Button].text       = state->theme.text;
    state->palettes[PaletteCode_SecondaryButton].background = state->theme.secondary_button_background;
    state->palettes[PaletteCode_SecondaryButton].border     = state->theme.secondary_button_border;
    state->palettes[PaletteCode_SecondaryButton].text       = state->theme.text;
    state->palettes[PaletteCode_Tab].background = state->theme.tab_background;
    state->palettes[PaletteCode_Tab].border     = state->theme.tab_border;
    state->palettes[PaletteCode_Tab].text       = state->theme.text;
    state->palettes[PaletteCode_InactiveTab].background = state->theme.inactive_tab_background;
    state->palettes[PaletteCode_InactiveTab].border     = state->theme.inactive_tab_border;
    state->palettes[PaletteCode_InactiveTab].text       = state->theme.text;
    state->palettes[PaletteCode_DropSiteOverlay].background = state->theme.drop_site_overlay;
    state->palettes[PaletteCode_DropSiteOverlay].border     = state->theme.drop_site_overlay;
    state->palettes[PaletteCode_DropSiteOverlay].text       = state->theme.text;

    V2U32 client_area = gfx_get_window_client_area();
    render_begin(client_area);
    draw_begin_frame();
    Draw_List *draw_list = draw_list_create();
    draw_list_push(draw_list);

    // NOTE(simon): Build UI
    {
        prof_zone_begin(prof_ui_build, "ui build");
        ui_select_state(state->ui);
        ui_begin(&ui_events, 1.0f / 60.0f);

        ui_palette_push(palette_from_code(PaletteCode_Base));

        // NOTE(simon): state->font_size points * dpi pixels per inch / 72 points per inch
        ui_font_size_push((U32) (state->font_size * gfx_dpi() / 72.0f));

        R2F32 root_rectangle = r2f32(0.0f, 0.0f, (F32) client_area.x, (F32) client_area.y);

        typedef struct DragTabData DragTabData;
        struct DragTabData {
            Handle panel;
            Handle tab;
        };

        // NOTE(simon): Animate command lister.
        {
            if (f32_abs((F32) state->show_command_lister - state->command_lister_t) > 0.001f) {
                state->command_lister_t += ((F32) state->show_command_lister - state->command_lister_t) * ui_animation_fast_rate();
                request_frame();
            } else {
                state->command_lister_t = (F32) state->show_command_lister;
            }
        }

        // NOTE(simon): Build command lister.
        if (state->show_command_lister) {
            Arena_Temporary scratch = arena_get_scratch(0, 0);

            local U8 buffer[1024];
            local U64 buffer_size = 0;
            local U64 cursor = 0;
            local U64 mark = 0;
            local S64 active_index = 0;
            local UI_ScrollPosition position = { 0 };

            CommandItem commands[Command_COUNT] = { 0 };
            U64 command_count = 0;

            // NOTE(simon): Gather commands.
            {
                // NOTE(simon): Fill commands
                for (CommandKind command = 0; command < array_count(commands); ++command) {
                    if (command_show_in_ui[command]) {
                        commands[command_count].command       = command;
                        commands[command_count].name          = command_names[command];
                        commands[command_count].description   = command_descriptions[command];
                        commands[command_count].fuzzy_matches = str8_fuzzy_match(scratch.arena, str8(buffer, buffer_size), command_names[command]);
                        ++command_count;
                    }
                }

                // NOTE(simon): Filter on number of matched parts
                for (U64 i = 0; i < command_count;) {
                    FuzzyMatchList matches = commands[i].fuzzy_matches;

                    B32 remove = false;

                    // NOTE(simon): If there are search terms and no matches, remove the item.
                    remove |= matches.needle_parts && !matches.count;

                    // NOTE(simon): If the number of mathes doesn't match the
                    // number of search terms, remove the item.
                    remove |= matches.needle_parts != matches.count;

                    if (remove) {
                        swap(commands[i], commands[command_count - 1], CommandItem);
                        --command_count;
                    } else {
                        ++i;
                    }
                }

                // NOTE(simon): Sort by number of matches and name.
                quicksort(commands, command_count);
            }

            F32 command_rectangle_width  = (F32) client_area.width * 0.6f * state->command_lister_t;
            F32 command_rectangle_height = (F32) client_area.height * 0.8f * state->command_lister_t;

            ui_fixed_x_next(((F32) client_area.width  - command_rectangle_width)  / 2.0f);
            ui_fixed_y_next(((F32) client_area.height - command_rectangle_height) / 2.0f);
            ui_width_next(ui_size_pixels(command_rectangle_width, 1.0f));
            ui_height_next(ui_size_pixels(command_rectangle_height, 1.0f));
            ui_layout_axis_next(Axis2_Y);
            ui_focus_next(UI_Focus_Root);
            UI_Box *command_box = ui_create_box_from_string(
                UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawDropShadow |
                UI_BoxFlag_Clickable | UI_BoxFlag_Scrollable,
                str8_literal("##command_lister")
            );

            ui_parent(command_box)
            ui_width(ui_size_fill())
            ui_height(ui_size_ems(1.5f, 1.0f))
            ui_text_x_padding(ui_size_ems(0.5f, 1.0f).value) {
                UI_Key key = ui_key_from_string(ui_active_seed_key(), str8_literal("##query"));
                ui_palette(palette_from_code(PaletteCode_Button))
                ui_focus(UI_Focus_Active) {
                    ui_line_edit(buffer, &buffer_size, array_count(buffer), &cursor, &mark, key);
                }

                ui_spacer_sized(ui_size_ems(0.5f, 1.0f));

                // NOTE(simon): Scroll region
                // TODO(simon): Replace with fixed size.
                ui_height_next(ui_size_fill());
                ui_layout_axis_next(Axis2_X);
                UI_Box *region = ui_create_box_from_string(UI_BoxFlag_OverflowY | UI_BoxFlag_Clip | UI_BoxFlag_Scrollable, str8_literal("##region"));
                ui_parent_push(region);

                V2F32 region_size = region->calculated_size;

                F32 scrollbar_width = (F32) ui_font_size_top();
                F32 container_width = region_size.width - scrollbar_width;
                F32 container_height = region_size.height;

                F32 height = ui_size_ems(3.0, 1.0f).value;

                // NOTE(simon): Properties of the data begin viewed.
                S64 first_row    = 0;
                S64 last_row     = (S64) command_count;
                S64 visible_rows = (S64) f32_ceil(container_height / height);

                S64 previous_active_index = active_index;

                // NOTE(simon): Properties of the current view.
                S64 top_row    = position.index + (S64) f32_floor(position.offset);
                S64 bottom_row = s64_min(top_row + (position.offset != 0.0f) + visible_rows, last_row);

                // NOTE(simon): Scroll container
                ui_width_next(ui_size_pixels(container_width, 1.0f));
                ui_height_next(ui_size_pixels(container_height, 1.0f));
                ui_layout_axis_next(Axis2_Y);
                UI_Box *container = ui_create_box_from_string(0, str8_literal("##commands"));
                container->view_offset.y = height * (f32_mod(position.offset, 1.0f) + (position.offset < 0.0f));

                ui_parent(container)
                ui_width(ui_size_fill())
                ui_height(ui_size_pixels(height, 1.0f)) {
                    for (S64 i = top_row; i < bottom_row; ++i) {
                        ui_focus_push(i == active_index ? UI_Focus_Active : UI_Focus_Inactive);
                        ui_palette_push(palette_from_code(i % 2 == 0 ? PaletteCode_Button : PaletteCode_SecondaryButton));

                        ui_hover_cursor_next(Gfx_Cursor_Hand);
                        ui_layout_axis_next(Axis2_Y);
                        UI_Box *command_button_box = ui_create_box_from_string_format(
                            UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder |
                            UI_BoxFlag_DrawHot | UI_BoxFlag_DrawActive |
                            UI_BoxFlag_Clickable | UI_BoxFlag_KeyboardClickable,
                            "##command_%u", commands[i].command
                        );

                        ui_height(ui_size_text_content(0.0f, 1.0f))
                        ui_parent(command_button_box) {
                            UI_Box *name_box = ui_create_box_from_string(UI_BoxFlag_DrawText, commands[i].name);
                            ui_box_set_fuzzy_match_list(name_box, commands[i].fuzzy_matches);

                            ui_font_size_next(((U32) (0.9f * (F32) ui_font_size_top())));
                            UI_Palette palette = ui_palette_top();
                            palette.text = color_from_theme(ThemeColor_WeakText);
                            ui_palette_next(palette);
                            ui_label(commands[i].description);
                        }
                        UI_Input command_button_input = ui_input_from_box(command_button_box);

                        if (command_button_input.input_flags & UI_InputFlag_Clicked) {
                            push_command(commands[i].command);
                            state->show_command_lister = 0;
                            buffer_size = 0;
                            cursor = 0;
                            mark = 0;
                            active_index = 0;
                            memory_zero_struct(&position);
                        }

                        ui_palette_pop();
                        ui_focus_pop();
                    }

                    for (UI_Event *event = 0; ui_next_event(&event);) {
                        if (event->kind != UI_EventKind_Navigation) {
                            continue;
                        }

                        S64 delta = 0;
                        switch (event->unit) {
                            case UI_EventDeltaUnit_Null: {
                            } break;
                            case UI_EventDeltaUnit_Character: {
                                delta += event->delta.y;
                            } break;
                            case UI_EventDeltaUnit_Word: {
                            } break;
                            case UI_EventDeltaUnit_Line: {
                            } break;
                            case UI_EventDeltaUnit_Page: {
                                delta += event->delta.y * visible_rows;
                            } break;
                            case UI_EventDeltaUnit_Whole: {
                            } break;
                            case UI_EventDeltaUnit_COUNT: {
                            } break;
                        }

                        active_index = s64_min(s64_max(0, active_index + delta), (S64) command_count - 1);

                        ui_consume_event(event);
                    }

                    active_index = s64_min(s64_max(0, active_index), (S64) command_count - 1);
                }

                ui_palette(palette_from_code(PaletteCode_Button))
                ui_width(ui_size_pixels(scrollbar_width, 1.0f))
                ui_height(ui_size_pixels(region_size.height, 1.0f)) {
                    position = ui_scroll_bar(position, 0, last_row, visible_rows);
                }

                // NOTE(simon): Region
                ui_parent_pop();

                // NOTE(simon): Scrolling
                UI_Input region_input = ui_input_from_box(region);
                S64 scroll_delta = (S64) f32_round(region_input.scroll.y);
                position.index  -= scroll_delta;
                position.offset += (F32) scroll_delta;

                // NOTE(simon): Recenter if the active index is out of view.
                if (previous_active_index != active_index) {
                    if (!(top_row <= active_index && active_index < bottom_row)) {
                        S64 target_row = active_index - visible_rows / 2;
                        S64 delta = target_row - position.index;
                        position.index  += delta;
                        position.offset -= (F32) delta;
                    }
                }

                // NOTE(simon): Clamp scrolling.
                if (position.index < 0) {
                    position.offset += (F32) position.index;
                    position.index = 0;
                } else if (last_row <= position.index) {
                    position.offset -= (F32) (s64_max(0, last_row - 1) - position.index);
                    position.index = s64_max(0, last_row - 1);
                }

                // NOTE(simon): Animation
                position.offset += -position.offset * ui_animation_slow_rate();
                if (f32_abs(position.offset) < 0.001f) {
                    position.offset = 0.0f;
                } else {
                    request_frame();
                }
            }

            ui_input_from_box(command_box);

            if (ui_consume_event_kind(UI_EventKind_Cancel)) {
                state->show_command_lister = 0;
                buffer_size = 0;
                cursor = 0;
                mark = 0;
                active_index = 0;
                memory_zero_struct(&position);
            }

            // NOTE(simon): Close lister if you click outside of the dialog.
            for (UI_Event *event = 0; ui_next_event(&event);) {
                if (
                    event->kind == UI_EventKind_KeyPress && (
                        event->key == Gfx_Key_MouseLeft ||
                        event->key == Gfx_Key_MouseMiddle ||
                        event->key == Gfx_Key_MouseRight
                    )
                ) {
                    state->show_command_lister = 0;
                    buffer_size = 0;
                    cursor = 0;
                    mark = 0;
                    active_index = 0;
                    memory_zero_struct(&position);
                }
            }

            arena_end_temporary(scratch);
        }

        // NOTE(simon): Only build preview if we are actually dragging the
        // view. Otherwise, the tooltip will be clipped to the current window.
        if (state->drag_state == DragState_Dragging) {
            DragTabData *data = ui_get_drag_data(DragTabData);
            Tab *tab = tab_from_handle(data->tab);
            if (tab && tab->build_view) {
                ui_tooltip() {
                    ui_width_next(ui_size_ems(60.0f, 1.0f));
                    ui_height_next(ui_size_ems(40.0f, 1.0f));
                    ui_corner_radius_next(10.0f);
                    ui_layout_axis_next(Axis2_Y);
                    UI_Box *preview_box = ui_create_box(UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawDropShadow);
                    ui_parent(preview_box) {
                        ui_corner_radius_00_next(10.0f);
                        ui_corner_radius_01_next(10.0f);
                        ui_text_align_next(UI_TextAlign_Left);
                        ui_width_next(ui_size_text_content(5.0f, 1.0f));
                        ui_height_next(ui_size_text_content(0.0f, 1.0f));
                        ui_palette_next(palette_from_code(PaletteCode_Tab));
                        UI_Box *tab_box = ui_create_box_from_string(UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawText, tab->name);

                        ui_spacer_sized(ui_size_pixels(10.0f, 1.0f));

                        ui_width_next(ui_size_fill());
                        ui_height_next(ui_size_fill());
                        UI_Box *content_box = ui_create_box_from_string(UI_BoxFlag_Clip, str8_literal("###drag_preview"));

                        ui_parent(content_box) {
                            tab->build_view(tab, content_box->calculated_rectangle);
                        }

                        ui_spacer_sized(ui_size_pixels(10.0f, 1.0f));
                    }
                }
            } else {
                drag_cancel();
            }
        }

        F32 panel_pad = 2.0f;

        // NOTE(simon): Build non-leaf panel UI.
        prof_zone_begin(prof_bulid_non_leaf_ui, "non-leaf ui");
        for (Panel *panel = state->panel_root; panel; panel = panel_iterator_depth_first_pre_order(panel, 0).next) {
            if (!panel->first) {
                continue;
            }

            R2F32 panel_rectangle = rectangle_from_panel(panel, root_rectangle);
            V2F32 panel_rectangle_size = r2f32_size(panel_rectangle);

            if (drag_is_active()) {
                F32 drop_major_half_size = 5.0f * (F32) ui_font_size_top();
                F32 drop_minor_half_size = 3.0f * (F32) ui_font_size_top();
                F32 corner_radius = 0.5f * (F32) ui_font_size_top();
                F32 padding = 0.5f * (F32) ui_font_size_top();

                Axis2 split_axis = panel->split_axis;
                if (panel == state->panel_root) {
                    ui_corner_radius(corner_radius)
                    for (Side side = 0; side < Side_COUNT; ++side) {
                        V2F32 panel_center = r2f32_center(panel_rectangle);

                        UI_Key key = ui_key_from_string_format(global_ui_null_key, "root_extra_split_%i", side);
                        R2F32 drop_rectangle = { 0 };
                        drop_rectangle.min.values[axis2_flip(split_axis)] = panel_rectangle.values[side].values[axis2_flip(split_axis)] - drop_minor_half_size;
                        drop_rectangle.max.values[axis2_flip(split_axis)] = panel_rectangle.values[side].values[axis2_flip(split_axis)] + drop_minor_half_size;
                        drop_rectangle.min.values[split_axis] = panel_center.values[split_axis] - drop_major_half_size;
                        drop_rectangle.max.values[split_axis] = panel_center.values[split_axis] + drop_major_half_size;

                        ui_fixed_position_next(drop_rectangle.min);
                        ui_width_next(ui_size_pixels(r2f32_size(drop_rectangle).width, 1.0f));
                        ui_height_next(ui_size_pixels(r2f32_size(drop_rectangle).height, 1.0f));

                        ui_layout_axis_next(Axis2_Y);
                        UI_Box *drop_site = ui_create_box_from_key(UI_BoxFlag_FloatingPosition | UI_BoxFlag_DropTarget, key);
                        ui_input_from_box(drop_site);

                        ui_parent(drop_site)
                        ui_width(ui_size_fill())
                        ui_height(ui_size_fill())
                        ui_palette(palette_from_code(PaletteCode_Base))
                        ui_padding(ui_size_pixels(padding, 1.0f))
                        ui_row()
                        ui_padding(ui_size_pixels(padding, 1.0f)) {
                            ui_layout_axis_next(split_axis);

                            if (ui_keys_match(key, ui_drop_hot_key())) {
                                UI_Palette overlay = ui_palette_top();
                                overlay.border = color_from_theme(ThemeColor_Hover);
                                ui_palette_next(overlay);
                            }
                            UI_Box *visualization = ui_create_box(UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawDropShadow);
                            ui_parent(visualization)
                            ui_padding(ui_size_pixels(padding, 1.0f))
                            ui_palette(palette_from_code(PaletteCode_Button)) {
                                ui_layout_axis_next(axis2_flip(split_axis));
                                UI_Box *row_or_column = ui_create_box(0);
                                ui_parent(row_or_column)
                                ui_padding(ui_size_pixels(padding, 1.0f)) {
                                    ui_create_box(UI_BoxFlag_DrawBorder);
                                    ui_spacer_sized(ui_size_pixels(padding, 1.0f));
                                    ui_create_box(UI_BoxFlag_DrawBorder);
                                }
                            }
                        }

                        if (ui_keys_match(key, ui_drop_hot_key())) {
                            R2F32 future_split_rectangle = drop_rectangle;
                            future_split_rectangle.min.values[axis2_flip(split_axis)] -= drop_major_half_size;
                            future_split_rectangle.max.values[axis2_flip(split_axis)] += drop_major_half_size;
                            future_split_rectangle.min.values[split_axis] = panel_rectangle.min.values[split_axis];
                            future_split_rectangle.max.values[split_axis] = panel_rectangle.max.values[split_axis];

                            ui_palette_next(palette_from_code(PaletteCode_DropSiteOverlay));
                            ui_fixed_position_next(future_split_rectangle.min);
                            ui_width_next(ui_size_pixels(r2f32_size(future_split_rectangle).width, 1.0f));
                            ui_height_next(ui_size_pixels(r2f32_size(future_split_rectangle).height, 1.0f));
                            ui_create_box(UI_BoxFlag_FloatingPosition | UI_BoxFlag_DrawBackground);
                        }

                        if (ui_keys_match(key, ui_drop_hot_key()) && drag_drop()) {
                            Direction2 direction = axis2_flip(split_axis) == Axis2_X ? Direction2_Left : Direction2_Up;
                            if (side == Side_Max) {
                                direction = axis2_flip(split_axis) == Axis2_X ? Direction2_Right : Direction2_Down;
                            }

                            DragTabData *data = ui_get_drag_data(DragTabData);
                            push_command(
                                Command_SplitPanel,
                                .panel = data->panel,
                                .tab   = data->tab,
                                .destination_panel = handle_from_panel(panel),
                                .direction = direction,
                            );
                        }
                    }
                }

                ui_corner_radius(corner_radius)
                for (Panel *child = panel->first;; child = child->next) {
                    R2F32 child_rectangle = rectangle_from_child_panel_parent_rectangle(panel, child, panel_rectangle);
                    V2F32 child_center = r2f32_center(child_rectangle);

                    UI_Key key = ui_key_from_string_format(global_ui_null_key, "drop_boundary_%p_%p", panel, child);
                    R2F32 drop_rectangle = { 0 };
                    drop_rectangle.min.values[split_axis] = child_rectangle.min.values[split_axis] - drop_minor_half_size;
                    drop_rectangle.max.values[split_axis] = child_rectangle.min.values[split_axis] + drop_minor_half_size;
                    drop_rectangle.min.values[axis2_flip(split_axis)] = child_center.values[axis2_flip(split_axis)] - drop_major_half_size;
                    drop_rectangle.max.values[axis2_flip(split_axis)] = child_center.values[axis2_flip(split_axis)] + drop_major_half_size;

                    ui_fixed_position_next(drop_rectangle.min);
                    ui_width_next(ui_size_pixels(r2f32_size(drop_rectangle).width, 1.0f));
                    ui_height_next(ui_size_pixels(r2f32_size(drop_rectangle).height, 1.0f));

                    ui_layout_axis_next(Axis2_Y);
                    UI_Box *drop_site = ui_create_box_from_key(UI_BoxFlag_FloatingPosition | UI_BoxFlag_DropTarget, key);
                    ui_input_from_box(drop_site);

                    ui_parent(drop_site)
                    ui_width(ui_size_fill())
                    ui_height(ui_size_fill())
                    ui_palette(palette_from_code(PaletteCode_Base))
                    ui_padding(ui_size_pixels(padding, 1.0f))
                    ui_row()
                    ui_padding(ui_size_pixels(padding, 1.0f)) {
                        ui_layout_axis_next(axis2_flip(split_axis));
                        if (ui_keys_match(key, ui_drop_hot_key())) {
                            UI_Palette overlay = ui_palette_top();
                            overlay.border = color_from_theme(ThemeColor_Hover);
                            ui_palette_next(overlay);
                        }
                        UI_Box *visualization = ui_create_box(UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawDropShadow);
                        ui_parent(visualization)
                        ui_padding(ui_size_pixels(padding, 1.0f))
                        ui_palette(palette_from_code(PaletteCode_Button)) {
                            ui_layout_axis_next(split_axis);
                            UI_Box *row_or_column = ui_create_box(0);
                            ui_parent(row_or_column)
                            ui_padding(ui_size_pixels(padding, 1.0f)) {
                                ui_create_box(UI_BoxFlag_DrawBorder);
                                ui_spacer_sized(ui_size_pixels(padding, 1.0f));
                                ui_create_box(UI_BoxFlag_DrawBorder);
                            }
                        }
                    }

                    if (ui_keys_match(key, ui_drop_hot_key())) {
                        R2F32 future_split_rectangle = drop_rectangle;
                        future_split_rectangle.min.values[split_axis] -= drop_major_half_size;
                        future_split_rectangle.max.values[split_axis] += drop_major_half_size;
                        future_split_rectangle.min.values[axis2_flip(split_axis)] = child_rectangle.min.values[axis2_flip(split_axis)];
                        future_split_rectangle.max.values[axis2_flip(split_axis)] = child_rectangle.max.values[axis2_flip(split_axis)];

                        ui_palette_next(palette_from_code(PaletteCode_DropSiteOverlay));
                        ui_fixed_position_next(future_split_rectangle.min);
                        ui_width_next(ui_size_pixels(r2f32_size(future_split_rectangle).width, 1.0f));
                        ui_height_next(ui_size_pixels(r2f32_size(future_split_rectangle).height, 1.0f));
                        ui_create_box(UI_BoxFlag_FloatingPosition | UI_BoxFlag_DrawBackground);
                    }

                    if (ui_keys_match(key, ui_drop_hot_key()) && drag_drop()) {
                        Direction2 direction = split_axis == Axis2_X ? Direction2_Left : Direction2_Up;
                        Panel *split_panel = child;
                        if (!split_panel) {
                            split_panel = panel->last;
                            direction = split_axis == Axis2_X ? Direction2_Right : Direction2_Down;
                        }

                        DragTabData *data = ui_get_drag_data(DragTabData);
                        push_command(
                            Command_SplitPanel,
                            .panel = data->panel,
                            .tab   = data->tab,
                            .destination_panel = handle_from_panel(split_panel),
                            .direction = direction,
                        );
                    }

                    if (!child) {
                        break;
                    }
                }
            }

            for (Panel *child = panel->first; child && child->next; child = child->next) {
                R2F32 child_rectangle = rectangle_from_child_panel_parent_rectangle(panel, child, panel_rectangle);
                R2F32 boundary_rectangle = child_rectangle;
                boundary_rectangle.min.values[panel->split_axis] = boundary_rectangle.max.values[panel->split_axis];
                boundary_rectangle.min.values[panel->split_axis] -= panel_pad;
                boundary_rectangle.max.values[panel->split_axis] += panel_pad;

                ui_fixed_position_next(boundary_rectangle.min);
                ui_width_next(ui_size_pixels(r2f32_size(boundary_rectangle).width, 1.0f));
                ui_height_next(ui_size_pixels(r2f32_size(boundary_rectangle).height, 1.0f));
                ui_hover_cursor_next(panel->split_axis == Axis2_X ? Gfx_Cursor_SizeWE : Gfx_Cursor_SizeNS);
                UI_Box *boundary_box = ui_create_box_from_string_format(UI_BoxFlag_Clickable | UI_BoxFlag_FloatingPosition, "###panel_boundary_%p", child);
                UI_Input input = ui_input_from_box(boundary_box);

                if (input.input_flags & UI_InputFlag_LeftDragging) {
                    Panel *min_child = child;
                    Panel *max_child = child->next;

                    if (input.input_flags & UI_InputFlag_LeftPressed) {
                        V2F32 drag_data = v2f32(min_child->percentage_of_parent, max_child->percentage_of_parent);
                        ui_set_drag_data(&drag_data);
                    }

                    V2F32 drag_data = *ui_get_drag_data(V2F32);

                    F32 min_child_percentage_pre_drag = drag_data.x;
                    F32 max_child_percentage_pre_drag = drag_data.y;
                    F32 min_child_pixels_pre_drag = min_child_percentage_pre_drag * panel_rectangle_size.values[panel->split_axis];
                    F32 max_child_pixels_pre_drag = max_child_percentage_pre_drag * panel_rectangle_size.values[panel->split_axis];

                    V2F32 both_drag_delta = ui_drag_delta();
                    F32 drag_delta = both_drag_delta.values[panel->split_axis];
                    F32 clamped_drag_delta = drag_delta;
                    if (drag_delta < 0.0f) {
                        clamped_drag_delta = -f32_min(-drag_delta, min_child_pixels_pre_drag - 2.0f * panel_pad);
                    } else {
                        clamped_drag_delta = f32_min(drag_delta, max_child_pixels_pre_drag - 2.0f * panel_pad);
                    }

                    F32 min_child_pixels_post_drag = min_child_pixels_pre_drag + clamped_drag_delta;
                    F32 max_child_pixels_post_drag = max_child_pixels_pre_drag - clamped_drag_delta;
                    F32 min_child_percentage_post_drag = min_child_pixels_post_drag / panel_rectangle_size.values[panel->split_axis];
                    F32 max_child_percentage_post_drag = max_child_pixels_post_drag / panel_rectangle_size.values[panel->split_axis];
                    min_child->percentage_of_parent = min_child_percentage_post_drag;
                    max_child->percentage_of_parent = max_child_percentage_post_drag;
                }
            }
        }
        prof_zone_end(prof_bulid_non_leaf_ui);

        // NOTE(simon): Animate panels.
        for (Panel *panel = state->panel_root; panel; panel = panel_iterator_depth_first_pre_order(panel, 0).next) {
            R2F32 target = rectangle_from_panel(panel, root_rectangle);
            R2F32 target_percentage = r2f32(
                target.min.x / (F32) client_area.x,
                target.min.y / (F32) client_area.y,
                target.max.x / (F32) client_area.x,
                target.max.y / (F32) client_area.y
            );

            B32 is_animating = false;
            is_animating |= f32_abs(target.min.x - panel->animated_rectangle_percentage.min.x * (F32) client_area.x) > 0.5f;
            is_animating |= f32_abs(target.min.y - panel->animated_rectangle_percentage.min.y * (F32) client_area.y) > 0.5f;
            is_animating |= f32_abs(target.max.x - panel->animated_rectangle_percentage.max.x * (F32) client_area.x) > 0.5f;
            is_animating |= f32_abs(target.max.y - panel->animated_rectangle_percentage.max.y * (F32) client_area.y) > 0.5f;

            if (is_animating) {
                panel->animated_rectangle_percentage.min.x += (target_percentage.min.x - panel->animated_rectangle_percentage.min.x) * ui_animation_fast_rate();
                panel->animated_rectangle_percentage.min.y += (target_percentage.min.y - panel->animated_rectangle_percentage.min.y) * ui_animation_fast_rate();
                panel->animated_rectangle_percentage.max.x += (target_percentage.max.x - panel->animated_rectangle_percentage.max.x) * ui_animation_fast_rate();
                panel->animated_rectangle_percentage.max.y += (target_percentage.max.y - panel->animated_rectangle_percentage.max.y) * ui_animation_fast_rate();
                request_frame();
            } else {
                panel->animated_rectangle_percentage = target_percentage;
            }
        }

        // NOTE(simon): Build leaf panel UI.
        prof_zone_begin(prof_bulid_leaf_ui, "leaf ui");
        ui_layout_axis(Axis2_Y)
        for (Panel *panel = state->panel_root; panel; panel = panel_iterator_depth_first_pre_order(panel, 0).next) {
            if (panel->first) {
                continue;
            }

            push_context(.panel = handle_from_panel(panel), .tab = panel->active_tab);

            ui_focus(panel == panel_from_handle(state->active_panel) && !state->show_command_lister ? UI_Focus_None : UI_Focus_Inactive) {
                R2F32 panel_rectangle = r2f32_pad(
                    r2f32(
                        panel->animated_rectangle_percentage.min.x * (F32) client_area.x,
                        panel->animated_rectangle_percentage.min.y * (F32) client_area.y,
                        panel->animated_rectangle_percentage.max.x * (F32) client_area.x,
                        panel->animated_rectangle_percentage.max.y * (F32) client_area.y
                    ),
                    -panel_pad
                );

                if (drag_is_active() && r2f32_contains_v2f32(panel_rectangle, ui_mouse())) {
                    V2F32 center = r2f32_center(panel_rectangle);
                    F32 drop_half_size = 3.0f * (F32) ui_font_size_top();
                    F32 corner_radius = 0.5f * (F32) ui_font_size_top();
                    F32 padding = 0.5f * (F32) ui_font_size_top();
                    typedef struct DropTarget DropTarget;
                    struct DropTarget {
                        UI_Key key;
                        Direction2 direction;
                        R2F32 rectangle;
                    };
                    DropTarget targets[] = {
                        {
                            ui_key_from_string_format(global_ui_null_key, "drop_center_%p", panel),
                            Direction2_Invalid,
                            r2f32(
                                center.x - drop_half_size, center.y - drop_half_size,
                                center.x + drop_half_size, center.y + drop_half_size
                            ),
                        },
                        {
                            ui_key_from_string_format(global_ui_null_key, "drop_left_%p", panel),
                            Direction2_Left,
                            r2f32(
                                center.x - drop_half_size - 2.0f * drop_half_size, center.y - drop_half_size,
                                center.x + drop_half_size - 2.0f * drop_half_size, center.y + drop_half_size
                            ),
                        },
                        {
                            ui_key_from_string_format(global_ui_null_key, "drop_up_%p", panel),
                            Direction2_Up,
                            r2f32(
                                center.x - drop_half_size, center.y - drop_half_size - 2.0f * drop_half_size,
                                center.x + drop_half_size, center.y + drop_half_size - 2.0f * drop_half_size
                            ),
                        },
                        {
                            ui_key_from_string_format(global_ui_null_key, "drop_right_%p", panel),
                            Direction2_Right,
                            r2f32(
                                center.x - drop_half_size + 2.0f * drop_half_size, center.y - drop_half_size,
                                center.x + drop_half_size + 2.0f * drop_half_size, center.y + drop_half_size
                            ),
                        },
                        {
                            ui_key_from_string_format(global_ui_null_key, "drop_down_%p", panel),
                            Direction2_Down,
                            r2f32(
                                center.x - drop_half_size, center.y - drop_half_size + 2.0f * drop_half_size,
                                center.x + drop_half_size, center.y + drop_half_size + 2.0f * drop_half_size
                            ),
                        },
                    };

                    ui_corner_radius(corner_radius)
                    for (U32 i = 0; i < array_count(targets); ++i) {
                        Axis2 axis = axis2_from_direction2(targets[i].direction);
                        Side side = side_from_direction2(targets[i].direction);
                        if (targets[i].direction != Direction2_Invalid && panel->parent && axis == panel->parent->split_axis) {
                            continue;
                        }

                        ui_fixed_position_next(targets[i].rectangle.min);
                        ui_width_next(ui_size_pixels(r2f32_size(targets[i].rectangle).width, 1.0f));
                        ui_height_next(ui_size_pixels(r2f32_size(targets[i].rectangle).height, 1.0f));

                        UI_Box *drop_site = ui_create_box_from_key(UI_BoxFlag_FloatingPosition | UI_BoxFlag_DropTarget, targets[i].key);
                        ui_input_from_box(drop_site);

                        ui_parent(drop_site)
                        ui_width(ui_size_fill())
                        ui_height(ui_size_fill())
                        ui_palette(palette_from_code(PaletteCode_Base))
                        ui_padding(ui_size_pixels(padding, 1.0f))
                        ui_row()
                        ui_padding(ui_size_pixels(padding, 1.0f)) {
                            if (ui_keys_match(targets[i].key, ui_drop_hot_key())) {
                                UI_Palette overlay = ui_palette_top();
                                overlay.border = color_from_theme(ThemeColor_Hover);
                                ui_palette_next(overlay);
                            }
                            ui_layout_axis_next(axis2_flip(axis));
                            UI_Box *visualization = ui_create_box(UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawDropShadow);
                            ui_parent(visualization)
                            ui_width(ui_size_fill())
                            ui_height(ui_size_fill())
                            ui_padding(ui_size_pixels(padding, 1.0f))
                            ui_palette(palette_from_code(PaletteCode_Button)) {
                                if (targets[i].direction != Direction2_Invalid) {
                                    ui_layout_axis_next(axis);
                                    UI_Box *row_or_column = ui_create_box(0);
                                    ui_parent(row_or_column)
                                    ui_padding(ui_size_pixels(padding, 1.0f)) {
                                        ui_create_box(side == Side_Min ? UI_BoxFlag_DrawBackground : UI_BoxFlag_DrawBorder);
                                        ui_spacer_sized(ui_size_pixels(padding, 1.0f));
                                        ui_create_box(side == Side_Max ? UI_BoxFlag_DrawBackground : UI_BoxFlag_DrawBorder);
                                    }
                                } else {
                                    ui_layout_axis_next(axis);
                                    UI_Box *row_or_column = ui_create_box(0);
                                    ui_parent(row_or_column)
                                    ui_padding(ui_size_pixels(padding, 1.0f)) {
                                        ui_create_box(UI_BoxFlag_DrawBackground);
                                    }
                                }
                            }
                        }

                        if (ui_keys_match(targets[i].key, ui_drop_hot_key()) && drag_drop()) {
                            if (targets[i].direction == Direction2_Invalid) {
                                DragTabData *data = ui_get_drag_data(DragTabData);
                                push_command(
                                    Command_MoveTab,
                                    .panel = data->panel,
                                    .tab   = data->tab,
                                    .destination_panel = handle_from_panel(panel),
                                    .previous_tab      = panel->active_tab,
                                );
                            } else {
                                DragTabData *data = ui_get_drag_data(DragTabData);
                                push_command(
                                    Command_SplitPanel,
                                    .panel = data->panel,
                                    .tab   = data->tab,
                                    .destination_panel = handle_from_panel(panel),
                                    .direction = targets[i].direction,
                                );
                            }
                        }
                    }

                    for (U32 i = 0; i < array_count(targets); ++i) {
                        if (ui_keys_match(targets[i].key, ui_drop_hot_key())) {
                            Axis2 split_axis = axis2_from_direction2(targets[i].direction);
                            Side split_side = side_from_direction2(targets[i].direction);
                            R2F32 future_split_rectangle = panel_rectangle;
                            if (targets[i].direction != Direction2_Invalid) {
                                V2F32 panel_center = r2f32_center(panel_rectangle);
                                future_split_rectangle.values[side_flip(split_side)].values[split_axis] = panel_center.values[split_axis];
                            }

                            ui_palette_next(palette_from_code(PaletteCode_DropSiteOverlay));
                            ui_fixed_position_next(future_split_rectangle.min);
                            ui_width_next(ui_size_pixels(r2f32_size(future_split_rectangle).width, 1.0f));
                            ui_height_next(ui_size_pixels(r2f32_size(future_split_rectangle).height, 1.0f));
                            ui_create_box(UI_BoxFlag_DrawBackground);
                        }
                    }
                }

                for (UI_Event *event = 0; ui_next_event(&event);) {
                    if (
                        event->kind == UI_EventKind_KeyPress && (
                            event->key == Gfx_Key_MouseLeft ||
                            event->key == Gfx_Key_MouseMiddle ||
                            event->key == Gfx_Key_MouseRight
                        ) &&
                        r2f32_contains_v2f32(panel_rectangle, event->position)
                    ) {
                        push_command(Command_FocusPanel);
                        break;
                    }
                }

                Tab *next_active_tab = tab_from_handle(panel->active_tab);

                UI_Size tab_height = ui_size_ems(2.0f, 1.0f);
                R2F32 tab_bar_rectangle = r2f32(panel_rectangle.min.x, panel_rectangle.min.y, panel_rectangle.max.x, panel_rectangle.min.y + tab_height.value);
                R2F32 content_rectangle = r2f32(panel_rectangle.min.x, panel_rectangle.min.y + tab_height.value, panel_rectangle.max.x, panel_rectangle.max.y);

                if (panel != panel_from_handle(state->active_panel) || state->show_command_lister) {
                    UI_Palette overlay = ui_palette_top();
                    overlay.background = color_from_theme(ThemeColor_InactivePanelOverlay);
                    ui_palette_next(overlay);
                    ui_fixed_position_next(panel_rectangle.min);
                    ui_width_next(ui_size_pixels(r2f32_size(panel_rectangle).width, 1.0f));
                    ui_height_next(ui_size_pixels(r2f32_size(panel_rectangle).height, 1.0f));
                    ui_create_box(UI_BoxFlag_DrawBackground | UI_BoxFlag_FloatingPosition);
                }

                ui_fixed_position_next(tab_bar_rectangle.min);
                ui_width_next(ui_size_pixels(r2f32_size(tab_bar_rectangle).width, 1.0f));
                ui_height_next(ui_size_pixels(r2f32_size(tab_bar_rectangle).height, 1.0f));
                ui_layout_axis_next(Axis2_X);
                UI_Box *tab_bar_box = ui_create_box_from_string_format(
                    UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder | UI_BoxFlag_Clickable | UI_BoxFlag_FloatingPosition | UI_BoxFlag_OverflowX | UI_BoxFlag_Clip,
                    "###tab_bar_box_%p", panel
                );

                ui_palette(palette_from_code(PaletteCode_InactiveTab))
                ui_width(ui_size_children_sum(1.0f))
                ui_height(tab_height)
                ui_layout_axis(Axis2_X)
                ui_parent(tab_bar_box)
                ui_corner_radius_00(ui_size_ems(0.75f, 1.0f).value)
                ui_corner_radius_01(ui_size_ems(0.75f, 1.0f).value) {
                    for (Tab *tab = panel->tab_first; tab; tab = tab->next) {
                        push_context(.tab = handle_from_tab(tab));
                        if (tab == tab_from_handle(panel->active_tab)) {
                            ui_palette_push(palette_from_code(PaletteCode_Tab));
                        }

                        ui_hover_cursor_next(Gfx_Cursor_Hand);
                        UI_Box *tab_box = ui_create_box_from_string_format(
                            UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder | UI_BoxFlag_Clickable | UI_BoxFlag_DrawHot | UI_BoxFlag_DrawActive | UI_BoxFlag_AnimateX,
                            "###tab_%p", tab
                        );

                        ui_parent(tab_box) {
                            ui_text_align_next(UI_TextAlign_Left);
                            ui_width_next(ui_size_text_content(5.0f, 1.0f));
                            ui_label(tab->name);

                            ui_width_next(ui_size_ems(1.5f, 1.0f));
                            ui_text_align_next(UI_TextAlign_Center);
                            ui_hover_cursor_next(Gfx_Cursor_Hand);
                            UI_Box *close_box = ui_create_box_from_string_format(
                                UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawText |
                                UI_BoxFlag_DrawHot | UI_BoxFlag_DrawActive |
                                UI_BoxFlag_Clickable,
                                "X##_tab_%p", tab
                            );
                            UI_Input close_input = ui_input_from_box(close_box);
                            if (close_input.input_flags & UI_InputFlag_LeftClicked) {
                                push_command(Command_CloseTab, .tab = handle_from_tab(tab));
                            }
                        }

                        UI_Input input = ui_input_from_box(tab_box);

                        if (input.input_flags & UI_InputFlag_LeftPressed) {
                            next_active_tab = tab;
                        }

                        if (tab->next) {
                            ui_spacer_sized(ui_size_ems(0.3f, 1.0f));
                        }

                        if (input.input_flags & UI_InputFlag_LeftDragging && !drag_is_active() && v2f32_length(ui_drag_delta()) > 10.0f) {
                            DragTabData data = {
                                .panel = handle_from_panel(panel),
                                .tab = handle_from_tab(tab),
                            };
                            ui_set_drag_data(&data);
                            drag_begin();
                        }

                        if (tab == tab_from_handle(panel->active_tab)) {
                            ui_palette_pop();
                        }

                        pop_context();
                    }
                }

                if (panel == panel_from_handle(state->active_panel)) {
                    UI_Palette overlay = ui_palette_top();
                    overlay.border = color_from_theme(ThemeColor_Focus);
                    ui_palette_next(overlay);
                }
                ui_fixed_position_next(content_rectangle.min);
                ui_width_next(ui_size_pixels(r2f32_size(content_rectangle).width, 1.0f));
                ui_height_next(ui_size_pixels(r2f32_size(content_rectangle).height, 1.0f));
                UI_Box *content_box = ui_create_box_from_string_format(
                    UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder | UI_BoxFlag_Clickable | UI_BoxFlag_DropTarget | UI_BoxFlag_FloatingPosition | UI_BoxFlag_Clip,
                    "###panel_box_%p", panel
                );

                ui_parent(content_box) {
                    Tab *tab = tab_from_handle(panel->active_tab);
                    if (tab && tab->build_view) {
                        tab->build_view(tab, content_rectangle);
                    } else {
                        ui_width(ui_size_parent_percent(1.0f, 1.0f))
                        ui_height(ui_size_parent_percent(1.0f, 1.0f))
                        ui_column() {
                            ui_spacer_sized(ui_size_fill());
                            ui_height(ui_size_children_sum(1.0f))
                            ui_row() {
                                ui_spacer_sized(ui_size_fill());
                                ui_width_next(ui_size_text_content(5.0f, 1.0f));
                                ui_height_next(ui_size_text_content(0.0f, 1.0f));
                                ui_palette_next(palette_from_code(PaletteCode_Button));
                                ui_corner_radius_next(5.0f);
                                UI_Input close_input = ui_button_format("Close panel###%p", panel);
                                if (close_input.input_flags & UI_InputFlag_LeftClicked) {
                                    push_command(Command_ClosePanel);
                                }

                                ui_spacer_sized(ui_size_fill());
                            }
                            ui_spacer_sized(ui_size_fill());
                        }
                    }
                }

                // NOTE(simon): Consume fallthrough events.
                ui_input_from_box(content_box);
                if (ui_drop_hot_key() == content_box->key && drag_drop()) {
                    DragTabData *data = ui_get_drag_data(DragTabData);
                    push_command(
                        Command_MoveTab,
                        .panel = data->panel,
                        .tab   = data->tab,
                        .destination_panel = handle_from_panel(panel),
                        .previous_tab      = panel->active_tab,
                    );
                }

                panel->active_tab = handle_from_tab(next_active_tab);
            }

            pop_context();
        }
        prof_zone_end(prof_bulid_leaf_ui);

        ui_font_size_pop();
        ui_palette_pop();
        ui_end();
        prof_zone_end(prof_ui_build);
    }

    // NOTE(simon): Draw
    {
        prof_zone_begin(prof_draw_ui, "draw");

        draw_rectangle(r2f32(0, 0, (F32) client_area.width, (F32) client_area.height), color_from_theme(ThemeColor_BaseBackground), 0, 0, 0);

        for (UI_Box *box = state->ui->root; box != &global_ui_null_box;) {
            if (box->flags & UI_BoxFlag_DrawDropShadow) {
                draw_rectangle(
                    r2f32(
                        box->calculated_rectangle.min.x - 4.0f,
                        box->calculated_rectangle.min.y - 4.0f,
                        box->calculated_rectangle.max.x + 12.0f,
                        box->calculated_rectangle.max.y + 12.0f
                    ),
                    color_from_theme(ThemeColor_DropShadow),
                    0.8f, 0.0f, 8.0f
                );
            }

            if (box->flags & UI_BoxFlag_DrawBackground) {
                {
                    Render_Shape *shape = draw_rectangle(box->calculated_rectangle, box->palette.background, 0.0f, 0.0f, 1.0f);
                    memory_copy(shape->radies, box->corner_radies, sizeof(shape->radies));
                }

                if (box->flags & UI_BoxFlag_DrawHot && box->hot_t > 0.0f) {
                    F32 active_t = box->active_t;
                    if (!(box->flags & UI_BoxFlag_DrawActive)) {
                        active_t = 0.0f;
                    }
                    V4F32 color = color_from_theme(ThemeColor_Hover);
                    color.a *= 0.2f * (box->hot_t - active_t);

                    Render_Shape *rect = draw_rectangle(box->calculated_rectangle, color, 0.0f, 0.0f, 1.0f);
                    memory_copy(rect->radies, box->corner_radies, sizeof(rect->radies));
                }

                if (box->flags & UI_BoxFlag_DrawActive && box->active_t > 0.0f) {
                    Render_Shape *rect = draw_rectangle(box->calculated_rectangle, v4f32(0.0f, 0.0f, 0.0f, 0.0f), 0.0f, 0.0f, 1.0f);
                    V4F32 color = color_from_theme(ThemeColor_Hover);
                    color.r *= 0.3f;
                    color.g *= 0.3f;
                    color.b *= 0.3f;
                    color.a *= 0.5f * box->active_t;
                    rect->colors[Corner_10] = color;
                    rect->colors[Corner_11] = color;
                    memory_copy(rect->radies, box->corner_radies, sizeof(rect->radies));
                }
            }

            if (box->flags & UI_BoxFlag_DrawText) {
                V2F32 origin = ui_box_text_location(box);

                {
                    F32 advance = 0.0f;
                    for (U64 i = 0; i < box->text.letter_count; ++i) {
                        FontCache_Letter *letter = &box->text.letters[i];
                        draw_glyph(
                            r2f32(
                                f32_floor(origin.x + letter->offset.x + advance),
                                f32_floor(origin.y + letter->offset.y),
                                f32_floor(origin.x + letter->offset.x + advance + letter->size.x),
                                f32_floor(origin.y + letter->offset.y + letter->size.y)
                            ),
                            letter->source,
                            letter->texture,
                            box->palette.text
                        );
                        advance += letter->advance;
                    }
                }

                if (box->flags & UI_BoxFlag_DrawFuzzyMatches) {
                    F32 ascent = box->text.ascent;
                    F32 descent = box->text.descent;
                    for (FuzzyMatch *match = box->fuzzy_matches.first; match; match = match->next) {
                        F32 pixel_min =  f32_infinity();
                        F32 pixel_max = -f32_infinity();
                        U64 byte_offset = 0;
                        F32 advance = 0.0f;
                        for (U64 i = 0; i < box->text.letter_count; ++i) {
                            FontCache_Letter *letter = &box->text.letters[i];

                            if (match->min <= byte_offset && byte_offset < match->max) {
                                F32 pre_offset  = advance + letter->offset.x;
                                F32 post_offset = advance + letter->advance;
                                pixel_min = f32_min(pre_offset,  pixel_min);
                                pixel_max = f32_max(post_offset, pixel_max);
                            }

                            advance += letter->advance;
                            byte_offset += letter->decode_size;
                        }
                        V4F32 color = color_from_theme(ThemeColor_Focus);
                        color.a *= 0.2f;
                        draw_rectangle(
                            r2f32(
                                f32_floor(origin.x + pixel_min),
                                f32_floor(origin.y - ascent),
                                f32_floor(origin.x + pixel_max),
                                f32_floor(origin.y - descent)
                            ),
                            color,
                            0,
                            0,
                            0
                        );
                    }
                }
            }

            if (box->flags & UI_BoxFlag_Clip) {
                R2F32 top_clip = draw_clip_top();
                R2F32 new_clip = r2f32_intersect(top_clip, box->calculated_rectangle);
                draw_clip_push(new_clip);
            }

            if (box->draw_list) {
                draw_transform(m3f32_translation(box->calculated_rectangle.min)) {
                    draw_sub_list(box->draw_list);
                }
            }

            if (box->draw_function) {
                box->draw_function(box, box->draw_data);
            }

            UI_BoxIterator iterator = ui_box_iterator_depth_first_pre_order(box);

            // NOTE(simon): We use `<=` because we need to pop our state when
            // moving to our siblings. Traversing siblings sets both `push_count`
            // and `pop_count` to 0.
            U32 pop_index = 0;
            for (UI_Box *parent = box; pop_index <= iterator.pop_count; parent = parent->parent, ++pop_index) {
                if (parent == box && iterator.push_count) {
                    continue;
                }

                if (parent->flags & UI_BoxFlag_Clip) {
                    draw_clip_pop();
                }

                if (parent->flags & UI_BoxFlag_DrawBorder) {
                    Render_Shape *shape = draw_rectangle(r2f32_pad(parent->calculated_rectangle, 1.0f), parent->palette.border, 0.0f, 1.0f, 1.0f);
                    memory_copy(shape->radies, parent->corner_radies, sizeof(shape->radies));

                    if (parent->flags & UI_BoxFlag_DrawHot && parent->hot_t > 0.0f) {
                        V4F32 color = color_from_theme(ThemeColor_Hover);
                        color.a *= parent->hot_t;

                        Render_Shape *rect = draw_rectangle(r2f32_pad(parent->calculated_rectangle, 1.0f), color, 0.0f, 1.0f, 1.0f);
                        memory_copy(rect->radies, parent->corner_radies, sizeof(rect->radies));
                    }
                }

                if (parent->flags & UI_BoxFlag_Clickable && parent->flags & UI_BoxFlag_FocusActive) {
                    V4F32 color = color_from_theme(ThemeColor_Focus);
                    color.a *= 0.2f * box->focus_active_t;
                    Render_Shape *shape = draw_rectangle(parent->calculated_rectangle, color, 0.0f, 0.0f, 0.0f);
                    memory_copy(shape->radies, parent->corner_radies, sizeof(shape->radies));
                }

                if (parent->flags & UI_BoxFlag_Clickable && parent->flags & UI_BoxFlag_FocusActive) {
                    V4F32 color = color_from_theme(ThemeColor_Focus);
                    color.a *= box->focus_active_t;
                    Render_Shape *shape = draw_rectangle(r2f32_pad(parent->calculated_rectangle, 1.0f), color, 0.0f, 1.0f, 1.0f);
                    memory_copy(shape->radies, parent->corner_radies, sizeof(shape->radies));
                }

                if (parent->flags & UI_BoxFlag_Disabled) {
                    V4F32 color = color_from_theme(ThemeColor_DisabledOverlay);
                    color.a *= box->disabled_t;
                    Render_Shape *shape = draw_rectangle(parent->calculated_rectangle, color, 0.0f, 0.0f, 1.0f);
                    memory_copy(shape->radies, box->corner_radies, sizeof(shape->radies));
                }
            }

            box = iterator.next;
        }

        prof_zone_end(prof_draw_ui);
    }
    draw_submit_list(draw_list);
    render_end();

    // NOTE(simon): Animate theme
    {
        B32 is_animating = false;
        for (ThemeColor color_index = 0; color_index < ThemeColor_COUNT; ++color_index) {
            V4F32 *color = &state->theme.colors[color_index];
            V4F32 *target_color = &state->target_theme.colors[color_index];

            is_animating |= f32_abs(target_color->r - color->r) > 0.001f;
            is_animating |= f32_abs(target_color->g - color->g) > 0.001f;
            is_animating |= f32_abs(target_color->b - color->b) > 0.001f;
            is_animating |= f32_abs(target_color->a - color->a) > 0.001f;

            color->r += (target_color->r - color->r) * ui_animation_slow_rate();
            color->g += (target_color->g - color->g) * ui_animation_slow_rate();
            color->b += (target_color->b - color->b) * ui_animation_slow_rate();
            color->a += (target_color->a - color->a) * ui_animation_slow_rate();
        }

        if (is_animating) {
            request_frame();
        }
    }

    // NOTE(simon): Cancel drag and drop if nothing caught it.
    if (state->drag_state == DragState_Dropping) {
        drag_cancel();
    }

    if (state->frames_to_render > 0) {
        --state->frames_to_render;
    }

    if (ui_is_animating_from_context(state->ui)) {
        request_frame();
    }

    if (PROFILE_BUILD) {
        request_frame();
    }

    ++state->frame_index;
    prof_frame_done();
}
