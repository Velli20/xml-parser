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
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "xml_parser.h"

// List of element names in test string.

static const PARSER_XML_NAME test_1_element_names[]=
{
    { "element_type_1"     },
    { "element_type_2"     },
    { "element_type_3"     },
    { "element_type_4"     },
    { "element_type_5"     },
    { "element_type_6"     },
    { "element_type_7"     },
    { "element_type_8"     },

    { "testElementId"   },
    { "intAttribute"    },
    { "floatAttribute"  },
    { "stringAttribute" },
};

// List attribute names in test string.

static const PARSER_XML_NAME test_1_attribute_names[]=
{
    { "testElementId"   },
    { "intAttribute"    },
    { "floatAttribute"  },
    { "stringAttribute" },
};

static const PARSER_CHAR parse_test_string[]=
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

#define COUNTOF(ARRAY) (sizeof(ARRAY) / sizeof(ARRAY[0]))

// test_split_parse

static PARSER_ERROR test_split_parse(void)
{
    PARSER_XML*  xml;
    PARSER_ERROR error;
    size_t       buffer_lenght;
    size_t       i;

    // Initialize xml-struct.

    xml = parser_begin(test_1_element_names, COUNTOF(test_1_element_names), test_1_attribute_names, COUNTOF(test_1_attribute_names));
    if ( !xml )
        return(1);

    // Test split parsing xml 1-byte at the time.

    buffer_lenght = strlen(parse_test_string);

    for ( i = 0; i < buffer_lenght; i++ )
    {
        error = parser_append(xml, &(parse_test_string[i]), 1);
        if ( error )
            break;
    }

    // End parsing.

    error = parser_finalize(xml);
    if ( error )
        return(error);

    // Free xml.

    error = parser_free_xml(xml);
    if ( error )
        return(error);

    return(0);
}

// test_parse

static PARSER_ERROR test_parse(void)
{
    PARSER_XML*  xml;
    PARSER_ERROR error;

    // Initialize xml-struct.

    xml = parser_begin(test_1_element_names, COUNTOF(test_1_element_names), test_1_attribute_names, COUNTOF(test_1_attribute_names));
    if ( !xml )
        return(1);

    // Parse string.

    error = parser_append(xml, parse_test_string, strlen(parse_test_string));
    if ( error )
        return(error);

    // End parsing.

    error = parser_finalize(xml);
    if ( error )
        return(error);

    // Free xml.

    error = parser_free_xml(xml);
    if ( error )
        return(error);

    return(0);
}

// List of element names in test string.

static const PARSER_XML_NAME test_find_elements_element_names[]=
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

// List of attribute names in test string
static const PARSER_XML_NAME test_find_elements_attribute_names[]=
{
    { "depth"  },
};

static const PARSER_CHAR test_find_elements_string[]=
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
    PARSER_INT            xml_string_length;
    PARSER_INT            element_list_length;
    PARSER_INT            attribute_list_length;
    int                   i;

    xml_string_length     = strlen(test_find_elements_string);
    element_list_length   = COUNTOF(test_find_elements_element_names);
    attribute_list_length = COUNTOF(test_find_elements_attribute_names);

    // Initialize xml-struct.

    xml = parser_begin(test_find_elements_element_names, element_list_length,
                       test_find_elements_attribute_names, attribute_list_length);
    if ( !xml )
        return(1);

    // Parse string.

    error = parser_append(xml, test_find_elements_string, xml_string_length);
    if ( error )
        return(error);

    // Make sure all elements are succesfully parsed.

    for ( i = 0, elem = 0; i < element_list_length; i++ )
    {
        elem = parser_find_element(xml, elem, 4, test_find_elements_element_names[i].name);
        if ( !elem )
        {
            printf("%s %d: Element %s not found.\n", __FUNCTION__, __LINE__, test_find_elements_element_names[i].name);
            error = PARSER_RESULT_ERROR;
            break;
        }
    }

    // End parsing.

    error = parser_finalize(xml);
    if ( error )
        return(error);

    return(0);
}

int main(void)
{
    PARSER_ERROR error;

    // Test parsing.

    error = test_parse();
    if ( error )
    {
        printf("Parse test error: %d\n", error);
        return(error);
    }

    // Test split parsing.

    error = test_split_parse();
    if ( error )
    {
        printf("Split parse test error: %d\n", error);
        return(error);
    }

    // Test element finding.

    error = test_find_element();
    if ( error )
        return(error);

    printf("LIBXML test ok\n");

    return(0);
}
