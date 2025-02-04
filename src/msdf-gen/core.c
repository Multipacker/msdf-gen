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
    for (Tab *tab = panel->tab_first; tab; tab = tab->next) {
        tab_free(state, tab);
    }

    ++panel->generation;
    sll_stack_push(state->panel_freelist, panel);
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

    arena_pop_to(frame_arena(), 0);

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
            { Gfx_Key_C,         Gfx_KeyModifier_Control,                         Command_Copy,                 },
            { Gfx_Key_V,         Gfx_KeyModifier_Control,                         Command_Paste,                },
            { Gfx_Key_X,         Gfx_KeyModifier_Control,                         Command_Cut,                  },
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
    for (Gfx_Event *event = events.first, *next; event; event = next) {
        next = event->next;
        B32 consume = false;
        UI_Event *ui_event = 0;

        if (event->kind == Gfx_EventKind_Quit) {
            consume = true;
            state->running = false;
        } else if (event->kind == Gfx_EventKind_KeyPress || event->kind == Gfx_EventKind_KeyRelease || event->kind == Gfx_EventKind_Text || event->kind == Gfx_EventKind_Scroll) {
            consume = true;
            UI_EventKind kind = UI_EventKind_Null;
            switch (event->kind) {
                case Gfx_EventKind_Null:       kind = UI_EventKind_Null;       break;
                case Gfx_EventKind_Quit:       kind = UI_EventKind_Null;       break;
                case Gfx_EventKind_KeyPress:   kind = UI_EventKind_KeyPress;   break;
                case Gfx_EventKind_KeyRelease: kind = UI_EventKind_KeyRelease; break;
                case Gfx_EventKind_Text:       kind = UI_EventKind_Text;       break;
                case Gfx_EventKind_Scroll:     kind = UI_EventKind_Scroll;     break;
                case Gfx_EventKind_Resize:     kind = UI_EventKind_Null;       break;
                case Gfx_EventKind_FileDrop:   kind = UI_EventKind_Null;       break;
                case Gfx_EventKind_COUNT:      kind = UI_EventKind_Null;       break;
            }
            ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
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
                    ui_event->delta.y = -1;
                    ui_event->unit = UI_EventDeltaUnit_Whole;
                    ui_event->flags = UI_EventFlag_KeepMark;
                } break;
                case Command_SelectWholeDown: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Navigation;
                    ui_event->delta.y = 1;
                    ui_event->unit = UI_EventDeltaUnit_Whole;
                    ui_event->flags = UI_EventFlag_KeepMark;
                } break;
                case Command_MoveWholeUp: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Navigation;
                    ui_event->delta.y = -1;
                    ui_event->unit = UI_EventDeltaUnit_Whole;
                    ui_event->flags = UI_EventFlag_PickSelectSide | UI_EventFlag_ZeroDeltaOnSelection;
                } break;
                case Command_MoveWholeDown: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Navigation;
                    ui_event->delta.y = 1;
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
                case Command_Copy: {
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
                    ui_event->kind = UI_EventKind_Edit;
                    ui_event = arena_push_struct_zero(ui_frame_arena(), UI_Event);
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

        // NOTE(simon): state->font_size pts * 96 pixels per inch / 72 points per inch
        ui_font_size_push((U32) (state->font_size * 96.0f / 72.0f));

        R2F32 root_rectangle = r2f32(0.0f, 0.0f, (F32) client_area.x, (F32) client_area.y);

        typedef struct DragTabData DragTabData;
        struct DragTabData {
            Handle panel;
            Handle tab;
        };

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
        for (Panel *panel = state->panel_root; panel; panel = panel_iterator_depth_first_pre_order(panel).next) {
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
                        ui_palette(palette_from_code(PaletteCode_DropSiteOverlay))
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
                            {
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
                    ui_palette(palette_from_code(PaletteCode_DropSiteOverlay))
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
                        {
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

        // NOTE(simon): Build leaf panel UI.
        prof_zone_begin(prof_bulid_leaf_ui, "leaf ui");
        ui_layout_axis(Axis2_Y)
        for (Panel *panel = state->panel_root; panel; panel = panel_iterator_depth_first_pre_order(panel).next) {
            if (panel->first) {
                continue;
            }

            push_context(.panel = handle_from_panel(panel), .tab = panel->active_tab);

            ui_focus(panel == panel_from_handle(state->active_panel) ? UI_Focus_None : UI_Focus_Inactive) {
                R2F32 panel_rectangle = r2f32_pad(rectangle_from_panel(panel, root_rectangle), -panel_pad);

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
                        ui_palette(palette_from_code(PaletteCode_DropSiteOverlay))
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
                            ui_padding(ui_size_pixels(padding, 1.0f)) {
                                if (targets[i].direction != Direction2_Invalid) {
                                    ui_layout_axis_next(axis);
                                    UI_Box *row_or_column = ui_create_box(0);
                                    ui_parent(row_or_column)
                                    ui_padding(ui_size_pixels(padding, 1.0f)) {
                                        ui_create_box(UI_BoxFlag_DrawBorder);
                                        ui_spacer_sized(ui_size_pixels(padding, 1.0f));
                                        ui_create_box(UI_BoxFlag_DrawBorder);
                                    }
                                } else {
                                    ui_layout_axis_next(axis);
                                    UI_Box *row_or_column = ui_create_box(0);
                                    ui_parent(row_or_column)
                                    ui_padding(ui_size_pixels(padding, 1.0f)) {
                                        ui_create_box(UI_BoxFlag_DrawBorder);
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

                for (UI_Event *event = global_ui_state->events->first; event; event = event->next) {
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

                if (panel != panel_from_handle(state->active_panel)) {
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
                ui_corner_radius_00(10.0f)
                ui_corner_radius_01(10.0f) {
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

            UI_BoxIterator iterator = ui_box_iterator_depth_first_post_order(box);

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

                    if (box->flags & UI_BoxFlag_DrawHot && box->hot_t > 0.0f) {
                        V4F32 color = color_from_theme(ThemeColor_Hover);
                        color.a *= box->hot_t;

                        Render_Shape *rect = draw_rectangle(r2f32_pad(box->calculated_rectangle, 1.0f), color, 0.0f, 1.0f, 1.0f);
                        memory_copy(rect->radies, box->corner_radies, sizeof(rect->radies));
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
