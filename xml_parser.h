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
#define xml_parser_h

// Includes.

#include <stddef.h>
#include <inttypes.h>

// Error codes.

#if !defined(ENOMEM)
#define ENOMEM 12
#endif

#if !defined(EINVAL)
#define EINVAL 22
#endif

//#define PARSER_INCLUDE_LOG
//#define PARSER_WITH_DYNAMIC_NAMES

#define PARSER_MAX_NAME_STRING_LENGTH       128
#define PARSER_MAX_VALUE_STRING_LENGTH      128

// Defines.

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

// Types.

typedef int32_t PARSER_ERROR;

typedef int32_t PARSER_INT;

typedef float PARSER_FLOAT;

typedef size_t PARSER_SIZE;

typedef char PARSER_CHAR;

typedef struct parser_xml_name
{
    const char* name;
}
PARSER_XML_NAME;

typedef enum
{
    ATTRIBUTE_TYPE_INTEGER = 0,
    ATTRIBUTE_TYPE_FLOAT,
    ATTRIBUTE_TYPE_STRING,
    ATTRIBUTE_TYPE_UNKNOWN
}
PARSER_ATTRIBUTE_TYPE;

typedef struct parser_attribute
{
    PARSER_INT attribute_type;
    PARSER_INT pad;

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

typedef struct parser_string
{
    PARSER_SIZE           buffer_size;
    PARSER_CHAR*          buffer;

    struct parser_string* next_string;
}
PARSER_STRING;

typedef struct parser_string_content
{
    PARSER_STRING* first_string;
    PARSER_STRING* last_string;
}
PARSER_STRING_CONTENT;

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

    // Element inner content.

    struct parser_string_content child_string;
    struct parser_child_element  child_element;
    // Element name.

    union PARSER_ELEMENT_NAME
    {
        PARSER_INT name_index;

#if defined(PARSER_WITH_DYNAMIC_NAMES)
        PARSER_CHAR* name_string;
#endif
    }
    elem_name;


    PARSER_INT content_type;
}
PARSER_ELEMENT;

// parser_state

typedef struct parser_state
{
    PARSER_ELEMENT* element;
    PARSER_ELEMENT* parent_element;

    PARSER_INT  flags;
    PARSER_INT  name_buf_pos;
    PARSER_INT  value_buf_pos;
    PARSER_INT  pad_1;

    PARSER_CHAR previous_char;
    PARSER_CHAR current_char;
    PARSER_CHAR next_char;
    PARSER_CHAR pad_2;

    PARSER_INT  pad_3;
    PARSER_CHAR temp_name_buffer[PARSER_MAX_NAME_STRING_LENGTH];
    PARSER_CHAR temp_value_buffer[PARSER_MAX_VALUE_STRING_LENGTH];
}
PARSER_STATE;

// parser_xml

typedef struct parser_xml
{
    PARSER_STATE* state;

    const PARSER_XML_NAME* element_name_list;
    const PARSER_XML_NAME* attribute_name_list;

    struct parser_element* first_element;
    struct parser_element* last_element;

    PARSER_INT element_name_list_length;
    PARSER_INT attribute_name_list_length;
}
PARSER_XML;

#define PARSER_GET_CHILD_STRING(PARENT_ELEMENT)    ((((PARSER_ELEMENT*)PARENT_ELEMENT)->child_string.first_string))

// parser_malloc

void* parser_malloc(size_t size);

// parser_free

void parser_free(void* ptr);

// parser_begin

PARSER_XML* parser_begin(const PARSER_XML_NAME* element_name_list,
                         PARSER_INT             element_name_list_length,
                         const PARSER_XML_NAME* attribute_name_list,
                         PARSER_INT             attribute_name_list_length);

PARSER_ERROR parser_finalize(PARSER_XML* xml);

PARSER_ERROR parser_append(PARSER_XML*        xml,
                           const PARSER_CHAR* xml_string,
                           PARSER_INT         xml_string_length);

// parser_free_xml

PARSER_ERROR parser_free_xml(PARSER_XML* xml);

// parser_find_element

const PARSER_ELEMENT* parser_find_element(const PARSER_XML*      xml,
                                          const PARSER_ELEMENT*  offset,
                                          PARSER_INT             max_depth,
                                          const PARSER_CHAR*     element_name);

const PARSER_ATTRIBUTE* parser_find_attribute(const PARSER_XML*       xml,
                                              const PARSER_ELEMENT*   element,
                                              const PARSER_ATTRIBUTE* offset,
                                              const PARSER_CHAR*      attribute_name);

const PARSER_ELEMENT* parser_get_next_element(const PARSER_ELEMENT* element);

const PARSER_ELEMENT* parser_get_first_child_element(const PARSER_ELEMENT* element);

PARSER_ATTRIBUTE_TYPE parser_get_attribute_type(const PARSER_ATTRIBUTE* attribute);

PARSER_ERROR parser_get_attribute_int_value(const PARSER_ATTRIBUTE* attribute,
                                            PARSER_INT*             int_value_ptr);

PARSER_ERROR parser_get_attribute_string_value(const PARSER_ATTRIBUTE* attribute,
                                               const PARSER_CHAR**     string_value_ptr);

const PARSER_ATTRIBUTE* parser_get_first_element_attribute(const PARSER_ELEMENT* element);

const PARSER_ATTRIBUTE* parser_get_next_element_attribute(const PARSER_ATTRIBUTE* attribute);

#endif
