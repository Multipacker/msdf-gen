#ifndef BASE_H
#define BASE_H

#if defined(__clang__)
# define COMPILER_CLANG 1
# if defined(_WIN32)
#  define OS_WINDOWS 1
# elif defined(__gnu_linux__)
#  define OS_LINUX 1
# elif defined(__APPLE__) && defined(__MACH__)
#  define OS_MAC 1
# else
#  error missing OS detection
# endif
# if defined(__amd64__)
#  define ARCH_X64 1
# elif defined(__i386__)
#  define ARCH_X86 1
# elif defined(__arm__)
#  define ARCH_ARM 1
# elif define(__aarch64__)
#  define ARCH_ARM64 1
# else
#  error missing ARCH detection
# endif

#elif defined(_MSC_VER)
# define COMPILER_CL 1
# if defined(_WIN32)
#  define OS_WINDOWS 1
# else
#  error missing OS detection
# endif
# if defined(_M_AMD64)
#  define ARCH_X64 1
# elif defined(_M_IX86)
#  define ARCH_X86 1
# elif defined(_M_ARM)
#  define ARCH_ARM 1
# elif defined(_M_ARM64)
#  define ARCH_ARM64 1
# else
#  error missing ARCH detection
# endif

#elif defined(__GNUC__)
# define COMPILER_GCC 1
# if defined(_WIN32)
#  define OS_WINDOWS 1
# elif defined(__gnu_linux__)
#  define OS_LINUX 1
# elif defined(__APPLE__) && defined(__MACH__)
#  define OS_MAC 1
# else
#  error missing OS detection
# endif
# if defined(__amd64__)
#  define ARCH_X64 1
# elif defined(__i386__)
#  define ARCH_X86 1
# elif defined(__arm__)
#  define ARCH_ARM 1
# elif define(__aarch64__)
#  define ARCH_ARM64 1
# else
#  error missing ARCH detection
# endif

#else
# error no context cracking for this compiler
#endif

#if !defined(COMPILER_CL)
# define COMPILER_CL 0
#endif
#if !defined(COMPILER_CLANG)
# define COMPILER_CLANG 0
#endif
#if !defined(COMPILER_GCC)
# define COMPILER_GCC 0
#endif
#if !defined(OS_WINDOWS)
# define OS_WINDOWS 0
#endif
#if !defined(OS_LINUX)
# define OS_LINUX 0
#endif
#if !defined(OS_MAC)
# define OS_MAC 0
#endif
#if !defined(ARCH_X64)
# define ARCH_X64 0
#endif
#if !defined(ARCH_X86)
# define ARCH_X86 0
#endif
#if !defined(ARCH_ARM)
# define ARCH_ARM 0
#endif
#if !defined(ARCH_ARM64)
# define ARCH_ARM64 0
#endif

#if OS_LINUX
// NOTE: This needs to be defined before we include any of the system headers.
# define _GNU_SOURCE
#endif

#if !defined(ENABLE_ASSERT)
# define ENABLE_ASSERT 0
#endif

#define macro_statement(statement) do { statement } while (0)

#if !defined(assert_break)
# define assert_break() (*(volatile int *) 0 = 0)
#endif

#if ENABLE_ASSERT
# define assert(expression) macro_statement( if (!(expression)) { assert_break(); } )
# define static_assert(expression) _Static_assert(expression, "")
#else
# define assert(expression) ((Void) (expression))
# define static_assert(expression)
#endif

#define array_count(array) (sizeof(array) / (sizeof(*array)))

// NOTE: Doing pointer subtraction with a null pointer is undefined behaviour, hence the convoluted mess that follows.
#define integer_from_pointer(pointer) ((U64) ((uintptr_t) (pointer)))
#define pointer_from_integer(integer) ((void *) ((uintptr_t) (integer)))

#define member(type, type_member) (((type *) 0)->type_member)
#define member_offset(type, type_member) integer_from_pointer(&member(type, type_member))

#define kilobytes(value) ((value) << 10)
#define megabytes(value) ((value) << 20)
#define gigabytes(value) ((value) << 30)
#define terabytes(value) ((value) << 40)

#define global       static
#define local        static
#define internal     static
#if COMPILER_CL
#  define thread_local __declspec(thread)
#else
#  define thread_local _Thread_local
#endif

#if COMPILER_CLANG || COMPILER_GCC
# define packed_struct(declaration) struct __attribute__((__packed__)) declaration
#elif COMPILER_CL
# define packed_struct(declaration) __pragma(pack(push, 1)) struct declaration __pragma(pack(pop))
#endif

#if COMPILER_CL
#  include <string.h>
#  define memory_zero(destination, count)         memset((destination), 0, (count))
#  define memory_zero_struct(structure)           memory_zero(structure, sizeof(*structure));
#  define memory_move(destination, source, count) memmove((destination), (source), (count))
#  define memory_copy(destination, source, count) memcpy((destination), (source), (count))
#  define memory_equal(a, b, count)               (memcmp((a), (b), (count)) == 0)
#else
#  define memory_zero(destination, count)         __builtin_memset((destination), 0, (count))
#  define memory_zero_struct(structure)           memory_zero(structure, sizeof(*structure));
#  define memory_move(destination, source, count) __builtin_memmove((destination), (source), (count))
#  define memory_copy(destination, source, count) __builtin_memcpy((destination), (source), (count))
#  define memory_equal(a, b, count)               (__builtin_memcmp((a), (b), (count)) == 0)
#endif

#define dll_insert_next_previous(first, last, p, n, next, previous)                                                      \
    ((first) == 0 ? (((first) = (last) = (n)), (n)->next = (n)->previous = 0) :                                          \
    (p) == 0 ? ((n)->previous = 0, (n)->next = (first), ((first) == 0 ? 0 : ((first)->previous = (n))), (first) = (n)) : \
    (((p)->next == 0 ? 0 : (((p)->next->previous) = (n))), (n)->next = (p)->next, (n)->previous = (p), (p)->next = (n),  \
    ((p) == (last) ? (last) = (n) : 0)))
#define dll_push_back(first, last, node)  dll_insert_next_previous(first, last, last, node, next, previous)
#define dll_push_front(last, first, node) dll_insert_next_previous(last, first, frist, node, previous, next)
#define dll_remove_next_previous(first, last, node, next, previous) \
    ((first) == (last) && (first) == (node) ?                       \
    ((first) = (last) = 0) :                                        \
    ((first) == (node) ?                                            \
    ((first) = (first)->next, (first)->previous = 0) :              \
    ((last) == (node) ?                                             \
    ((last) = (last)->previous, (last)->next = 0) :                 \
    ((node)->previous->next = (node)->next,                         \
    (node)->next->previous = (node)->previous))))
#define dll_remove(first, last, node) dll_remove_next_previous(first, last, node, next, previous)
#define dll_insert_before(first, last, node, new) dll_insert_next_previous(last, first, node, new, previous, next)
#define dll_insert_after(first, last, node, new)  dll_insert_next_previous(first, last, node, new, next, previous)

#define sll_queue_push(first, last, node)     \
    ((first) == 0 ?                           \
    (first) = (last) = (node) :               \
    ((last)->next = (node), (last) = (node)), \
    (node)->next = 0)
#define sll_queue_push_front(first, last, node)     \
    ((first) == 0 ?                                 \
    ((first) = (last) = (node), (node)->next = 0) : \
    ((node)->next = (first), (first) = (node)))
#define sll_queue_pop(first, last) \
    ((first) == (last) ?           \
    ((first) = (last) = 0) :       \
    ((first) = (first)->next))

#define sll_stack_push(first, node)             \
    ((first) == 0 ?                             \
    (first) = (node) :                          \
    ((node)->next = (first), (first) = (node)), \
    (node)->next = 0)
#define sll_stack_pop(first)   \
    ((first) == 0 ?            \
    0 :                        \
    ((first) = (first)->next))

#define swap(a, b, T)           \
    {                           \
        T temp##__LINE__ = (a); \
        (a) = (b);              \
        (b) = temp##__LINE__;   \
    }

#endif // BASE_H
