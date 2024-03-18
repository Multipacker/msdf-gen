#ifndef LINUX_ESSENTIAL_H
#define LINUX_ESSENTIAL_H

typedef struct {
    S32 file_descriptor;
    U32 bytes_read;
    U32 first_needed_byte;
    B32 done;
    U8 buffer[255];
} Linux_FileIterator;
static_assert(sizeof(Linux_FileIterator) <= sizeof(OS_FileIterator));

typedef packed_struct({
    U64 inode;
    U64 offset;
    unsigned short record_length;
    unsigned char type;
    char          name[];
}) Linux_DirentHeader;

struct tm;
internal DateTime  linux_date_time_from_tm_and_milliseconds(struct tm *time, U16 milliseconds);
internal struct tm linux_tm_from_date_time(DateTime *date_time);

struct stat;
internal Void linux_file_properties_from_stat(FileProperties *properties, struct stat *metadata);

#endif // LINUX_ESSENTIAL_H
