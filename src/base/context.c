internal OperatingSystem operating_system_from_context(Void) {
    OperatingSystem result = OperatingSystem_Null;
#if OS_WINDOWS
    result = OperatingSystem_Windows;
#elif OS_LINUX
    result = OperatingSystem_Linux;
#elif OS_MAC
    result = OperatingSystem_Mac;
#endif
    return result;
}

internal Architecture architecture_from_context(Void) {
    Architecture result = Architecture_Null;
#if ARCH_X64
    result = Architecture_X64;
#elif ARCH_X86
    result = Architecture_X86;
#elif ARCH_ARM
    result = Architecture_Arm;
#elif ARCH_ARM64
    result = Architecture_Arm64;
#endif
    return result;
}

internal Str8 string_from_operating_system(OperatingSystem os) {
    Str8 result = { 0 };

    switch (os) {
        case OperatingSystem_Windows: {
            result = str8_literal("windows");
        } break;
        case OperatingSystem_Linux: {
            result = str8_literal("linux");
        } break;
        case OperatingSystem_Mac: {
            result = str8_literal("mac");
        } break;
        default: {
            result = str8_literal("(null)");
        } break;
    }

    return result;
}

internal Str8 string_from_architecture(Architecture architecture) {
    Str8 result = { 0 };

    switch (architecture) {
        case Architecture_X64: {
            result = str8_literal("x64");
        } break;
        case Architecture_X86: {
            result = str8_literal("x86");
        } break;
        case Architecture_Arm: {
            result = str8_literal("arm");
        } break;
        case Architecture_Arm64: {
            result = str8_literal("arm64");
        } break;
        default: {
            result = str8_literal("(null)");
        } break;
    }

    return result;
}
