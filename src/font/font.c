// Shar's algorithm adapted from https://probablydance.com/2023/04/27/beautiful-branchless-binary-search/#more-11376
internal U32 font_codepoint_to_glyph_index(Font *font, U32 codepoint) {
    if (font->codepoint_count == 0) {
        return 0;
    }

    U32 index = 0;
    U32 step = u32_floor_to_power_of_2(font->codepoint_count);

    if (step != font->codepoint_count && font->codepoints[index + step] < codepoint) {
        if (font->codepoint_count == step + 1) {
            return 0;
        }
        step = u32_floor_to_power_of_2(font->codepoint_count - step - 1);
        index = font->codepoint_count - step;
    }

    for (step /= 2; step != 0; step /= 2) {
        if (font->codepoints[index + step] < codepoint) {
            index += step;
        }
    }

    index += (font->codepoints[index] < codepoint);

    if (font->codepoints[index] == codepoint) {
        return font->glyph_indicies[index];
    } else {
        return 0;
    }
}
