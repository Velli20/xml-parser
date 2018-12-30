/*
MIT License

Copyright (c) 2018 Velli20

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "test.h"
#include "xml_parser.h"

static const PARSER_XML_NAME elements[]=
{
    // Predefined element and attribute names.

    { "ELEM_type_1"     },
    { "ELEM_type_2"     },
    { "ELEM_type_3"     },

    { "testElementId"   },
    { "intAttribute"    },
    { "floatAttribute"  },
    { "stringAttribute" },
};

// test_split_parse

static PARSER_ERROR test_split_parse(const PARSER_CHAR* buffer,
                                     PARSER_INT         buffer_lenght,
                                     PARSER_CHAR*       buffer_out,
                                     PARSER_INT         buffer_out_size,
                                     PARSER_INT*        bytes_written)
{
    PARSER_XML*  xml;
    PARSER_ERROR error;
    int          i;

    // Initialize xml-struct.

    xml= parser_begin();
    if ( !xml )
    {
        printf("%s %d: XML-struct initialization failed.\n", __FUNCTION__, __LINE__);
        return(1);
    }

    // Test split parsing xml 1-byte at the time.

    for ( i= 0; i < buffer_lenght; i++ )
    {
        error= parser_parse_string(xml, &(buffer[i]), 1, elements, sizeof(elements)/sizeof(PARSER_XML_NAME));
        if ( error )
        {
            printf("%d %s: Error split parsing xml-string.", __FUNCTION__, __LINE__);
            break;
        }
    }

    // End parsing.

    error= parser_finalize(xml);
    if ( !error )
    {
        // Write parsed xml back to string.

        error= parser_write_xml_to_buffer(xml, elements, buffer_out, buffer_out_size, bytes_written, 0);
    }

    return(error);
}

// test_parse

static PARSER_ERROR test_parse(const PARSER_CHAR* buffer,
                               PARSER_INT         buffer_lenght,
                               PARSER_CHAR*       buffer_out,
                               PARSER_INT         buffer_out_size,
                               PARSER_INT*        bytes_written)
{
    PARSER_XML*  xml;
    PARSER_ERROR error;

    // Initialize xml-struct.

    xml= parser_begin();
    if ( !xml )
    {
        printf("%s %d: XML-struct initialization failed.\n", __FUNCTION__, __LINE__);
        return(1);
    }

    // Parse string.

    error= parser_parse_string(xml, buffer, buffer_lenght, elements, sizeof(elements)/sizeof(PARSER_XML_NAME));
    if ( error )
    {
        printf("%d %s: Error split parsing xml-string.", __FUNCTION__, __LINE__);
    }

    // End parsing.

    parser_finalize(xml);
    if ( !error )
    {
        // Write parsed xml back to string.

        error= parser_write_xml_to_buffer(xml, elements, buffer_out, buffer_out_size, bytes_written, 0);
    }

    return(error);
}

int main(void)
{
    PARSER_XML*  xml;
    PARSER_ERROR error;
    PARSER_CHAR  out[1024];
    PARSER_INT   bw;
    PARSER_INT   i;
    size_t       xml_string_length;
#if defined(PARSER_TEST_READ_FROM_FILE)
    FILE*       file;
    char*       buffer;
#else
    const char* buffer;
#endif

#if defined(PARSER_TEST_READ_FROM_FILE)

    file= fopen("src\\test_1.xml", "r");
    if ( !file )
    {
        printf("%d %s: Error while opening file.", __FUNCTION__, __LINE__);
        return(1);
    }

    // Get file size.

    fseek(file, 0, SEEK_END);
    xml_string_length= ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate temporary buffer.

    buffer= malloc(sizeof(char)*(xml_string_length+1));
    PARSER_ASSERT(buffer);

    fread(buffer, 1, xml_string_length, file);
    fclose(file);
#else

    // Use xml string from test.h

    buffer= test_1;
    xml_string_length= strlen(test_1);
#endif

    // Test parsing.

    error= test_parse(buffer, xml_string_length, out, sizeof(out), &bw);
    if ( error )
    {
        printf("Parse test error: %d\n%s", bw, out);
        return(error);
    }

    printf("Parse test: Bytes written:%d Buffer content:\n%s", bw, out);

    // Test split parsing.

    error= test_split_parse(buffer, xml_string_length, out, sizeof(out), &bw);
    if ( error )
    {
        printf("Parse test error: %d\n%s", bw, out);
        return(error);
    }

    printf("Split parse test: Bytes written:%d Buffer content:\n%s", bw, out);

    return(0);
}
