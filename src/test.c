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

int main(void)
{
    PARSER_XML*  xml;
    PARSER_ERROR error;
    PARSER_CHAR  out[1024];
    PARSER_INT   bw;
#if defined(PARSER_TEST_FILE)
    FILE*       file;
    char*       buffer;
    size_t      size;
#endif

    // Initialize xml-struct.

    xml= parser_begin();
    if ( !xml )
    {
        printf("%s %d: XML-struct initialization failed.\n", __FUNCTION__, __LINE__);
        return(1);
    }

#if defined(PARSER_TEST_FILE)

    file= fopen("src\\test_1.xml", "r");
    if ( !file )
    {
        printf("%d %s: Error while opening file.", __FUNCTION__, __LINE__);
        return(1);
    }

    // Get file size.

    fseek(file, 0, SEEK_END);
    size= ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate temporary buffer.
    
    buffer= malloc(sizeof(char)*(size+1));
    PARSER_ASSERT(buffer);

    fread(buffer, 1, size, file);
    fclose(file);

    // Parse xml-string.

    error= parser_parse_string(xml, buffer, elements, sizeof(elements)/sizeof(PARSER_XML_NAME));
    free(buffer);

#else
    // Parse xml-string.

    error= parser_parse_string(xml, test_1, elements, sizeof(elements)/sizeof(PARSER_XML_NAME));
#endif

    if ( error )
    {
        printf("%d %s: Error parsing xml-string.", __FUNCTION__, __LINE__);
        goto end;
    }

    // End parsing.

    error= parser_finalize(xml);
    if ( !error )
    {
        // Write parsed xml back to string.

        error= parser_write_xml_to_buffer(xml, elements, out, sizeof(out), &bw, 0);
        if ( !error )
            printf("Bytes written:%d Buffer content:\n%s", bw, out);
    }

    end:

    // Free xml.

    if ( xml )
        parser_free_xml(xml);

    return(0);
}
