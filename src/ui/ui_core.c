global UI_Box global_ui_null_box = {
    .parent   = &global_ui_null_box,
    .next     = &global_ui_null_box,
    .previous = &global_ui_null_box,
    .first    = &global_ui_null_box,
    .last     = &global_ui_null_box,
};

internal UI_Size ui_size_pixels(F32 pixels, F32 strictness) {
    UI_Size result = { 0 };
    result.kind = UI_Size_Pixels;
    result.value = pixels;
    result.strictness = strictness;
    return result;
}

internal UI_Size ui_size_parent_percent(F32 percent, F32 strictness) {
    UI_Size result = { 0 };
    result.kind = UI_Size_ParentPercent;
    result.value = percent;
    result.strictness = strictness;
    return result;
}

internal UI_Size ui_size_children_sum(F32 strictness) {
    UI_Size result = { 0 };
    result.kind = UI_Size_ChildrenSum;
    result.strictness = strictness;
    return result;
}



internal UI_Context *ui_create(Void) {
    Arena *arena = arena_create();
    UI_Context *ui = arena_push_struct_zero(arena, UI_Context);

    ui->permanent_arena = arena;
    ui->frame_arena = arena_create();
    ui->root = &global_ui_null_box;

    return ui;
}



internal Void ui_begin(Gfx_Context *gfx, UI_Context *ui) {
    // NOTE(simon): Reset stacks
    ui->parent_stack.top      = 0;
    ui->parent_stack.freelist = 0;
    ui->parent_stack.auto_pop = false;
    ui->color_stack.top      = 0;
    ui->color_stack.freelist = 0;
    ui->color_stack.auto_pop = false;
    ui->size_stacks[Axis2_X].top      = 0;
    ui->size_stacks[Axis2_X].freelist = 0;
    ui->size_stacks[Axis2_X].auto_pop = false;
    ui->size_stacks[Axis2_Y].top      = 0;
    ui->size_stacks[Axis2_Y].freelist = 0;
    ui->size_stacks[Axis2_Y].auto_pop = false;
    ui->layout_axis_stack.top      = 0;
    ui->layout_axis_stack.freelist = 0;
    ui->layout_axis_stack.auto_pop = false;

    // NOTE(simon): Give default values to all stacks
    ui_parent_next(ui, &global_ui_null_box);
    ui_color_push(ui, v4f32(0.0f, 0.0f, 0.0f, 0.0f));
    ui_width_push(ui, ui_size_pixels(0.0f, 0.0f));
    ui_height_push(ui, ui_size_pixels(0.0f, 0.0f));
    ui_layout_axis_push(ui, Axis2_X);

    // NOTE(simon): Build root
    V2U32 window_size = gfx_get_window_client_area(gfx);
    ui_width_next(ui, ui_size_pixels(window_size.width, 1.0f));
    ui_height_next(ui, ui_size_pixels(window_size.height, 1.0f));
    ui->root = ui_box_create(ui, 0);

    ui_parent_push(ui, ui->root);
}

internal Void ui_layout_independent_sizes(UI_Box *box, Axis2 axis) {
    if (box->size[axis].kind == UI_Size_Pixels) {
        box->calculated_size.values[axis] = box->size[axis].value;
    }

    for (UI_Box *child = box->first; child != &global_ui_null_box; child = child->next) {
        ui_layout_independent_sizes(child, axis);
    }
}

internal Void ui_layout_upwards_dependent_sizes_no_recurse(UI_Box *box, Axis2 axis) {
    if (box->size[axis].kind == UI_Size_ParentPercent) {
        UI_Box *parent = box->parent;
        while (parent != &global_ui_null_box && parent->size[axis].kind != UI_Size_Pixels) {
            parent = parent->parent;
        }

        box->calculated_size.values[axis] = parent->calculated_size.values[axis] * box->size[axis].value;
    }
}

internal Void ui_layout_upwards_dependent_sizes(UI_Box *box, Axis2 axis) {
    ui_layout_upwards_dependent_sizes_no_recurse(box, axis);

    for (UI_Box *child = box->first; child != &global_ui_null_box; child = child->next) {
        ui_layout_upwards_dependent_sizes(child, axis);
    }
}

internal Void ui_layout_downwards_dependent_sizes(UI_Box *box, Axis2 axis) {
    for (UI_Box *child = box->first; child != &global_ui_null_box; child = child->next) {
        ui_layout_downwards_dependent_sizes(child, axis);
    }

    if (box->size[axis].kind == UI_Size_ChildrenSum) {
        F32 sum = 0.0f;
        if (axis == box->layout_axis) {
            for (UI_Box *child = box->first; child != &global_ui_null_box; child = child->next) {
                sum += child->calculated_size.values[axis];
            }
        } else {
            for (UI_Box *child = box->first; child != &global_ui_null_box; child = child->next) {
                sum = f32_max(sum, child->calculated_size.values[axis]);
            }
        }

        box->calculated_size.values[axis] = sum;
    }
}

internal Void ui_layout_position(UI_Box *box, Axis2 axis) {
    // NOTE(simon): Calculate final rectangle
    box->calculated_rectangle.min.values[axis] = f32_floor(box->calculated_position.values[axis]);
    box->calculated_rectangle.max.values[axis] = f32_floor(box->calculated_position.values[axis] + box->calculated_size.values[axis]);

    // NOTE(simon): Position children
    if (axis == box->layout_axis) {
        F32 position = box->calculated_position.values[axis];
        for (UI_Box *child = box->first; child != &global_ui_null_box; child = child->next) {
            child->calculated_position.values[axis] = position;
            position += child->calculated_size.values[axis];
        }
    } else {
        for (UI_Box *child = box->first; child != &global_ui_null_box; child = child->next) {
            child->calculated_position.values[axis] = box->calculated_position.values[axis];
        }
    }

    // NOTE(simon): Recurse
    for (UI_Box *child = box->first; child != &global_ui_null_box; child = child->next) {
        ui_layout_position(child, axis);
    }
}

internal Void ui_layout_resolve_violations(UI_Box *box, Axis2 axis) {
    if (box->flags & (UI_BoxFlags_OverflowX << axis)) {
        for (UI_Box *child = box->first; child != &global_ui_null_box; child = child->next) {
            ui_layout_upwards_dependent_sizes_no_recurse(child, axis);
        }
    } else {
        if (axis == box->layout_axis) {
            F32 total_size = 0.0f;
            F32 total_adjustable_size = 0.0f;
            for (UI_Box *child = box->first; child != &global_ui_null_box; child = child->next) {
                total_size += child->calculated_size.values[axis];
                total_adjustable_size += child->calculated_size.values[axis] * (1.0f - child->size[axis].strictness);
            }

            F32 violation = total_size - box->calculated_size.values[axis];
            if (violation > 0.0f) {
                // NOTE(simon): Adjust children
                F32 adjust_percent = violation / total_adjustable_size;
                for (UI_Box *child = box->first; child != &global_ui_null_box; child = child->next) {
                    F32 adjustable_size = child->calculated_size.values[axis] * (1.0f - child->size[axis].strictness);
                    child->calculated_size.values[axis] -= adjustable_size * adjust_percent;
                }
            }
        } else {
            for (UI_Box *child = box->first; child != &global_ui_null_box; child = child->next) {
                F32 violation = f32_max(0.0f, child->calculated_size.values[axis] - box->calculated_size.values[axis]);
                child->calculated_size.values[axis] -= violation;
            }
        }
    }

    // NOTE(simon): Recurse
    for (UI_Box *child = box->first; child != &global_ui_null_box; child = child->next) {
        ui_layout_resolve_violations(child, axis);
    }
}

internal Void ui_end(UI_Context *ui) {
    // NOTE(simon): Layout
    for (Axis2 axis = 0; axis < Axis2_COUNT; ++axis) {
        ui_layout_independent_sizes(ui->root, axis);
        ui_layout_upwards_dependent_sizes(ui->root, axis);
        ui_layout_downwards_dependent_sizes(ui->root, axis);
        ui_layout_resolve_violations(ui->root, axis);
        ui_layout_position(ui->root, axis);
    }

    arena_pop_to(ui->frame_arena, 0);
}



internal UI_Box *ui_box_create(UI_Context *ui, UI_BoxFlags flags) {
    UI_Box *box = arena_push_struct_zero(ui->frame_arena, UI_Box);

    // NOTE(simon): Set links
    box->parent = ui->parent_stack.top->item;
    if (box->parent != &global_ui_null_box) {
        // TODO: Make macros work with generic zeros
        dll_insert_next_previous_zero(box->parent->first, box->parent->last, box->parent->last, box, next, previous, &global_ui_null_box);
    }
    box->next     = &global_ui_null_box;
    box->previous = &global_ui_null_box;
    box->first    = &global_ui_null_box;
    box->last     = &global_ui_null_box;

    box->size[Axis2_X] = ui->size_stacks[Axis2_X].top->item;
    box->size[Axis2_Y] = ui->size_stacks[Axis2_Y].top->item;

    box->flags       = flags;
    box->color       = ui->color_stack.top->item;
    box->layout_axis = ui->layout_axis_stack.top->item;

    // NOTE(simon): Handle autopops
    ui_parent_auto_pop(ui);
    ui_color_auto_pop(ui);
    ui_width_auto_pop(ui);
    ui_height_auto_pop(ui);
    ui_layout_axis_auto_pop(ui);

    return box;
}
