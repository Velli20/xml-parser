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

// Includes

#include "xml_parser.h"
#include <string.h>
#if defined(PARSER_INCLUDE_LOG)
#include <stdio.h>
#include <stdarg.h>
#endif
#include <stdlib.h>
#include <math.h>

// Defines

#define PARSER_STATE_COMMENT_OPEN             0x001
#define PARSER_STATE_ELEMENT_START_TAG_OPEN   0x002
#define PARSER_STATE_ATTRIBUTE_VALUE_OPEN     0x004
#define PARSER_STATE_ELEMENT_NAME_OPEN        0x008
#define PARSER_STATE_ELEMENT_OPEN             0x010
#define PARSER_STATE_ATTRIBUTE_NAME_OPEN      0x020
#define PARSER_STATE_ELEMENT_END_TAG_OPEN     0x040
#define PARSER_STATE_CONTENT_TYPE_STRING      0x080
#define PARSER_STATE_XML_PROLOG_OPEN          0x100
#define PARSER_STATE_PARSE_INT_VALUE          0x200
#define PARSER_STATE_PARSE_FLOAT_VALUE        0x400
#define PARSER_STATE_PARSE_STRING_VALUE       0x800

// Macros

#define IS_VALID_NAME_CHARACHTER(C)((C >= '0'  && C <= 'z' && C != '>' && C !='<'))
#define IS_PARENTHESIS(C)          ((C == '\'' || C == '"'))
#define IS_ALPHA_CHAR(C)           ((C >= 'a'  && C <= 'z') || (C >= 'A' && C <= 'Z'))
#define IS_WHITE_CHAR(C)           ((C == ' '  || C == '\0' ||  C =='\r' || C == '\n'))
#define IS_COMMA_CHAR(C)           ((C == ','  || C == '.'))
#define IS_NUMERIC_CHAR(C)         ((C >= '0'  && C <= '9'))

#define IS_START_OF_ELEMENT_TAG(CURRENT_CHAR, NEXT_CHAR)    (CURRENT_CHAR == '<' && IS_ALPHA_CHAR(NEXT_CHAR))
#define IS_START_OF_ELEMENT_END_TAG(CURRENT_CHAR, NEXT_CHAR)(CURRENT_CHAR == '<' && NEXT_CHAR == '/')
#define IS_END_OF_ELEMENT(CURRENT_CHAR, NEXT_CHAR)          (CURRENT_CHAR == '>' && IS_ALPHA_CHAR(NEXT_CHAR))
#define IS_END_OF_ELEMENT_NAME(CURRENT_CHAR)                (CURRENT_CHAR == '>' || IS_WHITE_CHAR(CURRENT_CHAR))
#define IS_START_OF_ATTRIBUTE_NAME(CURRENT_CHAR, NEXT_CHAR) (CURRENT_CHAR == ' ' && IS_ALPHA_CHAR(NEXT_CHAR))
#define IS_END_OF_ATTRIBUTE_NAME(CURRENT_CHAR)              (CURRENT_CHAR == '=')
#define IS_START_OF_XML_PROLOG(CURRENT_CHAR, NEXT_CHAR)     (CURRENT_CHAR == '<' && NEXT_CHAR == '?')
#define IS_END_OF_XML_PROLOG(CURRENT_CHAR, NEXT_CHAR)       (CURRENT_CHAR == '?' && NEXT_CHAR == '>')

// parser_log

#if defined(PARSER_INCLUDE_LOG)
static void parser_log(int         line,
                       const char* function,
                       const char* format, ...)
{
    va_list arg;

    printf("line: %d function: %s ", line, function);

    va_start (arg, format);
    vfprintf (stdout, format, arg);
    va_end (arg);

    printf("\n");
}
#else
#define parser_log(LINE, FUNCTION, FORMAT, ...)
#endif

// Macros

#define PARSER_ASSERT(CONDITION)                                                      \
    if ( !(CONDITION) )                                                               \
    {                                                                                 \
        parser_log(__LINE__, __FUNCTION__, "Parser assert failed '%s'", #CONDITION);  \
        return(PARSER_RESULT_ERROR);                                                  \
    }

// parser_strncmp

static inline PARSER_INT parser_strncmp(const PARSER_CHAR* str_1,
                                        const PARSER_CHAR* str_2,
                                        PARSER_INT         max_length)
{
    PARSER_INT n;

    for ( n= 0; *str_1 == *str_2 && n < max_length; n++, str_1++, str_2++ )
    {
        if ( *str_1 == '\0' )
            break;
    }

    return(*str_1 - *str_2);
}

// parser_strnlen

static inline PARSER_SIZE parser_strnlen(const PARSER_CHAR* str,
                                         PARSER_SIZE        max_length)
{
    PARSER_SIZE n;

    for ( n= 0; n < max_length && *str != '\0'; n++, str++ );

    return(n);
}

// parser_strncpy

static inline PARSER_SIZE parser_strncpy(const PARSER_CHAR* src,
                                         PARSER_CHAR*       dest,
                                         PARSER_SIZE        max_length)
{
    PARSER_SIZE n;

    for ( n= 0; n < max_length && *src != '\0'; *dest++ = *src++ );

    *dest= '\0';

    return(n);
}

// find_matching_string_index

static inline PARSER_ERROR find_matching_string_index(const PARSER_CHAR*     name_string,
                                                      const PARSER_XML_NAME* name_list,
                                                      PARSER_INT             name_list_count,
                                                      PARSER_INT*            index)
{
    PARSER_INT i;

    if ( !name_string || !*name_string || !index )
    {
        *index = PARSER_UNKNOWN_INDEX;
        return(0);
    }

    if ( !name_list || name_list_count == 0 )
    {
        *index = PARSER_UNKNOWN_INDEX;
        return(0);
    }

    for ( i = 0; i < name_list_count; i++ )
    {
        if ( !name_list[i].name || !*name_list[i].name )
        {
            parser_log(__LINE__, __FUNCTION__, "Invalid string at index: %d", i);
            return(EINVAL);
        }

        // Return index if match is found.

        if ( !parser_strncmp(name_list[i].name, name_string, PARSER_MAX_NAME_STRING_LENGTH) )
        {
            *index = i;
            return(0);
        }
    }

    *index = PARSER_UNKNOWN_INDEX;

    return(0);
}

// parser_add_new_element

static PARSER_ERROR parser_add_new_element(PARSER_XML*        xml,
                                           PARSER_ELEMENT*    parent_element,
                                           const PARSER_CHAR* element_name,
                                           PARSER_ELEMENT**   inserted_child_element)
{
    PARSER_ELEMENT* child_element;
    PARSER_INT      index;
#if defined(PARSER_WITH_DYNAMIC_NAMES)
    PARSER_SIZE     length;
#endif
    PARSER_ERROR    error;

    if ( !xml || !inserted_child_element )
        return(EINVAL);

    *inserted_child_element = 0;

    // Allocate memory for the element struct.

    child_element = parser_malloc(sizeof(PARSER_ELEMENT));
    if ( !child_element )
    {
        parser_log(__LINE__, __FUNCTION__, "Parser: Out of memory");
        return(0);
    }

    memset(child_element, 0, sizeof(PARSER_ELEMENT));

    // Link inner element.

    if ( parent_element )
    {
        if ( !parent_element->child_element.first_element )
        {
            parent_element->child_element.first_element = child_element;
        }

        else if ( !parent_element->child_element.last_element )
        {
            parser_free(child_element);
            parser_log(__LINE__, __FUNCTION__, "Parser error: last_element == NULL");
            return(0);
        }

        else
        {
            parent_element->child_element.last_element->next_element = child_element;
        }

        // Inherit parent element pointer from previous element.

        parent_element->child_element.last_element = child_element;
        child_element->parent_element              = parent_element;
    }

    // Link root element.

    else
    {
        if ( !xml->first_element )
            xml->first_element = child_element;

        if ( xml->last_element )
            xml->last_element->next_element = child_element;

        xml->last_element = child_element;
    }

    *inserted_child_element = child_element;

    // Find element name index in xml name list.

    error = find_matching_string_index(element_name, xml->element_name_list, xml->element_name_list_length, &index);
    if ( error )
    {
        parser_log(__LINE__, __FUNCTION__, "Error %d at finding xml name index", error);
        return(error);
    }

    // Set element name type to xml name list index.

#if defined(PARSER_WITH_DYNAMIC_NAMES)
    if ( index != PARSER_UNKNOWN_INDEX )
    {
#endif
        child_element->elem_name.name_index = index;
        child_element->content_type         = PARSER_ELEMENT_NAME_TYPE_INDEX;

#if defined(PARSER_WITH_DYNAMIC_NAMES)
    }
#endif

    // Otherwise copy full element name.

#if defined(PARSER_WITH_DYNAMIC_NAMES)

    else
    {
        // Get element name length.

        length = parser_strnlen(element_name, PARSER_MAX_NAME_STRING_LENGTH);
        if ( length < 1 )
        {
            child_element->content_type = PARSER_ELEMENT_NAME_TYPE_NONE;
            parser_log(__LINE__, __FUNCTION__, "Warning: Element name length < 1");
            return(0);
        }

        // Allocate memory for the element name string buffer.

        child_element->elem_name.name_string = parser_malloc(sizeof(PARSER_CHAR) * (length + 1));
        if ( !child_element->elem_name.name_string )
        {
            *inserted_child_element = 0;

            parser_log(__LINE__, __FUNCTION__, "Parser: Out of memory");
            parser_free(child_element);
            return(ENOMEM);
        }

        // Copy element name to buffer.

        parser_strncpy(element_name, child_element->elem_name.name_string, (length + 1));

        child_element->content_type = PARSER_ELEMENT_NAME_TYPE_STRING;
    }
#endif

    return(0);
}

// parser_add_attribute_to_element

static PARSER_ERROR parser_add_attribute_to_element(const PARSER_XML*  xml,
                                                    PARSER_ELEMENT*    parent_element,
                                                    const PARSER_CHAR* attribute_name_string,
                                                    const PARSER_CHAR* attribute_value_string)
{
    PARSER_ATTRIBUTE* attribute;
    PARSER_ERROR      error;
    PARSER_SIZE       length;
    PARSER_INT        index;
    PARSER_INT        int_value;
    PARSER_FLOAT      float_value;
    PARSER_SIZE       n;

    if ( !parent_element || !attribute_name_string || !*attribute_name_string || !attribute_value_string || !*attribute_value_string )
    {
        parser_log(__LINE__, __FUNCTION__, "Parser error: Invalid parameter");
        return(EINVAL);
    }

    // Check start/stop indexes.

    length = parser_strnlen(attribute_value_string, PARSER_MAX_VALUE_STRING_LENGTH);
    PARSER_ASSERT(length > 0);

    // Find matching XML name index.

    error = find_matching_string_index(attribute_name_string, xml->attribute_name_list, xml->attribute_name_list_length, &index);
    PARSER_ASSERT(!error);

    // Allocate memory for element attribute struct.

    attribute = parser_malloc(sizeof(PARSER_ATTRIBUTE));
    if ( !attribute )
    {
        parser_log(__LINE__, __FUNCTION__, "Parser: Out of memory");
        return(ENOMEM);
    }

    memset(attribute, 0, sizeof(PARSER_ATTRIBUTE));

    attribute->attribute_type = PARSER_ATTRIBUTE_VALUE_TYPE_INTEGER;

    // Get value data type.

    for ( n= 0, int_value= 0, float_value= 0; n < length; n++ )
    {
        // Switch data type to float.

        if ( IS_COMMA_CHAR(attribute_value_string[n]) && attribute->attribute_type != PARSER_ATTRIBUTE_VALUE_TYPE_FLOAT )
        {
            attribute->attribute_type = PARSER_ATTRIBUTE_VALUE_TYPE_FLOAT;

            float_value = int_value;
            int_value   = 1;
            continue;
        }

        // Switch data type to string.

        else if ( !IS_NUMERIC_CHAR(attribute_value_string[n]) )
        {
            attribute->attribute_type = PARSER_ATTRIBUTE_VALUE_TYPE_STRING;
            break;
        }

        // Parse float.

        if ( attribute->attribute_type == PARSER_ATTRIBUTE_VALUE_TYPE_FLOAT )
        {
            float_value += (PARSER_FLOAT)((PARSER_FLOAT)(attribute_value_string[n] - '0') / (PARSER_FLOAT)(int_value * 10));
            int_value   *= 10;
        }

        // Parse integer.

        else
        {
            int_value *= 10;
            int_value += attribute_value_string[n] - '0';
        }
    }

    // Copy parsed float/integer value to attribute struct.

    if ( attribute->attribute_type == PARSER_ATTRIBUTE_VALUE_TYPE_FLOAT )
    {
        attribute->attr_val.float_value = float_value;
    }

    else if ( attribute->attribute_type == PARSER_ATTRIBUTE_VALUE_TYPE_INTEGER )
    {
        attribute->attr_val.int_value = int_value;
    }

    // Copy full string value.

    else
    {
        // Allocate memory for the value string.

        attribute->attr_val.string_ptr = parser_malloc(((PARSER_SIZE)(length + 1)) * sizeof(PARSER_CHAR));
        if ( !attribute->attr_val.string_ptr )
        {
            parser_log(__LINE__, __FUNCTION__, "Parser error: Out of memory.");
            return(ENOMEM);
        }

        parser_strncpy(attribute_value_string, attribute->attr_val.string_ptr, length + 1);
    }

    // Link attribute to parent element.

    if ( !parent_element->first_attribute )
        parent_element->first_attribute = attribute;

    if ( parent_element->last_attribute )
        parent_element->last_attribute->next_attribute = attribute;

    parent_element->last_attribute = attribute;

    // If attribute name is not found in PARSER_XML_NAME -list then
    // allocate memory for the string buffer and copy full attribute name to it.

    if ( index == PARSER_UNKNOWN_INDEX )
    {
        // Get attribute name length.

        length = parser_strnlen(attribute_name_string, PARSER_MAX_NAME_STRING_LENGTH);
        if ( length < 1 )
        {
            parser_log(__LINE__, __FUNCTION__, "Parser warning: Attribute length < 1.");
            attribute->attribute_type|= PARSER_ATTRIBUTE_NAME_TYPE_NONE;

            return(0);
        }

        attribute->attribute_type        |= PARSER_ATTRIBUTE_NAME_TYPE_STRING;
        attribute->attr_name.name_string  = parser_malloc(sizeof(PARSER_CHAR) * (length + 1));

        if ( !attribute->attr_name.name_string )
        {
            parser_log(__LINE__, __FUNCTION__, "Parser error: Out of memory.");
            return(ENOMEM);
        }

        parser_strncpy(attribute_name_string, attribute->attr_name.name_string, length + 1);
    }

    // No need to copy attrbute name string.

    else
    {
        attribute->attribute_type           |= PARSER_ATTRIBUTE_NAME_TYPE_INDEX;
        attribute->attr_name.attribute_index = index;
    }

    return(0);
}

// parser_copy_element_content_string

static PARSER_ERROR parser_copy_element_content_string(PARSER_ELEMENT*    owner_element,
                                                       const PARSER_CHAR* buffer)
{
    PARSER_STRING* string;
    PARSER_SIZE    length;

    PARSER_ASSERT(owner_element);
    PARSER_ASSERT(buffer);

    // Get content string length.

    length = parser_strnlen(buffer, PARSER_MAX_VALUE_STRING_LENGTH);
    PARSER_ASSERT(length > 0);

    // Allocate memory for the string struct.

    string = parser_malloc(sizeof(PARSER_STRING));
    if ( !string )
        return(ENOMEM);

    // Allocate memory for content string.

    string->buffer = parser_malloc(sizeof(PARSER_CHAR) * ((PARSER_SIZE)(length + 1)));
    if ( !string->buffer )
    {
        parser_free(string);
        parser_log(__LINE__, __FUNCTION__, "Parser error: Out of memory.");
        return(EXIT_FAILURE);
    }

    // Copy element inner content.

    parser_strncpy(buffer, string->buffer, length + 1);

    string->buffer_size = length + 1;
    string->next_string = 0;

    // Link first string struct to owner element.

    if ( !owner_element->child_string.first_string )
        owner_element->child_string.first_string = string;

    if ( owner_element->child_string.last_string )
        owner_element->child_string.last_string->next_string = string;

    owner_element->child_string.last_string = string;

    return(0);
}

// parser_free_element

static PARSER_ERROR parser_free_element(PARSER_ELEMENT* element)
{
    PARSER_ATTRIBUTE* attribute;
    PARSER_ATTRIBUTE* prev_attribute;
    PARSER_ELEMENT*   prev_element;
    PARSER_STRING*    string;
    PARSER_STRING*    prev_string;

    while ( element )
    {
        // Free allocated element name string.

#if defined(PARSER_WITH_DYNAMIC_NAMES)

        if ( element->content_type & PARSER_ELEMENT_NAME_TYPE_STRING && element->elem_name.name_string )
            parser_free(element->elem_name.name_string);

#endif

        // Free inner element structs.

        if ( element->child_element.first_element )
            parser_free_element(element->child_element.first_element);

        // Free inner string structs.

        if ( element->child_string.first_string )
        {
            string = element->child_string.first_string;
            while ( string )
            {
                prev_string = string;
                string      = string->next_string;

                parser_free(prev_string->buffer);
                parser_free(prev_string);
            }
        }

        // Free attributes.

        if ( element->first_attribute )
        {
            attribute = element->first_attribute;
            while ( attribute )
            {
                // Free attribute name string.

                if ( (attribute->attribute_type & PARSER_ATTRIBUTE_NAME_TYPE_STRING) && attribute->attr_name.name_string  )
                {
                    parser_free(attribute->attr_name.name_string);
                }

                // Free attribute value string.

                if ( (attribute->attribute_type & PARSER_ATTRIBUTE_VALUE_TYPE_STRING) && attribute->attr_val.string_ptr )
                {
                    parser_free(attribute->attr_val.string_ptr);
                }

                prev_attribute = attribute;
                attribute      = attribute->next_attribute;

                parser_free(prev_attribute);
            }
        }

        prev_element = element;
        element      = element->next_element;

        // Free previous element struct.

        parser_free(prev_element);
    }

    return(0);
}

// parser_finalize
// Called after xml parsing is completed. Memory alloctated
// for parser state is freed and xml struct becomes no longer
// valid for parsing.

PARSER_ERROR parser_finalize(PARSER_XML* xml)
{
    if ( !xml )
        return(EINVAL);

    if ( !xml->state )
        return(0);

    // Free xml state.

    parser_free(xml->state);

    xml->state= 0;

    return(0);
}

// parser_free_xml

PARSER_ERROR parser_free_xml(PARSER_XML* xml)
{
    PARSER_ERROR error;

    if ( !xml )
        return(EINVAL);

    // Finalize parser state if not finalized yet.

    error = parser_finalize(xml);
    if ( error )
        return(error);

    // Free all buffers.

    error = parser_free_element(xml->first_element);
    if ( error )
        return(error);

    // Free xml struct.

    parser_free(xml);

    return(0);
}

// parser_begin
// Returns new xml struct fith initialized parser state.
// XML struct must be freed after it is no longer needed
// anymore with parser_free_xml() function.

PARSER_XML* parser_begin(const PARSER_XML_NAME* element_name_list,
                         PARSER_INT             element_name_list_length,
                         const PARSER_XML_NAME* attribute_name_list,
                         PARSER_INT             attribute_name_list_length)
{
    PARSER_XML* xml;

#if !defined(PARSER_WITH_DYNAMIC_NAMES)

    // Check element name list.

    if ( !element_name_list || element_name_list_length < 1 )
    {
        parser_log(__LINE__, __FUNCTION__, "Error: No xml element name list.");
        return(0);
    }

#endif

    // Allocate memory for xml struct.

    xml = parser_malloc(sizeof(PARSER_XML));
    if ( !xml )
    {
        parser_log(__LINE__, __FUNCTION__, "Error: Out of memory while allocating xml struct");
        return(0);
    }

    xml->first_element = 0;
    xml->last_element  = 0;

    // Allocate memory for xml parser state.

    xml->state = parser_malloc(sizeof(PARSER_STATE));
    if ( !xml->state )
    {
        parser_log(__LINE__, __FUNCTION__, "Error: Out of memory while allocating xml parser state");
        parser_free(xml);
    }

    xml->state->flags          = 0;
    xml->state->name_buf_pos   = 0;
    xml->state->value_buf_pos  = 0;
    xml->state->previous_char  = 0;
    xml->state->current_char   = 0;
    xml->state->next_char      = 0;
    xml->state->element        = 0;
    xml->state->parent_element = 0;

    xml->element_name_list          = element_name_list;
    xml->element_name_list_length   = element_name_list_length;
    xml->attribute_name_list        = attribute_name_list;
    xml->attribute_name_list_length = attribute_name_list_length;

    return(xml);
}

// parser_append

PARSER_ERROR parser_append(PARSER_XML*        xml,
                           const PARSER_CHAR* xml_string,
                           PARSER_INT         xml_string_length)
{
    PARSER_INT   i;
    PARSER_ERROR error;

    // Check input string...

    if ( !xml_string || !*xml_string || xml_string_length < 1 )
    {
        parser_log(__LINE__, __FUNCTION__, "Error: xml string empty/null");
        return(EINVAL);
    }

    if ( !xml )
    {
        parser_log(__LINE__, __FUNCTION__, "Error: Invalid XML struct.");
        return(EINVAL);
    }

    else if ( !xml->state )
    {
        parser_log(__LINE__, __FUNCTION__, "Error: XML struct is finalized.");
        return(EINVAL);
    }

    // Start parsing.

    for ( i = 0; i < xml_string_length; i++ )
    {
        xml->state->previous_char = xml->state->current_char;
        xml->state->current_char  = xml->state->next_char;
        xml->state->next_char     = i < xml_string_length ? xml_string[i] : '\0';

        // Comment line start.

        if ( !(xml->state->flags & PARSER_STATE_ATTRIBUTE_VALUE_OPEN) && xml->state->current_char == '<' && xml->state->next_char == '!' )
        {
            xml->state->flags |= PARSER_STATE_COMMENT_OPEN;

#if defined(PARSER_DEBUG)
            xml->state->value_buf_pos = 0;
#endif
            continue;
        }

        // Comment line end.

        if ( (xml->state->flags & PARSER_STATE_COMMENT_OPEN) && xml->state->current_char == '-' && xml->state->next_char == '>' )
        {
            // Skip next character.

            xml->state->flags &= ~PARSER_STATE_COMMENT_OPEN;

#if defined(PARSER_DEBUG)
            xml->state->temp_value_buffer[xml->state->value_buf_pos] = '\0';
            parser_log(__LINE__, __FUNCTION__, "Comment: |%s|\n", xml->state->temp_value_buffer);
#endif /* PARSER_DEBUG */
            continue;
        }

        // Skip comment line chars.

        if ( (xml->state->flags & PARSER_STATE_COMMENT_OPEN) )
        {
#if defined(PARSER_DEBUG)
            xml->state->temp_value_buffer[xml->state->value_buf_pos] = xml->state->current_char;
            xml->state->value_buf_pos += 1;

#endif /* PARSER_DEBUG */
            continue;
        }

        // Store attribute value in to a temporary value buffer.

        if ( (xml->state->flags & PARSER_STATE_ATTRIBUTE_VALUE_OPEN) && !IS_PARENTHESIS(xml->state->current_char) )
        {
            xml->state->temp_value_buffer[xml->state->value_buf_pos] = xml->state->current_char;
            xml->state->value_buf_pos += 1;

            continue;
        }

        // XML prolog start.

        if ( IS_START_OF_XML_PROLOG(xml->state->current_char, xml->state->next_char) )
        {
            xml->state->flags |= PARSER_STATE_XML_PROLOG_OPEN;
            continue;
        }

        // XML prolog end.

        if ( (xml->state->flags & PARSER_STATE_XML_PROLOG_OPEN) && xml->state->previous_char == '?' && xml->state->current_char == '>' )
        {
            xml->state->flags &= ~PARSER_STATE_XML_PROLOG_OPEN;
            continue;
        }

        // Beginning of an element start tag.

        if ( !(xml->state->flags & PARSER_STATE_ELEMENT_START_TAG_OPEN) && IS_START_OF_ELEMENT_TAG(xml->state->current_char, xml->state->next_char) )
        {
            xml->state->flags |= PARSER_STATE_ELEMENT_START_TAG_OPEN;
            xml->state->flags |= PARSER_STATE_ELEMENT_NAME_OPEN;

            if ( (xml->state->flags & PARSER_STATE_CONTENT_TYPE_STRING) && xml->state->value_buf_pos > 0 )
            {
                xml->state->temp_value_buffer[xml->state->value_buf_pos] = '\0';

                // Copy element content string.

                error = parser_copy_element_content_string(xml->state->element, xml->state->temp_value_buffer);
                if ( error )
                {
                    parser_log(__LINE__, __FUNCTION__, "Error %d while copying string", error);
                    return(error);
                }
            }

            xml->state->name_buf_pos = 0;

            continue;
        }

        // End of element name.

        if ( (xml->state->flags & PARSER_STATE_ELEMENT_NAME_OPEN) && IS_END_OF_ELEMENT_NAME(xml->state->current_char) )
        {
            xml->state->flags &= ~PARSER_STATE_ELEMENT_NAME_OPEN;
            xml->state->temp_name_buffer[xml->state->name_buf_pos] = '\0';

            // Add new element to xml struct.

            error = parser_add_new_element(xml, xml->state->parent_element, xml->state->temp_name_buffer, &(xml->state->element));
            if ( error )
            {
                parser_log(__LINE__, __FUNCTION__, "Error while inserting new element.");
                return(error);
            }
        }

        // Write element name charachter in to a temporary name buffer.

        if ( (xml->state->flags & PARSER_STATE_ELEMENT_NAME_OPEN) && IS_VALID_NAME_CHARACHTER(xml->state->current_char) )
        {
            xml->state->temp_name_buffer[xml->state->name_buf_pos] = xml->state->current_char;
            xml->state->name_buf_pos += 1;
            continue;
        }

        // End of element start tag.

        if ( (xml->state->flags & PARSER_STATE_ELEMENT_START_TAG_OPEN) && xml->state->current_char == '>' )
        {
            xml->state->flags &= ~PARSER_STATE_ELEMENT_START_TAG_OPEN;
            xml->state->flags &= ~PARSER_STATE_ATTRIBUTE_NAME_OPEN;
            xml->state->flags &= ~PARSER_STATE_CONTENT_TYPE_STRING;

            if ( !xml->state->element )
            {
                parser_log(__LINE__, __FUNCTION__, "Error: !element");
                return(EINVAL);
            }

            // Element content start.

            if ( xml->state->element && xml->state->previous_char != '/' )
            {
                xml->state->parent_element = xml->state->element;
                xml->state->value_buf_pos  = 0;

                xml->state->flags |= PARSER_STATE_ELEMENT_OPEN;
            }

            continue;
        }

        // Element closing tag beginning.

        if ( (xml->state->flags & PARSER_STATE_ELEMENT_OPEN) && IS_START_OF_ELEMENT_END_TAG(xml->state->current_char, xml->state->next_char) )
        {
            if ( !xml->state->element )
            {
                parser_log(__LINE__, __FUNCTION__, "Error: !element");
                return(EINVAL);
            }

            if ( (xml->state->flags & PARSER_STATE_CONTENT_TYPE_STRING) && xml->state->value_buf_pos > 0 )
            {
                xml->state->temp_value_buffer[xml->state->value_buf_pos] = '\0';

                // Copy element content string.

                error = parser_copy_element_content_string(xml->state->element, xml->state->temp_value_buffer);
                if ( error )
                {
                    parser_log(__LINE__, __FUNCTION__, "Error while inserting new element.");
                    return(error);
                }
            }

            xml->state->flags |=  PARSER_STATE_ELEMENT_END_TAG_OPEN;
            xml->state->flags &= ~PARSER_STATE_CONTENT_TYPE_STRING;

            xml->state->name_buf_pos = 0;
            xml->state->element      = xml->state->parent_element;
        }

        // Copy content string in to a temporary value buffer.

        else if ( (xml->state->flags & PARSER_STATE_ELEMENT_OPEN)        &&
                  (xml->state->flags & PARSER_STATE_CONTENT_TYPE_STRING) &&
                 !(xml->state->flags & PARSER_STATE_ELEMENT_START_TAG_OPEN) )
        {
            xml->state->temp_value_buffer[xml->state->value_buf_pos]= xml->state->current_char;
            xml->state->value_buf_pos += 1;
        }

        // Set element content type to string unless child element starting tag occurs later.

        else if ( (xml->state->flags & PARSER_STATE_ELEMENT_OPEN)           &&
                 !(xml->state->flags & PARSER_STATE_CONTENT_TYPE_STRING)    &&
                 !(xml->state->flags & PARSER_STATE_ELEMENT_START_TAG_OPEN) &&
                 !(xml->state->flags & PARSER_STATE_ELEMENT_END_TAG_OPEN)   && (IS_ALPHA_CHAR(xml->state->current_char) || IS_NUMERIC_CHAR(xml->state->current_char)) )
        {
            xml->state->flags        |= PARSER_STATE_CONTENT_TYPE_STRING;
            xml->state->value_buf_pos = 0;

            xml->state->temp_value_buffer[xml->state->value_buf_pos] = xml->state->current_char;
            xml->state->value_buf_pos += 1;
        }

        // Element closing tag end.

        if ( (xml->state->flags & PARSER_STATE_ELEMENT_END_TAG_OPEN) && xml->state->current_char == '>' )
        {
            xml->state->flags&= ~PARSER_STATE_ELEMENT_END_TAG_OPEN;

            if ( xml->state->element->parent_element )
            {
                xml->state->element        = xml->state->element->parent_element;
                xml->state->parent_element = xml->state->element;
            }

            else
            {
                xml->state->parent_element = 0;
                xml->state->flags         &= ~PARSER_STATE_ELEMENT_OPEN;
            }
        }

        // Start of attribute name.

        if ( !(xml->state->flags & PARSER_STATE_ATTRIBUTE_NAME_OPEN)    &&
             !(xml->state->flags & PARSER_STATE_ATTRIBUTE_VALUE_OPEN)   &&
             ((xml->state->flags & PARSER_STATE_ELEMENT_START_TAG_OPEN) ||
              (xml->state->flags & PARSER_STATE_XML_PROLOG_OPEN))       &&
             IS_START_OF_ATTRIBUTE_NAME(xml->state->current_char, xml->state->next_char) )
        {
            xml->state->flags       |= PARSER_STATE_ATTRIBUTE_NAME_OPEN;
            xml->state->name_buf_pos = 0;

            continue;
        }

        // End of attribute name.

        if ( (xml->state->flags & PARSER_STATE_ATTRIBUTE_NAME_OPEN) && IS_END_OF_ATTRIBUTE_NAME(xml->state->current_char) )
        {
            xml->state->flags &= ~PARSER_STATE_ATTRIBUTE_NAME_OPEN;
            xml->state->temp_name_buffer[xml->state->name_buf_pos] = '\0';

            continue;
        }

        // Store attribute name in to a temporary name buffer.

        if ( (xml->state->flags & PARSER_STATE_ATTRIBUTE_NAME_OPEN) )
        {
            xml->state->temp_name_buffer[xml->state->name_buf_pos] = xml->state->current_char;
            xml->state->name_buf_pos += 1;

            continue;
        }

        // Attribute value start/end.

        if ( (xml->state->flags & (PARSER_STATE_ELEMENT_START_TAG_OPEN | PARSER_STATE_XML_PROLOG_OPEN)) && IS_PARENTHESIS(xml->state->current_char) )
        {
            // Attribute value start.

            if ( !(xml->state->flags & PARSER_STATE_ATTRIBUTE_VALUE_OPEN) )
            {
                // By default expect attribute value to be integer.

                xml->state->flags |= PARSER_STATE_PARSE_INT_VALUE;
                xml->state->flags |= PARSER_STATE_ATTRIBUTE_VALUE_OPEN;

                xml->state->value_buf_pos = 0;
            }

            // Attribute value end.

            else if ( (xml->state->flags & PARSER_STATE_ELEMENT_START_TAG_OPEN) )
            {
                xml->state->flags &= ~PARSER_STATE_ATTRIBUTE_VALUE_OPEN;
                xml->state->flags &= ~PARSER_STATE_PARSE_INT_VALUE;
                xml->state->flags &= ~PARSER_STATE_PARSE_FLOAT_VALUE;
                xml->state->flags &= ~PARSER_STATE_PARSE_STRING_VALUE;

                xml->state->temp_value_buffer[xml->state->value_buf_pos] = '\0';

#if !defined(PARSER_WITH_DYNAMIC_NAMES)

                // Give warning and skip attribute if attribute is not found in
                // attribute names list.

                if ( !xml->attribute_name_list || xml->attribute_name_list_length < 1 )
                {
                    parser_log(__LINE__, __FUNCTION__, "Warning: Attribute found in xml while attribute name list is empty and compiled without dynamically allocated string buffers.");
                    continue;
                }

#endif /* !PARSER_WITH_DYNAMIC_NAMES */

                // Link attribute to parent element.

                error = parser_add_attribute_to_element(xml, xml->state->element, xml->state->temp_name_buffer, xml->state->temp_value_buffer);
                if ( error )
                {
                    parser_log(__LINE__, __FUNCTION__, "Error while inserting new attribute.");
                    return(error);
                }
            }

            // XML prolog attribute end.

            else if ( (xml->state->flags & PARSER_STATE_XML_PROLOG_OPEN) )
            {
                xml->state->flags &= ~PARSER_STATE_ATTRIBUTE_VALUE_OPEN;
                xml->state->temp_value_buffer[xml->state->value_buf_pos] = '\0';

                parser_log(__LINE__, __FUNCTION__, "XML prolog: %s=\"%s\"\n", xml->state->temp_name_buffer, xml->state->temp_value_buffer);
            }
        }

    }

    return(0);
}

// parser_find_element
// Returns first element after offset with matching element name.

const PARSER_ELEMENT* parser_find_element(const PARSER_XML*     xml,
                                          const PARSER_ELEMENT* offset,
                                          PARSER_INT            max_depth,
                                          const PARSER_CHAR*    element_name)
{
    const PARSER_XML_NAME* xml_name_list;
    const PARSER_ELEMENT*  element;
    PARSER_INT             xml_name_list_length;
    PARSER_INT             depth;

    if ( !xml || !xml->state )
    {
        parser_log(__LINE__, __FUNCTION__, " Error: Invalid XML-pointer.");
        return(0);
    }

    xml_name_list        = xml->element_name_list;
    xml_name_list_length = xml->element_name_list_length;

#if !defined(PARSER_WITH_DYNAMIC_NAMES)

    if ( !xml_name_list || xml_name_list_length < 1 )
    {
        parser_log(__LINE__, __FUNCTION__, " Error: XML-name list is empty.");
        return(0);
    }

#endif

    if ( !element_name || !*element_name )
    {
        parser_log(__LINE__, __FUNCTION__, " Error: Inavlid element name.");
        return(0);
    }

    // Iterate trough all elements if no element offset is provided.

    element = offset ? offset : xml->first_element;
    depth   = 0;

    while ( element && depth < max_depth )
    {
        // Return current element if name matches.

        if ( (element->content_type & PARSER_ELEMENT_NAME_TYPE_INDEX) &&
              element->elem_name.name_index != PARSER_UNKNOWN_INDEX &&
              element->elem_name.name_index < xml_name_list_length )
        {
            if ( !parser_strncmp(xml_name_list[element->elem_name.name_index].name, element_name, PARSER_MAX_NAME_STRING_LENGTH) )
                return(element);
        }

#if defined(PARSER_WITH_DYNAMIC_NAMES)

        else if ( (element->content_type & PARSER_ELEMENT_NAME_TYPE_STRING) && element->elem_name.name_string )
        {
            if ( !parser_strncmp(element->elem_name.name_string, element_name, PARSER_MAX_NAME_STRING_LENGTH) )
                return(element);
        }

#endif

        // Move to inner element.

        if ( element->child_element.first_element && depth < max_depth)
        {
            element = element->child_element.first_element;
            depth++;
        }

        // If no more elements are available and element has parent element then move to parent element.

        else if ( !element->next_element && element->parent_element && !element->child_element.first_element )
        {
            // Iterate until parent element with next element is found.

            while ( element && element->parent_element )
            {
                depth--;

                if ( element->parent_element->next_element )
                {
                    element = element->parent_element->next_element;
                    break;
                }

                else
                {
                    element = element->parent_element;
                }
            }
        }

        // Move to next element in linked list.

        else
        {
            element = element->next_element;
        }
    }

    return(0);
}

// parser_find_attribute

const PARSER_ATTRIBUTE* parser_find_attribute(const PARSER_XML*       xml,
                                              const PARSER_ELEMENT*   element,
                                              const PARSER_ATTRIBUTE* offset,
                                              const PARSER_CHAR*      attribute_name)
{
    const PARSER_ATTRIBUTE* attribute;
    const PARSER_XML_NAME*  xml_name_list;
    PARSER_INT              xml_name_list_length;

    if ( !xml )
    {
        parser_log(__LINE__, __FUNCTION__, " Error: Inavlid XML-struct pointer.");
        return(0);
    }

    // Element can be null if attribute offset is given.

    if ( !element && !offset )
    {
        parser_log(__LINE__, __FUNCTION__, " Error: No element or attribute offset pointer.");
        return(0);
    }

    xml_name_list        = xml->attribute_name_list;
    xml_name_list_length = xml->attribute_name_list_length;

#if !defined(PARSER_WITH_DYNAMIC_NAMES)

    if ( !xml_name_list || xml_name_list_length < 1 )
    {
        parser_log(__LINE__, __FUNCTION__, " Error: XML-name list is empty.");
        return(0);
    }

#endif

    if ( !attribute_name || !*attribute_name )
    {
        parser_log(__LINE__, __FUNCTION__, " Error: Invalid attribute name.");
        return(0);
    }

    attribute = offset ? offset : element->first_attribute;

    // Iterate trough attribute list.

    while ( attribute )
    {
        // Return attribute pointer if name matches the name in the xml-name list with given index.

        if ( (attribute->attribute_type & PARSER_ATTRIBUTE_NAME_TYPE_INDEX) &&
             (attribute->attr_name.attribute_index != PARSER_UNKNOWN_INDEX) &&
             xml_name_list && (attribute->attr_name.attribute_index < xml_name_list_length &&
             !parser_strncmp(xml_name_list[attribute->attr_name.attribute_index].name, attribute_name, PARSER_MAX_NAME_STRING_LENGTH)) )
        {
            return(attribute);
        }

#if defined(PARSER_WITH_DYNAMIC_NAMES)

        // If compiled with dynamically allocated string buffers.

        else if ( (attribute->attribute_type & PARSER_ATTRIBUTE_NAME_TYPE_STRING)         &&
                  (attribute->attr_name.name_string && *attribute->attr_name.name_string) &&
                  !parser_strncmp(attribute->attr_name.name_string, attribute_name, PARSER_MAX_NAME_STRING_LENGTH) )
        {
            return(attribute);
        }

#endif

        attribute = attribute->next_attribute;
    }

    return(0);
}

// parser_get_next_element

const PARSER_ELEMENT* parser_get_next_element(const PARSER_ELEMENT* element)
{
    if ( !element )
        return(0);

    return(element->next_element);
}

// parser_get_first_child_element

const PARSER_ELEMENT* parser_get_first_child_element(const PARSER_ELEMENT* element)
{
    if ( !element )
        return(0);

    return(element->child_element.first_element);
}

// parser_get_attribute_type

PARSER_ATTRIBUTE_TYPE parser_get_attribute_type(const PARSER_ATTRIBUTE* attribute)
{
    if ( !attribute )
        return(ATTRIBUTE_TYPE_UNKNOWN);

    if ( attribute->attribute_type & PARSER_ATTRIBUTE_VALUE_TYPE_INTEGER )
        return(ATTRIBUTE_TYPE_INTEGER);

    if ( attribute->attribute_type & PARSER_ATTRIBUTE_VALUE_TYPE_FLOAT )
        return(ATTRIBUTE_TYPE_FLOAT);

    if ( attribute->attribute_type & PARSER_ATTRIBUTE_VALUE_TYPE_STRING )
        return(ATTRIBUTE_TYPE_STRING);

    return(ATTRIBUTE_TYPE_UNKNOWN);
}

// parser_get_first_element_attribute

const PARSER_ATTRIBUTE* parser_get_first_element_attribute(const PARSER_ELEMENT* element)
{
    if ( !element )
        return(0);

    return(element->first_attribute);
}

// parser_get_next_element_attribute

const PARSER_ATTRIBUTE* parser_get_next_element_attribute(const PARSER_ATTRIBUTE* attribute)
{
    if ( !attribute )
        return(0);

    return(attribute->next_attribute);
}

// parser_get_attribute_int_value

PARSER_ERROR parser_get_attribute_int_value(const PARSER_ATTRIBUTE* attribute,
                                            PARSER_INT*             int_value_ptr)
{
    if ( !attribute || !int_value_ptr )
        return(EINVAL);

    if ( !(attribute->attribute_type & PARSER_ATTRIBUTE_VALUE_TYPE_INTEGER) )
        return(EINVAL);

    *int_value_ptr = attribute->attr_val.int_value;

    return(0);
}

// parser_get_attribute_string_value

PARSER_ERROR parser_get_attribute_string_value(const PARSER_ATTRIBUTE* attribute,
                                               const PARSER_CHAR**     string_value_ptr)
{
    if ( !attribute || !string_value_ptr )
        return(EINVAL);

    *string_value_ptr = 0;

    if ( !(attribute->attribute_type & PARSER_ATTRIBUTE_VALUE_TYPE_STRING) )
        return(EINVAL);

    *string_value_ptr = attribute->attr_val.string_ptr;

    return(0);
}