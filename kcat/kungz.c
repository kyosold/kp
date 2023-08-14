#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#define CHUNK_SIZE 16384

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <input_gzip_file> <output_file>\n", argv[0]);
        return 1;
    }

    const char *input_file = argv[1];
    const char *output_file = argv[2];

    FILE *source = fopen(input_file, "rb");
    if (!source)
    {
        perror("Error opening input file");
        return 1;
    }

    FILE *dest = fopen(output_file, "wb");
    if (!dest)
    {
        perror("Error opening output file");
        fclose(source);
        return 1;
    }

    unsigned char in_buffer[CHUNK_SIZE];
    unsigned char out_buffer[CHUNK_SIZE];

    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    stream.avail_in = 0;
    stream.next_in = Z_NULL;

    int ret = inflateInit2(&stream, 16 + MAX_WBITS); // +MAX_WBITS for automatic header detection
    if (ret != Z_OK)
    {
        fclose(source);
        fclose(dest);
        fprintf(stderr, "inflateInit2 failed with error code %d\n", ret);
        return 1;
    }

    do
    {
        stream.avail_in = fread(in_buffer, 1, CHUNK_SIZE, source);
        if (ferror(source))
        {
            inflateEnd(&stream);
            fclose(source);
            fclose(dest);
            perror("Error reading input file");
            return 1;
        }
        stream.next_in = in_buffer;

        do
        {
            stream.avail_out = CHUNK_SIZE;
            stream.next_out = out_buffer;

            ret = inflate(&stream, Z_NO_FLUSH);
            if (ret < 0)
            {
                inflateEnd(&stream);
                fclose(source);
                fclose(dest);
                fprintf(stderr, "inflate failed with error code %d\n", ret);
                return 1;
            }

            size_t have = CHUNK_SIZE - stream.avail_out;
            if (fwrite(out_buffer, 1, have, dest) != have || ferror(dest))
            {
                inflateEnd(&stream);
                fclose(source);
                fclose(dest);
                perror("Error writing output file");
                return 1;
            }
        } while (stream.avail_out == 0);
    } while (ret != Z_STREAM_END);

    inflateEnd(&stream);
    fclose(source);
    fclose(dest);

    printf("Decompression successful.\n");

    return 0;
}
