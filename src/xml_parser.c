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
#endif

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

    return (*str_1 - *str_2);
}

// parser_strnlen

static inline PARSER_INT parser_strnlen(const PARSER_CHAR* str,
                                        PARSER_INT         max_length)
{
    PARSER_INT n;

    for ( n= 0; n < max_length && *str != '\0'; n++, str++ );

    return (n);
}

// parser_strncpy

static inline PARSER_INT parser_strncpy(const PARSER_CHAR* src,
                                        PARSER_CHAR*       dest,
                                        PARSER_INT         max_length)
{
    PARSER_INT n;

    for ( n= 0; n < max_length && *src != '\0'; *dest++ = *src++ );

    *dest= '\0';

    return (n);
}


// find_matching_string_index

static inline PARSER_ERROR find_matching_string_index(const PARSER_CHAR*     name_string,
                                                      const PARSER_XML_NAME* name_list,
                                                      PARSER_INT             name_list_count,
                                                      PARSER_INT*            index)
{
    PARSER_INT i;

    if ( !name_string || !*name_string || !index )
        return (-1);

    if ( !name_list || name_list_count == 0 )
        return (-1);

    for ( i= 0; i < name_list_count; i++ )
    {
        if ( !name_list[i].name || !*name_list[i].name )
        {
#if defined(PARSER_INCLUDE_LOG)
            parser_log(__LINE__, __FUNCTION__, "Invalid string at index: %d", i);
#endif
            return (-1);
        }

        // Return index if match is found.

        if ( !parser_strncmp(name_list[i].name, name_string, PARSER_MAX_NAME_STRING_LENGTH) )
        {
            *index= i;
            return (0);
        }
    }

    *index= -1;

    return (0);
}

// parser_add_new_element

static PARSER_ELEMENT* parser_add_new_element(PARSER_XML*            xml,
                                              const PARSER_XML_NAME* xml_name_list,
                                              PARSER_INT             xml_name_list_length,
                                              PARSER_ELEMENT*        parent_element,
                                              const PARSER_CHAR*     element_name)
{
    PARSER_ELEMENT* child_element;
    PARSER_INT      index;
#if defined(PARSER_WITH_DYNAMIC_NAMES)
    PARSER_INT      length;
#endif
    PARSER_ERROR    error;

    if ( !xml )
    {
#if defined(PARSER_INCLUDE_LOG)
        parser_log(__LINE__, __FUNCTION__, "Parser error: Invalid xml struct pointer");
#endif
        return (0);
    }

    // Allocate memory for element struct.

    child_element= calloc(1, sizeof(PARSER_ELEMENT));
    if ( !child_element )
    {
#if defined(PARSER_INCLUDE_LOG)
        parser_log(__LINE__, __FUNCTION__, "Parser: Out of memory");
#endif
        return (0);
    }

    child_element->content_type=    0;
    child_element->inner_string.first_string= 0;
    child_element->next_element=    0;
    child_element->parent_element=  0;
    child_element->first_attribute= 0;
    child_element->last_attribute=  0;

    // Link inner element.

    if ( parent_element )
    {
        if ( !parent_element->inner_element.first_element )
        {
            parent_element->inner_element.first_element= child_element;
        }

        else if ( !parent_element->inner_element.last_element )
        {
            free(child_element);
#if defined(PARSER_INCLUDE_LOG)
            parser_log(__LINE__, __FUNCTION__, "Parser error: last_element == NULL");
#endif
            return (0);
        }

        else
        {
            parent_element->inner_element.last_element->next_element= child_element;
        }

        // Inherit parent element pointer from previous element.

        parent_element->inner_element.last_element= child_element;
        child_element->parent_element=              parent_element;
    }

    // Link root element.

    else
    {
        if ( !xml->first_element )
            xml->first_element= child_element;

        if ( xml->last_element )
            xml->last_element->next_element= child_element;

        xml->last_element= child_element;
    }

    // Find element name index in xml name list.

    error= find_matching_string_index(element_name, xml_name_list, xml_name_list_length, &index);
    if ( error )
    {
#if defined(PARSER_INCLUDE_LOG)
        parser_log(__LINE__, __FUNCTION__, "Error %d at finding xml name index", error);
#endif
        return (0);
    }

    // Set element name type to xml name list index.

#if defined(PARSER_WITH_DYNAMIC_NAMES)
    if ( index != PARSER_UNKNOWN_INDEX )
    {
#endif
        child_element->elem_name.name_index= index;
        child_element->content_type=         PARSER_ELEMENT_NAME_TYPE_INDEX;

#if defined(PARSER_WITH_DYNAMIC_NAMES)
    }
#endif

    // Otherwise copy full element name.

#if defined(PARSER_WITH_DYNAMIC_NAMES)
    else
    {
        // Get element name length.

        length= parser_strnlen(element_name, PARSER_MAX_NAME_STRING_LENGTH);
        if ( length < 1 )
        {
            child_element->content_type= PARSER_ELEMENT_NAME_TYPE_NONE;
#if defined(PARSER_INCLUDE_LOG)
            parser_log(__LINE__, __FUNCTION__, "Warning: Element name length < 1");
#endif
            return (child_element);
        }

        // Allocate memory for the element name string buffer.

        child_element->elem_name.name_string= malloc(sizeof(PARSER_CHAR) * (length + 1));
        if ( !child_element->elem_name.name_string )
        {
#if defined(PARSER_INCLUDE_LOG)
            parser_log(__LINE__, __FUNCTION__, "Parser: Out of memory");
#endif
            free(child_element);
            return (0);
        }

        // Copy element name to buffer.

        parser_strncpy(element_name, child_element->elem_name.name_string, (length + 1));

        child_element->content_type= PARSER_ELEMENT_NAME_TYPE_STRING;
    }
#endif

    return (child_element);
}

// parser_add_attribute_to_element

static PARSER_ERROR parser_add_attribute_to_element(PARSER_ELEMENT*        parent_element,
                                                    const PARSER_XML_NAME* xml_name_list,
                                                    PARSER_INT             xml_name_list_length,
                                                    const PARSER_CHAR*     attribute_name_string,
                                                    const PARSER_CHAR*     attribute_value_string)
{
    PARSER_ATTRIBUTE* attribute;
    PARSER_ERROR      error;
    PARSER_INT        length;
    PARSER_INT        index;
    PARSER_INT        int_value;
    PARSER_FLOAT      float_value;
    PARSER_INT        n;

    if ( !parent_element || !attribute_name_string || !*attribute_name_string || !attribute_value_string || !*attribute_value_string )
    {
#if defined(PARSER_INCLUDE_LOG)
        parser_log(__LINE__, __FUNCTION__, "Parser error: Invalid parameter");
#endif
        return (PARSER_RESULT_ERROR);
    }

    // Check start/stop indexes.

    length= parser_strnlen(attribute_value_string, PARSER_MAX_VALUE_STRING_LENGTH);
    PARSER_ASSERT(length > 0);

    // Find matching XML name index.

    error= find_matching_string_index(attribute_name_string, xml_name_list, xml_name_list_length, &index);
    PARSER_ASSERT(!error);

    // Allocate memory for element attribute struct.

    attribute= calloc(1, sizeof(PARSER_ATTRIBUTE));
    if ( !attribute )
    {
#if defined(PARSER_INCLUDE_LOG)
        parser_log(__LINE__, __FUNCTION__, "Parser: Out of memory");
#endif
        return (PARSER_RESULT_OUT_OF_MEMORY);
    }

    attribute->attribute_type= PARSER_ATTRIBUTE_VALUE_TYPE_INTEGER;

    // Get value data type.

    for ( n= 0, int_value= 0, float_value= 0; n < length; n++ )
    {
        // Switch data type to float.

        if ( IS_COMMA_CHAR(attribute_value_string[n]) && attribute->attribute_type != PARSER_ATTRIBUTE_VALUE_TYPE_FLOAT )
        {
            attribute->attribute_type= PARSER_ATTRIBUTE_VALUE_TYPE_FLOAT;
            float_value= int_value;
            int_value=   1;
            continue;
        }

        // Switch data type to string.

        else if ( !IS_NUMERIC_CHAR(attribute_value_string[n]) )
        {
            attribute->attribute_type= PARSER_ATTRIBUTE_VALUE_TYPE_STRING;
            break;
        }

        // Parse float.

        if ( attribute->attribute_type == PARSER_ATTRIBUTE_VALUE_TYPE_FLOAT )
        {
            float_value+= (PARSER_FLOAT)((PARSER_FLOAT)(attribute_value_string[n] - '0') / (PARSER_FLOAT)(int_value*10));
            int_value*= 10;
        }

        // Parse integer.

        else
        {
            int_value*= 10;
            int_value+= attribute_value_string[n] - '0';
        }
    }

    // Copy parsed float/integer value to attribute struct.

    if ( attribute->attribute_type == PARSER_ATTRIBUTE_VALUE_TYPE_FLOAT )
    {
        attribute->attr_val.float_value= float_value;
    }

    else if ( attribute->attribute_type == PARSER_ATTRIBUTE_VALUE_TYPE_INTEGER )
    {
        attribute->attr_val.int_value= int_value;
    }

    // Copy full string value.

    else
    {
        // Allocate memory for the value string.

        attribute->attr_val.string_ptr= calloc((length+1), sizeof(PARSER_CHAR));
        if ( !attribute->attr_val.string_ptr )
        {
#if defined(PARSER_INCLUDE_LOG)
            parser_log(__LINE__, __FUNCTION__, "Parser error: Out of memory.");
#endif
            return(PARSER_RESULT_OUT_OF_MEMORY);
        }

        parser_strncpy(attribute_value_string, attribute->attr_val.string_ptr, length+1);
    }

    // Link attribute to parent element.

    if ( !parent_element->first_attribute )
        parent_element->first_attribute= attribute;

    if ( parent_element->last_attribute )
        parent_element->last_attribute->next_attribute= attribute;

    parent_element->last_attribute= attribute;

    // If attribute name is not found in PARSER_XML_NAME -list then
    // allocate memory for the string buffer and copy full attribute name to it.

    if ( index == PARSER_UNKNOWN_INDEX )
    {
        // Get attribute name length.

        length= parser_strnlen(attribute_name_string, PARSER_MAX_NAME_STRING_LENGTH);
        if ( length < 1 )
        {
#if defined(PARSER_INCLUDE_LOG)
            parser_log(__LINE__, __FUNCTION__, "Parser warning: Attribute length < 1.");
#endif
            attribute->attribute_type|= PARSER_ATTRIBUTE_NAME_TYPE_NONE;

            return(0);
        }
        attribute->attribute_type|= PARSER_ATTRIBUTE_NAME_TYPE_STRING;
        attribute->attr_name.name_string= malloc(sizeof(PARSER_CHAR)*(length + 1));
        if ( !attribute->attr_name.name_string )
        {
#if defined(PARSER_INCLUDE_LOG)
            parser_log(__LINE__, __FUNCTION__, "Parser error: Out of memory.");
#endif
            return(PARSER_RESULT_OUT_OF_MEMORY);
        }

        parser_strncpy(attribute_name_string, attribute->attr_name.name_string, length + 1);
    }

    // No need to copy attrbute name string.

    else
    {
        attribute->attribute_type|= PARSER_ATTRIBUTE_NAME_TYPE_INDEX;
        attribute->attr_name.attribute_index= index;
    }

    return (0);
}

// parser_copy_element_content_string

static PARSER_ERROR parser_copy_element_content_string(PARSER_ELEMENT*    owner_element,
                                                       const PARSER_CHAR* buffer)
{
    PARSER_STRING* string;
    PARSER_INT     length;

    PARSER_ASSERT(owner_element);
    PARSER_ASSERT(buffer);

    // Get content string length.

    length= parser_strnlen(buffer, PARSER_MAX_VALUE_STRING_LENGTH);
    PARSER_ASSERT(length > 0);

    // Allocate memory for the string struct.

    string= malloc(sizeof(PARSER_STRING));
    if ( !string )
        return(PARSER_RESULT_OUT_OF_MEMORY);

    // Allocate memory for content string.

    string->buffer= malloc(sizeof(PARSER_CHAR) * (length+1));
    if ( !string->buffer )
    {
        free(string);
#if defined(PARSER_INCLUDE_LOG)
        parser_log(__LINE__, __FUNCTION__, "Parser error: Out of memory.");
#endif
        return(EXIT_FAILURE);
    }

    // Copy element inner content.

    parser_strncpy(buffer, string->buffer, length+1);

    string->buffer_size= length+1;
    string->next_string= 0;

    // Link first string struct to owner element.

    if ( !owner_element->inner_string.first_string )
        owner_element->inner_string.first_string= string;

    if ( owner_element->inner_string.last_string )
        owner_element->inner_string.last_string->next_string= string;

    owner_element->inner_string.last_string= string;

    return (0);
}

// parser_free_element

static PARSER_ERROR parser_free_element(PARSER_ELEMENT* element)
{
    PARSER_ELEMENT*   prev_element;
    PARSER_ATTRIBUTE* attribute;
    PARSER_ATTRIBUTE* prev_attribute;
    PARSER_STRING*    string;
    PARSER_STRING*    prev_string;

    while ( element )
    {
        // Free allocated element name string.

#if defined(PARSER_WITH_DYNAMIC_NAMES)
        if ( element->content_type & PARSER_ELEMENT_NAME_TYPE_STRING && element->elem_name.name_string )
            free(element->elem_name.name_string);
#endif

        // Free inner element structs.

        if ( element->inner_element.first_element )
            parser_free_element(element->inner_element.first_element);

        // Free inner string structs.

        if ( element->inner_string.first_string )
        {
            string= element->inner_string.first_string;
            while ( string )
            {
                prev_string= string;
                string=      string->next_string;

                free(prev_string->buffer);
                free(prev_string);
            }
        }

        // Free attributes.

        if ( element->first_attribute )
        {
            attribute= element->first_attribute;
            while ( attribute )
            {
                // Free attribute name string.

                if ( (attribute->attribute_type & PARSER_ATTRIBUTE_NAME_TYPE_STRING) && attribute->attr_name.name_string  )
                {
                    free(attribute->attr_name.name_string);
                }

                // Free attribute value string.

                if ( (attribute->attribute_type & PARSER_ATTRIBUTE_VALUE_TYPE_STRING) && attribute->attr_val.string_ptr )
                {
                    free(attribute->attr_val.string_ptr);
                }

                prev_attribute= attribute;
                attribute=      attribute->next_attribute;

                free(prev_attribute);
            }
        }

        prev_element= element;
        element=      element->next_element;

        // Free previous element struct.

        free(prev_element);
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
    {
#if defined(PARSER_INCLUDE_LOG)
        parser_log(__LINE__, __FUNCTION__, "Error: attempting to finalize on a null pointer.");
#endif
        return(PARSER_RESULT_ERROR);
    }

    if ( !xml->state )
        return(0);

    // Free xml state.

    free(xml->state);

    xml->state= 0;

    return(0);
}

// parser_free_xml

PARSER_ERROR parser_free_xml(PARSER_XML* xml)
{
    PARSER_ERROR error;

    if ( !xml )
        return(0);

    // Finalize parser state if not finalized yet.

    error= parser_finalize(xml);
    if ( error )
        return(error);

    // Free all buffers.

    error= parser_free_element(xml->first_element);
    return(error);

    // Free xml struct.

    free(xml);

    return(0);
}

// parser_begin
// Returns new xml struct fith initialized parser state.
// XML struct must be freed after it is no longer needed
// anymore with parser_free_xml() function.

PARSER_XML* parser_begin(void)
{
    PARSER_XML* xml;

    // Allocate memory for xml struct.

    xml= calloc(1, sizeof(PARSER_XML));
    if ( !xml )
    {
#if defined(PARSER_INCLUDE_LOG)
        parser_log(__LINE__, __FUNCTION__, "Error: Out of memory while allocating xml struct");
#endif
        return(0);
    }

    // Allocate memory for xml parser state.

    xml->state= calloc(1, sizeof(PARSER_STATE));
    if ( !xml->state )
    {
#if defined(PARSER_INCLUDE_LOG)
        parser_log(__LINE__, __FUNCTION__, "Error: Out of memory while allocating xml parser state");
#endif
        free(xml);
    }

    return(xml);
}

// parser_parse_string

PARSER_ERROR parser_parse_string(PARSER_XML*            xml,
                                 const PARSER_CHAR*     xml_string,
                                 PARSER_INT             xml_string_length,
                                 const PARSER_XML_NAME* xml_name_list,
                                 PARSER_INT             xml_name_list_length)
{
    PARSER_INT   i;
    PARSER_ERROR error;

    // Check input string...

    if ( !xml_string || !*xml_string || xml_string_length < 1 )
    {
#if defined(PARSER_INCLUDE_LOG)
        parser_log(__LINE__, __FUNCTION__, "Error: xml string empty/null");
#endif
        return(PARSER_RESULT_ERROR);
    }

    // ... and xml name list.

    else if ( !xml_name_list || xml_name_list_length < 1 )
    {
#if defined(PARSER_INCLUDE_LOG)
        parser_log(__LINE__, __FUNCTION__, "Error: No xml name count == 0");
#endif
        return(PARSER_RESULT_ERROR);
    }

    if ( !xml )
    {
#if defined(PARSER_INCLUDE_LOG)
        parser_log(__LINE__, __FUNCTION__, "Error: Invalid XML struct.");
#endif
        return(PARSER_RESULT_ERROR);
    }

    else if ( !xml->state )
    {
#if defined(PARSER_INCLUDE_LOG)
        parser_log(__LINE__, __FUNCTION__, "Error: XML struct is finalized.");
#endif
        return(PARSER_RESULT_ERROR);
    }

    // Start parsing.

    for ( i= 0; i < xml_string_length; i++ )
    {
        xml->state->previous_char= xml->state->current_char;
        xml->state->current_char=  xml->state->next_char;
        xml->state->next_char=     i < xml_string_length ? xml_string[i] : '\0';

        // Comment line start.

        if ( !(xml->state->flags & PARSER_STATE_ATTRIBUTE_VALUE_OPEN) && xml->state->current_char == '<' && xml->state->next_char == '!' )
        {
            xml->state->flags|= PARSER_STATE_COMMENT_OPEN;

#if defined(PARSER_DEBUG)
            xml->state->value_buf_pos= 0;
#endif
            continue;
        }

        // Comment line end.

        if ( (xml->state->flags & PARSER_STATE_COMMENT_OPEN) && xml->state->current_char == '-' && xml->state->next_char == '>' )
        {
            // Skip next character.

            xml->state->flags&= ~PARSER_STATE_COMMENT_OPEN;

#if defined(PARSER_DEBUG)
            xml->state->temp_value_buffer[xml->state->value_buf_pos]= '\0';
#if defined(PARSER_INCLUDE_LOG)
            parser_log(__LINE__, __FUNCTION__, "Comment: |%s|\n", xml->state->temp_value_buffer);
#endif
#endif
            continue;
        }

        // Skip comment line chars.

        if ( (xml->state->flags & PARSER_STATE_COMMENT_OPEN) )
        {
#if defined(PARSER_DEBUG)
            xml->state->temp_value_buffer[xml->state->value_buf_pos++]= xml->state->current_char;
#endif
            continue;
        }

        // Store attribute value in to a temporary value buffer.

        if ( (xml->state->flags & PARSER_STATE_ATTRIBUTE_VALUE_OPEN) && !IS_PARENTHESIS(xml->state->current_char) )
        {
            xml->state->temp_value_buffer[xml->state->value_buf_pos++]= xml->state->current_char;
            continue;
        }

        // XML prolog start.

        if ( IS_START_OF_XML_PROLOG(xml->state->current_char, xml->state->next_char) )
        {
            xml->state->flags|= PARSER_STATE_XML_PROLOG_OPEN;
            continue;
        }

        // XML prolog end.

        if ( (xml->state->flags & PARSER_STATE_XML_PROLOG_OPEN) && xml->state->previous_char == '?' && xml->state->current_char == '>' )
        {
            xml->state->flags&= ~PARSER_STATE_XML_PROLOG_OPEN;
            continue;
        }

        // Beginning of an element start tag.

        if ( !(xml->state->flags & PARSER_STATE_ELEMENT_START_TAG_OPEN) && IS_START_OF_ELEMENT_TAG(xml->state->current_char, xml->state->next_char) )
        {
            xml->state->flags|= PARSER_STATE_ELEMENT_START_TAG_OPEN;
            xml->state->flags|= PARSER_STATE_ELEMENT_NAME_OPEN;

            if ( (xml->state->flags & PARSER_STATE_CONTENT_TYPE_STRING) && xml->state->value_buf_pos > 0 )
            {
                xml->state->temp_value_buffer[xml->state->value_buf_pos]= '\0';

                // Copy element content string.

                error= parser_copy_element_content_string(xml->state->element, xml->state->temp_value_buffer);
                if ( error )
                {
#if defined(PARSER_INCLUDE_LOG)
                    parser_log(__LINE__, __FUNCTION__, "Error %d while copying string", error);
#endif
                    return(PARSER_RESULT_ERROR);
                }
            }

            xml->state->name_buf_pos= 0;

            continue;
        }

        // End of element name.

        if ( (xml->state->flags & PARSER_STATE_ELEMENT_NAME_OPEN) && IS_END_OF_ELEMENT_NAME(xml->state->current_char) )
        {
            xml->state->flags&= ~PARSER_STATE_ELEMENT_NAME_OPEN;
            xml->state->temp_name_buffer[xml->state->name_buf_pos]= '\0';

            // Add new element to xml struct.

            xml->state->element= parser_add_new_element(xml, xml_name_list, xml_name_list_length, xml->state->parent_element, xml->state->temp_name_buffer);
            if ( !xml->state->element )
            {
#if defined(PARSER_INCLUDE_LOG)
                parser_log(__LINE__, __FUNCTION__, "Error while inserting new element.");
#endif
                return(PARSER_RESULT_ERROR);
            }
        }

        // Write element name charachter in to a temporary name buffer.

        if ( (xml->state->flags & PARSER_STATE_ELEMENT_NAME_OPEN) && IS_VALID_NAME_CHARACHTER(xml->state->current_char) )
        {
            xml->state->temp_name_buffer[xml->state->name_buf_pos++]= xml->state->current_char;

            continue;
        }

        // End of element start tag.

        if ( (xml->state->flags & PARSER_STATE_ELEMENT_START_TAG_OPEN) && xml->state->current_char == '>' )
        {
            xml->state->flags&= ~PARSER_STATE_ELEMENT_START_TAG_OPEN;
            xml->state->flags&= ~PARSER_STATE_ATTRIBUTE_NAME_OPEN;
            xml->state->flags&= ~PARSER_STATE_CONTENT_TYPE_STRING;

            if ( !xml->state->element )
            {
#if defined(PARSER_INCLUDE_LOG)
                parser_log(__LINE__, __FUNCTION__, "Error: !element");
#endif
                return(PARSER_RESULT_ERROR);
            }

            // Element content start.

            if ( xml->state->element && xml->state->previous_char != '/' )
            {
                xml->state->parent_element= xml->state->element;
                xml->state->value_buf_pos=  0;

                xml->state->flags|= PARSER_STATE_ELEMENT_OPEN;
            }

            continue;
        }

        // Element closing tag beginning.

        if ( (xml->state->flags & PARSER_STATE_ELEMENT_OPEN) && IS_START_OF_ELEMENT_END_TAG(xml->state->current_char, xml->state->next_char) )
        {
            if ( !xml->state->element )
            {
#if defined(PARSER_INCLUDE_LOG)
                parser_log(__LINE__, __FUNCTION__, "Error: !element");
#endif
                return(PARSER_RESULT_ERROR);
            }

            if ( (xml->state->flags & PARSER_STATE_CONTENT_TYPE_STRING) && xml->state->value_buf_pos > 0 )
            {
                xml->state->temp_value_buffer[xml->state->value_buf_pos]= '\0';

                // Copy element content string.

                error= parser_copy_element_content_string(xml->state->element, xml->state->temp_value_buffer);
                if ( error )
                {
#if defined(PARSER_INCLUDE_LOG)
                    parser_log(__LINE__, __FUNCTION__, "Error while inserting new element.");
#endif
                    return(PARSER_RESULT_ERROR);
                }
            }

            xml->state->flags|=  PARSER_STATE_ELEMENT_END_TAG_OPEN;
            xml->state->flags&= ~PARSER_STATE_CONTENT_TYPE_STRING;
            xml->state->name_buf_pos=  0;

            xml->state->element= xml->state->parent_element;
        }

        // Copy content string in to a temporary value buffer.

        else if ( (xml->state->flags & PARSER_STATE_ELEMENT_OPEN) && (xml->state->flags & PARSER_STATE_CONTENT_TYPE_STRING) && !(xml->state->flags & PARSER_STATE_ELEMENT_START_TAG_OPEN) )
        {
            xml->state->temp_value_buffer[xml->state->value_buf_pos++]= xml->state->current_char;
        }

        // Set element content type to string unless child element starting tag occurs later.

        else if ( (xml->state->flags & PARSER_STATE_ELEMENT_OPEN)           &&
                 !(xml->state->flags & PARSER_STATE_CONTENT_TYPE_STRING)    &&
                 !(xml->state->flags & PARSER_STATE_ELEMENT_START_TAG_OPEN) &&
                 !(xml->state->flags & PARSER_STATE_ELEMENT_END_TAG_OPEN)   && (IS_ALPHA_CHAR(xml->state->current_char) || IS_NUMERIC_CHAR(xml->state->current_char)) )
        {
            xml->state->flags|= PARSER_STATE_CONTENT_TYPE_STRING;
            xml->state->value_buf_pos= 0;
            xml->state->temp_value_buffer[xml->state->value_buf_pos++]= xml->state->current_char;
        }

        // Element closing tag end.

        if ( (xml->state->flags & PARSER_STATE_ELEMENT_END_TAG_OPEN) && xml->state->current_char == '>' )
        {
            xml->state->flags&= ~PARSER_STATE_ELEMENT_END_TAG_OPEN;

            if ( xml->state->element->parent_element )
            {
                xml->state->element=        xml->state->element->parent_element;
                xml->state->parent_element= xml->state->element;
            }

            else
            {
                xml->state->parent_element= 0;
                xml->state->flags&= ~PARSER_STATE_ELEMENT_OPEN;
            }
        }

        // Start of attribute name.

        if ( !(xml->state->flags & PARSER_STATE_ATTRIBUTE_NAME_OPEN)    &&
             !(xml->state->flags & PARSER_STATE_ATTRIBUTE_VALUE_OPEN)   &&
             ((xml->state->flags & PARSER_STATE_ELEMENT_START_TAG_OPEN) ||
             (xml->state->flags & PARSER_STATE_XML_PROLOG_OPEN))        &&
             IS_START_OF_ATTRIBUTE_NAME(xml->state->current_char, xml->state->next_char) )
        {
            xml->state->flags|= PARSER_STATE_ATTRIBUTE_NAME_OPEN;
            xml->state->name_buf_pos= 0;

            continue;
        }

        // End of attribute name.

        if ( (xml->state->flags & PARSER_STATE_ATTRIBUTE_NAME_OPEN) && IS_END_OF_ATTRIBUTE_NAME(xml->state->current_char) )
        {
            xml->state->flags&= ~PARSER_STATE_ATTRIBUTE_NAME_OPEN;
            xml->state->temp_name_buffer[xml->state->name_buf_pos]= '\0';

            continue;
        }

        // Store attribute name in to a temporary name buffer.

        if ( (xml->state->flags & PARSER_STATE_ATTRIBUTE_NAME_OPEN) )
        {
            xml->state->temp_name_buffer[xml->state->name_buf_pos++]= xml->state->current_char;

            continue;
        }

        // Attribute value start/end.

        if ( ((xml->state->flags & PARSER_STATE_ELEMENT_START_TAG_OPEN) || (xml->state->flags & PARSER_STATE_XML_PROLOG_OPEN)) && IS_PARENTHESIS(xml->state->current_char) )
        {
            // Attribute value start.

            if ( !(xml->state->flags & PARSER_STATE_ATTRIBUTE_VALUE_OPEN) )
            {
                // By default expect attribute value to be integer.

                xml->state->flags|= PARSER_STATE_PARSE_INT_VALUE;
                xml->state->flags|= PARSER_STATE_ATTRIBUTE_VALUE_OPEN;

                xml->state->value_buf_pos= 0;
            }

            // Attribute value end.

            else if ( (xml->state->flags & PARSER_STATE_ELEMENT_START_TAG_OPEN) )
            {
                xml->state->flags&= ~PARSER_STATE_ATTRIBUTE_VALUE_OPEN;
                xml->state->flags&= ~PARSER_STATE_PARSE_INT_VALUE;
                xml->state->flags&= ~PARSER_STATE_PARSE_FLOAT_VALUE;
                xml->state->flags&= ~PARSER_STATE_PARSE_STRING_VALUE;

                xml->state->temp_value_buffer[xml->state->value_buf_pos]= '\0';

                // Link attribute to parent element.

                error= parser_add_attribute_to_element(xml->state->element, xml_name_list, xml_name_list_length, xml->state->temp_name_buffer, xml->state->temp_value_buffer);
                if ( error )
                {
#if defined(PARSER_INCLUDE_LOG)
                    parser_log(__LINE__, __FUNCTION__, "Error while inserting new element.");
#endif
                    return(PARSER_RESULT_ERROR);
                }
            }

            // XML prolog attribute end.

            else if ( (xml->state->flags & PARSER_STATE_XML_PROLOG_OPEN) )
            {
                xml->state->flags&= ~PARSER_STATE_ATTRIBUTE_VALUE_OPEN;
                xml->state->temp_value_buffer[xml->state->value_buf_pos]= '\0';

#if defined(PARSER_INCLUDE_LOG)
                parser_log(__LINE__, __FUNCTION__, "XML prolog: %s=\"%s\"\n", xml->state->temp_name_buffer, xml->state->temp_value_buffer);
#endif
            }
        }

    }

    return (0);
}

#if defined(PARSER_CONFIG_INCLUDE_XML_WRITE)

// buffer_append_int

static inline void buffer_append_int(PARSER_INT   value,
                                     PARSER_CHAR* buffer,
                                     PARSER_INT   buffer_size,
                                     PARSER_INT*  bytes_written)
{
    PARSER_INT n;
    PARSER_INT i;

    if ( value == 0 && (*bytes_written) < (buffer_size-1) )
    {
        buffer[(*bytes_written)]= '0';
        (*bytes_written)++;

        return;
    }

    for ( i = 0, n = (log10(value) + 1); i < n && (*bytes_written) < (buffer_size-1); ++i, value /= 10 )
    {
        buffer[(*bytes_written)]= (value % 10) + '0';
        (*bytes_written)++;
    }
}

// buffer_append_float

static inline void buffer_append_float(PARSER_FLOAT value,
                                       PARSER_CHAR* buffer,
                                       PARSER_INT   buffer_size,
                                       PARSER_INT*  bytes_written)
{
    PARSER_INT n;
    PARSER_INT i;

    n= log10(value) + 1;

    for ( i = 0; i < n && (*bytes_written) < (buffer_size-1) ; ++i, value /= 10.0f )
    {
        buffer[(*bytes_written)]= (value / 10.0f) + '0';
        (*bytes_written)++;
    }
}

// buffer_append_string

static void buffer_append_string(const PARSER_CHAR* str,
                                 PARSER_CHAR*       buffer,
                                 PARSER_INT         buffer_size,
                                 PARSER_INT*        bytes_written)
{

    while ( *str != '\0' && (*bytes_written) < (buffer_size-1) )
    {
        buffer[(*bytes_written)]= *str++;
        (*bytes_written)++;
    }
}

// print_attributes

static void print_attributes(PARSER_ATTRIBUTE*      attribute,
                             const PARSER_XML_NAME* xml_name_list,
                             PARSER_CHAR*           buffer,
                             PARSER_INT             buffer_size,
                             PARSER_INT*            bytes_written)
{
    while ( attribute && (*bytes_written) < (buffer_size-1) )
    {
        // Print attribute name.

        if ( (attribute->attribute_type & PARSER_ATTRIBUTE_NAME_TYPE_INDEX) && attribute->attr_name.attribute_index != PARSER_UNKNOWN_INDEX && (*bytes_written) < (buffer_size-1) )
        {
#if defined(PARSER_CONFIG_INCLUDE_STDIO_LIB)
            (*bytes_written)+= snprintf(&buffer[(*bytes_written)], (buffer_size-(*bytes_written))," %s=", xml_name_list[attribute->attr_name.attribute_index].name);
#else
            buffer_append_string(" ", buffer, buffer_size, bytes_written);
            buffer_append_string(xml_name_list[attribute->attr_name.attribute_index].name, buffer, buffer_size, bytes_written);
            buffer_append_string("=", buffer, buffer_size, bytes_written);
#endif
        }

        else if ( (attribute->attribute_type & PARSER_ATTRIBUTE_NAME_TYPE_STRING) && attribute->attr_name.name_string && *attribute->attr_name.name_string && (*bytes_written) < (buffer_size-1) )
        {
#if defined(PARSER_CONFIG_INCLUDE_STDIO_LIB)
            (*bytes_written)+= snprintf(&buffer[(*bytes_written)], (buffer_size-(*bytes_written))," %s=", attribute->attr_name.name_string);
#else
            buffer_append_string(" ", buffer, buffer_size, bytes_written);
            buffer_append_string(attribute->attr_name.name_string, buffer, buffer_size, bytes_written);
            buffer_append_string("=", buffer, buffer_size, bytes_written);
#endif
        }

        else if ( (attribute->attribute_type & PARSER_ATTRIBUTE_NAME_TYPE_NONE) && (*bytes_written) < (buffer_size-1) )
        {
#if defined(PARSER_CONFIG_INCLUDE_STDIO_LIB)
            (*bytes_written)+= snprintf(&buffer[(*bytes_written)], (buffer_size-(*bytes_written))," null=");
#else
            buffer_append_string(" null=", buffer, buffer_size, bytes_written);
#endif
        }

        // Print attribute value.

        if ( (attribute->attribute_type & PARSER_ATTRIBUTE_VALUE_TYPE_STRING) && attribute->attr_val.string_ptr && *attribute->attr_val.string_ptr && (*bytes_written) < (buffer_size-1) )
        {
#if defined(PARSER_CONFIG_INCLUDE_STDIO_LIB)
            (*bytes_written)+= snprintf(&buffer[(*bytes_written)], (buffer_size-(*bytes_written)),"\"%s\"", attribute->attr_val.string_ptr);
#else
            buffer_append_string("\"", buffer, buffer_size, bytes_written);
            buffer_append_string(attribute->attr_val.string_ptr, buffer, buffer_size, bytes_written);
            buffer_append_string("\"", buffer, buffer_size, bytes_written);
#endif
        }

        else if ( (attribute->attribute_type & PARSER_ATTRIBUTE_VALUE_TYPE_INTEGER) && (*bytes_written) < (buffer_size-1) )
        {
#if defined(PARSER_CONFIG_INCLUDE_STDIO_LIB)
            (*bytes_written)+= snprintf(&buffer[(*bytes_written)], (buffer_size-(*bytes_written)),"\"%d\"", attribute->attr_val.int_value);
#else
            buffer_append_string("\"", buffer, buffer_size, bytes_written);
            buffer_append_int(attribute->attr_val.int_value, buffer, buffer_size, bytes_written);
            buffer_append_string("\"", buffer, buffer_size, bytes_written);
#endif
        }

        else if ( (attribute->attribute_type & PARSER_ATTRIBUTE_VALUE_TYPE_FLOAT) && (*bytes_written) < (buffer_size-1) )
        {
#if defined(PARSER_CONFIG_INCLUDE_STDIO_LIB)
            (*bytes_written)+= snprintf(&buffer[(*bytes_written)], (buffer_size-(*bytes_written)),"\"%f\"", attribute->attr_val.float_value);
#else
            buffer_append_string("\"", buffer, buffer_size, bytes_written);
            buffer_append_float(attribute->attr_val.float_value, buffer, buffer_size, bytes_written);
            buffer_append_string("\"", buffer, buffer_size, bytes_written);
#endif
        }

        else if ( (*bytes_written) < (buffer_size-1) )
        {
#if defined(PARSER_CONFIG_INCLUDE_STDIO_LIB)
            (*bytes_written)+= snprintf(&buffer[(*bytes_written)], (buffer_size-(*bytes_written)),"\"null\"");
#else
            buffer_append_string("\"null\"", buffer, buffer_size, bytes_written);
#endif
        }

        attribute= attribute->next_attribute;
    }
}

// print_inner_strings

static void print_inner_strings(PARSER_STRING* string,
                                PARSER_CHAR*   buffer,
                                PARSER_INT     buffer_size,
                                PARSER_INT*    bytes_written,
                                PARSER_INT     depth)
{
    while ( string && string->buffer && (*bytes_written) < (buffer_size-1) )
    {
        if ( *string->buffer && (*bytes_written) < (buffer_size-1) )
        {
#if defined(PARSER_CONFIG_INCLUDE_STDIO_LIB)
            (*bytes_written)+= snprintf(&buffer[(*bytes_written)], (buffer_size-(*bytes_written)),"%s", string->buffer);
#else
            buffer_append_string(string->buffer, buffer, buffer_size, bytes_written);
#endif
        }

        else if ( (*bytes_written) < (buffer_size-1) )
        {
#if defined(PARSER_CONFIG_INCLUDE_STDIO_LIB)
            (*bytes_written)+= snprintf(&buffer[(*bytes_written)], (buffer_size-(*bytes_written)),"null");
#else
            buffer_append_string("null", buffer, buffer_size, bytes_written);
#endif
        }

        string= string->next_string;
    }
}

// parser_write_xml_element_to_buffer

static PARSER_INT parser_write_xml_element_to_buffer(PARSER_ELEMENT*        element,
                                                     const PARSER_XML_NAME* xml_name_list,
                                                     PARSER_CHAR*           buffer,
                                                     PARSER_INT             buffer_size,
                                                     PARSER_INT*            bytes_written,
                                                     PARSER_INT             depth,
                                                     PARSER_INT             flags)
{

    while ( element && (*bytes_written) < (buffer_size-1) )
    {
#if defined(PARSER_CONFIG_INCLUDE_STDIO_LIB)
        (*bytes_written)+= snprintf(&buffer[(*bytes_written)], (buffer_size-(*bytes_written)),"%*s", depth*2, "");
#endif
        if ( (element->content_type & PARSER_ELEMENT_NAME_TYPE_INDEX) && element->elem_name.name_index != PARSER_UNKNOWN_INDEX && (*bytes_written) < (buffer_size-1) )
        {
#if defined(PARSER_CONFIG_INCLUDE_STDIO_LIB)
            (*bytes_written)+= snprintf(&buffer[(*bytes_written)], (buffer_size-(*bytes_written)),"<%s", xml_name_list[element->elem_name.name_index].name);
#else
            buffer_append_string("<", buffer, buffer_size, bytes_written);
            buffer_append_string(xml_name_list[element->elem_name.name_index].name, buffer, buffer_size, bytes_written);
#endif
        }
#if defined(PARSER_WITH_DYNAMIC_NAMES)
        else if ( (element->content_type & PARSER_ELEMENT_NAME_TYPE_STRING) && element->elem_name.name_string && (*bytes_written) < (buffer_size-1) )
        {
#if defined(PARSER_CONFIG_INCLUDE_STDIO_LIB)
            (*bytes_written)+= snprintf(&buffer[(*bytes_written)], (buffer_size-(*bytes_written)),"<%s", element->elem_name.name_string);
#else
            buffer_append_string("<", buffer, buffer_size, bytes_written);
            buffer_append_string(element->elem_name.name_string, buffer, buffer_size, bytes_written);
#endif
        }
#endif
        else if ( (*bytes_written) < (buffer_size-1) )
        {
#if defined(PARSER_CONFIG_INCLUDE_STDIO_LIB)
            (*bytes_written)+= snprintf(&buffer[(*bytes_written)], (buffer_size-(*bytes_written)),"<null");
#else
            buffer_append_string("<null ", buffer, buffer_size, bytes_written);
#endif
        }

        if ( element->first_attribute )
        {
            print_attributes(element->first_attribute, xml_name_list, buffer, buffer_size, bytes_written);
        }

        if ( (element->inner_element.first_element || element->inner_string.first_string) && (*bytes_written) < (buffer_size-1) )
        {
#if defined(PARSER_CONFIG_INCLUDE_STDIO_LIB)
            (*bytes_written)+= snprintf(&buffer[(*bytes_written)], (buffer_size-(*bytes_written)),">");
#else
            buffer_append_string(">", buffer, buffer_size, bytes_written);
#endif
        }

        if ( element->inner_element.first_element && (*bytes_written) < (buffer_size-1) )
        {
#if defined(PARSER_CONFIG_INCLUDE_STDIO_LIB)
            (*bytes_written)+= snprintf(&buffer[(*bytes_written)], (buffer_size-(*bytes_written)),"\n");
#endif
            parser_write_xml_element_to_buffer(element->inner_element.first_element, xml_name_list, buffer, buffer_size, bytes_written, depth+1, 0);
#if defined(PARSER_CONFIG_INCLUDE_STDIO_LIB)
            (*bytes_written)+= snprintf(&buffer[(*bytes_written)], (buffer_size-(*bytes_written)),"%*s", depth*2, "");
#endif
        }

        if ( element->inner_string.first_string && (*bytes_written) < (buffer_size-1)  )
        {
            print_inner_strings(element->inner_string.first_string, buffer, buffer_size, bytes_written, depth+1);
        }

        if ( !element->inner_string.first_string && !element->inner_element.first_element && (*bytes_written) < (buffer_size-1) )
        {
#if defined(PARSER_CONFIG_INCLUDE_STDIO_LIB)
            (*bytes_written)+= snprintf(&buffer[(*bytes_written)], (buffer_size-(*bytes_written)),"/>\n");
#else
            buffer_append_string("/>", buffer, buffer_size, bytes_written);
#endif
        }

        else if ( (element->content_type & PARSER_ELEMENT_NAME_TYPE_INDEX) && element->elem_name.name_index != PARSER_UNKNOWN_INDEX && (*bytes_written) < (buffer_size-1) )
        {
#if defined(PARSER_CONFIG_INCLUDE_STDIO_LIB)
            (*bytes_written)+= snprintf(&buffer[(*bytes_written)], (buffer_size-(*bytes_written)),"</%s>\n", xml_name_list[element->elem_name.name_index].name);
#else
            buffer_append_string("</", buffer, buffer_size, bytes_written);
            buffer_append_string(xml_name_list[element->elem_name.name_index].name, buffer, buffer_size, bytes_written);
            buffer_append_string(">", buffer, buffer_size, bytes_written);
#endif
        }
#if defined(PARSER_WITH_DYNAMIC_NAMES)
        else if ( (element->content_type & PARSER_ELEMENT_NAME_TYPE_STRING) && element->elem_name.name_string && (*bytes_written) < (buffer_size-1) )
        {
#if defined(PARSER_CONFIG_INCLUDE_STDIO_LIB)
            (*bytes_written)+= snprintf(&buffer[(*bytes_written)], (buffer_size-(*bytes_written)),"</%s>\n", element->elem_name.name_string);
#else
            buffer_append_string("</", buffer, buffer_size, bytes_written);
            buffer_append_string(element->elem_name.name_string, buffer, buffer_size, bytes_written);
            buffer_append_string(">", buffer, buffer_size, bytes_written);
#endif
        }
#endif
        else if ( (*bytes_written) < (buffer_size-1) )
        {
#if defined(PARSER_CONFIG_INCLUDE_STDIO_LIB)
            (*bytes_written)+= snprintf(&buffer[(*bytes_written)], (buffer_size-(*bytes_written)),"</null>\n");
#else
            buffer_append_string("</null>", buffer, buffer_size, bytes_written);
#endif
        }

        element= element->next_element;
    }

    return (0);
}

// parser_write_xml__to_buffer

PARSER_ERROR parser_write_xml_to_buffer(const PARSER_XML*      xml,
                                        const PARSER_XML_NAME* xml_name_list,
                                        PARSER_CHAR*           buffer,
                                        PARSER_INT             buffer_size,
                                        PARSER_INT*            bytes_written,
                                        PARSER_INT             flags)
{
    if ( !xml )
    {
#if defined(PARSER_INCLUDE_LOG)
        parser_log(__LINE__, __FUNCTION__, "Error: Invalid XML struct.");
#endif
        return(PARSER_RESULT_ERROR);
    }

    else if ( !buffer || buffer_size < 1 )
    {
#if defined(PARSER_INCLUDE_LOG)
        parser_log(__LINE__, __FUNCTION__, "Error: Invalid output buffer.");
#endif
        return(PARSER_RESULT_ERROR);
    }

    (*bytes_written)= 0;
    if ( !xml->first_element )
        return(0);

    parser_write_xml_element_to_buffer(xml->first_element, xml_name_list, buffer, buffer_size, bytes_written, 0, flags);

    // Make sure buffer is ended with \0.

    buffer[(*bytes_written) > buffer_size? buffer_size-1 : (*bytes_written)]= '\0';

    return (0);
}
#endif

// parser_find_element
// Returns first element after offset with matching element name.

const PARSER_ELEMENT* parser_find_element(const PARSER_XML*      xml,
                                          const PARSER_XML_NAME* xml_name_list,
                                          const PARSER_ELEMENT*  offset,
                                          PARSER_INT             max_depth,
                                          PARSER_INT             xml_name_list_length,
                                          const PARSER_CHAR*     element_name)
{
    const PARSER_ELEMENT* element;
    PARSER_INT            depth;

    if ( !xml )
    {
#if defined(PARSER_INCLUDE_LOG)
        parser_log(__LINE__, __FUNCTION__, " Error: Inavlid XML-pointer.");
#endif
        return(0);
    }

#if !defined(PARSER_WITH_DYNAMIC_NAMES)
    if ( !xml_name_list || xml_name_list_length < 1 )
    {
#if defined(PARSER_INCLUDE_LOG)
        parser_log(__LINE__, __FUNCTION__, " Error: XML-name list is empty.");
#endif
        return(0);
    }
#endif

    if ( !element_name || !*element_name )
    {
#if defined(PARSER_INCLUDE_LOG)
        parser_log(__LINE__, __FUNCTION__, " Error: Inavlid element name.");
#endif
        return(0);
    }

    if ( offset )
        element= offset;
    else
        element= xml->first_element;

    depth= 0;
    while ( element && depth <= max_depth )
    {
        // Return current element if name matches.

        if ( (element->content_type & PARSER_ELEMENT_NAME_TYPE_INDEX) && element->elem_name.name_index != PARSER_UNKNOWN_INDEX && element->elem_name.name_index < xml_name_list_length )
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

        if ( element->inner_element.first_element && depth < max_depth)
        {
            element= element= element->inner_element.first_element;
            depth++;
        }

        // If no more elements are available depth > 0 then move to parent element.

        else if ( !element->next_element && depth > 0 && !element->inner_element.first_element )
        {
            // Iterate until parent element with next element is found.

            while ( depth > 0 && element && element->parent_element )
            {
                depth--;

                if ( element->parent_element->next_element )
                {
                    element= element->parent_element->next_element;
                    break;
                }

                else
                {
                    element= element->parent_element;
                }
            }
        }

        // Move to next element in linked list.

        else
        {
            element= element->next_element;
        }

    }

    return(0);
}
