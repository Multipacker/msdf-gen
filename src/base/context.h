#ifndef CONTEXT_H
#define CONTEXT_H

internal OperatingSystem operating_system_from_context(Void);
internal Architecture    architecture_from_context(Void);

internal Str8 string_from_operating_system(OperatingSystem os);
internal Str8 string_from_architecture(Architecture architecture);

#endif // CONTEXT_H
