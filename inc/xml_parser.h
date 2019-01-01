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

#ifndef xml_parser_h
#define xml_paser_h

// Includes

#include <stdio.h>
#include <inttypes.h>
#include "xml_parser_config.h"

// Defines

#define PARSER_RESULT_ERROR                 0x01
#define PARSER_RESULT_OUT_OF_MEMORY         0x02

#define PARSER_UNKNOWN_INDEX                -1

#define PARSER_ELEMENT_CONTENT_TYPE_NONE    0x00
#define PARSER_ELEMENT_CONTENT_TYPE_STRING  0x01
#define PARSER_ELEMENT_CONTENT_TYPE_ELEMENT 0x02

#define PARSER_ELEMENT_NAME_TYPE_INDEX      0x10
#define PARSER_ELEMENT_NAME_TYPE_STRING     0x20
#define PARSER_ELEMENT_NAME_TYPE_NONE       0x40

#define PARSER_ATTRIBUTE_VALUE_TYPE_STRING  0x01
#define PARSER_ATTRIBUTE_VALUE_TYPE_INTEGER 0x02
#define PARSER_ATTRIBUTE_VALUE_TYPE_FLOAT   0x04

#define PARSER_ATTRIBUTE_NAME_TYPE_INDEX    0x10
#define PARSER_ATTRIBUTE_NAME_TYPE_STRING   0x20
#define PARSER_ATTRIBUTE_NAME_TYPE_NONE     0x40


// Parser typedefs

typedef uint8_t PARSER_ERROR;

typedef int32_t PARSER_INT;

typedef float PARSER_FLOAT;

typedef int32_t PARSER_SIZE;

typedef char PARSER_CHAR;

// parser_xml_name

typedef struct parser_xml_name
{
    const char* name;
}
PARSER_XML_NAME;

// parser_attribute

typedef struct parser_attribute
{
    PARSER_INT attribute_type;

    union ATTRIBUTE_VALUE
    {
        char*        string_ptr;
        PARSER_INT   int_value;
        PARSER_FLOAT float_value;
    }
    attr_val;

    // Attribute name either can be dynamically allocated string
    // or an integer that represents index in the
    // PARSER_XML_NAME -list that is passed optionally
    // at the beginning of parsing.

    union ATTRIBUTE_NAME
    {
        PARSER_CHAR* name_string;
        PARSER_INT   attribute_index;
    }
    attr_name;

    struct parser_attribute* next_attribute;
}
PARSER_ATTRIBUTE;

// parser_string

typedef struct parser_string
{
    PARSER_CHAR*          buffer;
    PARSER_SIZE           buffer_size;

    struct parser_string* next_string;
}
PARSER_STRING;


// parser_string_content

typedef struct parser_string_content
{
    PARSER_STRING* first_string;
    PARSER_STRING* last_string;
}
PARSER_STRING_CONTENT;

// parser_child_element

typedef struct parser_child_element
{
    struct parser_element* first_element;
    struct parser_element* last_element;
}
PARSER_CHILD_ELEMENT;

// parser_element

typedef struct parser_element
{
    struct parser_element* next_element;
    struct parser_element* parent_element;

    struct parser_attribute* first_attribute;
    struct parser_attribute* last_attribute;

    // Element name.

    union PARSER_ELEMENT_NAME
    {
        PARSER_INT   name_index;
#if defined(PARSER_WITH_DYNAMIC_NAMES)
        PARSER_CHAR* name_string;
#endif
    }
    elem_name;

    // Element inner content.

    struct parser_string_content inner_string;
    struct parser_child_element  inner_element;

    PARSER_INT content_type;
}
PARSER_ELEMENT;

// parser_state

typedef struct parser_state
{
    PARSER_INT  flags;
    PARSER_INT  name_buf_pos;
    PARSER_INT  value_buf_pos;

    PARSER_CHAR previous_char;
    PARSER_CHAR current_char;
    PARSER_CHAR next_char;

    PARSER_CHAR temp_name_buffer[PARSER_MAX_NAME_STRING_LENGTH];
    PARSER_CHAR temp_value_buffer[PARSER_MAX_NAME_STRING_LENGTH];

    PARSER_ELEMENT* element;
    PARSER_ELEMENT* parent_element;
}
PARSER_STATE;

// parser_xml

typedef struct parser_xml
{
    PARSER_STATE* state;

    struct parser_element* first_element;
    struct parser_element* last_element;
}
PARSER_XML;

// Assert macro

#define PARSER_ASSERT(CONDITION)             \
    if ( !(CONDITION) )                      \
    {                                        \
        printf("Parser assert %s %d: %s",    \
        __FUNCTION__, __LINE__, #CONDITION); \
        return(PARSER_RESULT_ERROR);         \
    }

// parser_begin

PARSER_XML* parser_begin(void);

// parser_finalize

PARSER_ERROR parser_finalize(PARSER_XML* xml);

// parse_xml

PARSER_ERROR parser_parse_string(PARSER_XML*            xml,
                                 const PARSER_CHAR*     xml_string,
                                 PARSER_INT             xml_string_length,
                                 const PARSER_XML_NAME* xml_name_list,
                                 PARSER_INT             xml_name_list_length);

// parser_free_xml

PARSER_ERROR parser_free_xml(PARSER_XML* xml);

// parser_print_xml

PARSER_ERROR parser_write_xml_to_buffer(const PARSER_XML*      xml,
                                        const PARSER_XML_NAME* xml_name_list,
                                        PARSER_CHAR*           buffer,
                                        PARSER_INT             buffer_size,
                                        PARSER_INT*            bytes_written,
                                        PARSER_INT             flags);

// parser_find_element

const PARSER_ELEMENT* parser_find_element(const PARSER_XML*      xml,
                                          const PARSER_XML_NAME* xml_name_list,
                                          const PARSER_ELEMENT*  offset,
                                          PARSER_INT             max_depth,
                                          PARSER_INT             xml_name_list_length,
                                          const PARSER_CHAR*     element_name);

#endif
