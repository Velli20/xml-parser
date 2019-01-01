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

// List of element and attribute names in test string.

static const PARSER_XML_NAME xml_names[]=
{
    { "element_type_1"     },
    { "element_type_2"     },
    { "element_type_3"     },
    { "element_type_4"     },
    { "element_type_5"     },
    { "element_type_6"     },
    { "element_type_7"     },

    { "testElementId"   },
    { "intAttribute"    },
    { "floatAttribute"  },
    { "stringAttribute" },
};

static const char parse_test_string[]=
{
"<element_type_1 testElementId=\"0\" intAttribute=\"20\" floatAttribute=\"1.230000\" stringAttribute=\"TEST\">\n"
"  <element_type_2 testElementId=\"1\">\n"
"    <element_type_3 testElementId=\"11\"/>\n"
"    <element_type_3 testElementId=\"12\"/>\n"
"    <element_type_3 testElementId=\"13\"/>\n"
"    <element_type_3 testElementId=\"14\"/>\n"
"  </element_type_2>\n"
"  <element_type_4>\n"
"    <element_type_5>\n"
"      <element_type_6 testElementId=\"15\"/>\n"
"      <element_type_6 testElementId=\"17\"/>\n"
"    </element_type_5>\n"
"  </element_type_4>\n"
"  <element_type_7>\n"
"    <element_type_8 testElementId=\"18\"/>\n"
"  </element_type_7>\n"
"</element_type_1>\n"
};

// test_split_parse

static PARSER_ERROR test_split_parse(void)
{
    PARSER_XML*  xml;
    PARSER_CHAR  buffer_out[1024];
    PARSER_INT   buffer_lenght;
    PARSER_INT   bytes_written;
    PARSER_ERROR error;
    int          i;

    // Initialize xml-struct.

    xml= parser_begin();
    if ( !xml )
        return(1);

    // Test split parsing xml 1-byte at the time.

    for ( i= 0, buffer_lenght= strlen(parse_test_string); i < buffer_lenght; i++ )
    {
        error= parser_parse_string(xml, &(parse_test_string[i]), 1, xml_names, sizeof(xml_names)/sizeof(PARSER_XML_NAME));
        if ( error )
            break;
    }

    // End parsing.

    error= parser_finalize(xml);
    if ( error )
        return(error);

    // Write parsed xml back to string format.

    error= parser_write_xml_to_buffer(xml, xml_names, buffer_out, sizeof(buffer_out), &bytes_written, 0);
    if ( error )
        return(error);

    // Check that output buffer matches original string.

    if ( strncmp(buffer_out, parse_test_string, sizeof(buffer_out)) )
    {
        printf("%s %d: Split parse result error: input and output string does not match.\n", __FUNCTION__, __LINE__);
        printf("Original string:\n%s\n\n", parse_test_string);
        printf("Output buffer string:\n%s\n\n", buffer_out);
    }

    // Free xml.

    error= parser_free_xml(xml);
    if ( error )
        return(error);

    return(0);
}

// test_parse

static PARSER_ERROR test_parse(void)
{
    PARSER_XML*  xml;
    PARSER_CHAR  buffer_out[1024];
    PARSER_INT   buffer_lenght;
    PARSER_INT   bytes_written;
    PARSER_ERROR error;

    // Initialize xml-struct.

    xml= parser_begin();
    if ( !xml )
        return(1);

    // Parse string.

    error= parser_parse_string(xml, parse_test_string, strlen(parse_test_string), xml_names, sizeof(xml_names)/sizeof(PARSER_XML_NAME));
    if ( error )
        return(error);

    // End parsing.

    error= parser_finalize(xml);
    if ( error )
        return(error);

    // Write parsed xml back to string.

    error= parser_write_xml_to_buffer(xml, xml_names, buffer_out, sizeof(buffer_out), &bytes_written, 0);
    if ( error )
        return(error);

    // Check that output buffer matches original string.

    if ( strncmp(buffer_out, parse_test_string, sizeof(buffer_out)) )
    {
        printf("%s %d: Parse result error: input and output string does not match.\n", __FUNCTION__, __LINE__);
        printf("Original string:\n%s\n\n", parse_test_string);
        printf("Output buffer string:\n%s\n\n", buffer_out);
    }

    // Free xml.

    error= parser_free_xml(xml);
    if ( error )
        return(error);

    return(0);
}

// List of element names in test string.

static const PARSER_XML_NAME test_find_elements_xml_names[]=
{
    { "element_type_1"  },
    { "element_type_2"  },
    { "element_type_3"  },
    { "element_type_4"  },
    { "element_type_5"  },
    { "element_type_6"  },
    { "element_type_7"  },
    { "element_type_8"  },
    { "element_type_9"  },
    { "element_type_10" },
};

static const char test_find_elements_string[]=
{
"<element_type_1          depth='0'>\n"
"\n"
"  <!-- Depth: 2 -->\n"
"  <element_type_2        depth='1'>\n"
"    <element_type_3      depth='2'/>\n"
"  </element_type_2>\n"
"\n"
"  <!-- Depth: 3 -->\n"
"  <element_type_4        depth='1'>\n"
"    <element_type_5      depth='2'>\n"
"      <element_type_6    depth='3'/>\n"
"    </element_type_5>\n"
"  </element_type_4>\n"
"\n"
"  <!-- Depth: 4 -->\n"
"  <element_type_7        depth='1'>\n"
"    <element_type_8      depth='2'>\n"
"      <element_type_9    depth='3'>\n"
"        <element_type_10 depth='4'/>\n"
"      </element_type_9>\n"
"    </element_type_8>\n"
"  </element_type_7>\n"
"</element_type_1>\n"
};

// test_find_element

static PARSER_ERROR test_find_element(void)
{
    const PARSER_ELEMENT* elem;
    PARSER_XML*           xml;
    PARSER_ERROR          error;
    int                   i;

    // Initialize xml-struct.

    xml= parser_begin();
    if ( !xml )
        return(1);

    // Parse string.

    error= parser_parse_string(xml, test_find_elements_string, strlen(test_find_elements_string), test_find_elements_xml_names, sizeof(test_find_elements_xml_names)/sizeof(PARSER_XML_NAME));
    if ( error )
        return(error);

    // End parsing.

    error= parser_finalize(xml);
    if ( error )
        return(error);

    // Make sure all elements are found.

    for ( i= 0; i < sizeof(test_find_elements_xml_names)/sizeof(PARSER_XML_NAME); i++ )
    {
        elem= parser_find_element(xml, test_find_elements_xml_names, 0, 4, sizeof(test_find_elements_xml_names)/sizeof(PARSER_XML_NAME), test_find_elements_xml_names[i].name);
        if ( !elem )
        {
            printf("%s %d: Element %s not found.\n", __FUNCTION__, __LINE__, test_find_elements_xml_names[i].name);
        }
        else
        {
            printf("%s %d: Element %s found.\n", __FUNCTION__, __LINE__, test_find_elements_xml_names[i].name);
        }
    }

    // Free xml.

    error= parser_free_xml(xml);
    if ( error )
        return(error);

    return(0);
}

int main(void)
{
    PARSER_XML*  xml;
    PARSER_ERROR error;

    // Test parsing.

    error= test_parse();
    if ( error )
    {
        printf("Parse test error: %d\n", error);
        return(error);
    }

    // Test split parsing.

    error= test_split_parse();
    if ( error )
    {
        printf("Split parse test error: %d\n", error);
        return(error);
    }

    // Test element finding.

    error= test_find_element();
    if ( error )
        return(error);

    return(0);
}
