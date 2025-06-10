#ifndef META_H
#define META_H

typedef struct Meta_Layer Meta_Layer;
struct Meta_Layer {
    Meta_Layer *next;
    Meta_Layer *previous;

    Str8     path;
    Str8List header_lines;
    Object   object;
};

#endif // META_H
