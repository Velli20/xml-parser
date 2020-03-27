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

#include <xml_parser_cpp_wrapper.h>

namespace xml_parser
{

Parser::Parser(const PARSER_XML_NAME* element_name_list,
               PARSER_INT             element_name_list_length,
               const PARSER_XML_NAME* attribute_name_list,
               PARSER_INT             attribute_name_list_length)
{
    xml_ = parser_begin(element_name_list,
                        element_name_list_length,
                        attribute_name_list,
                        attribute_name_list_length);
}

Parser::~Parser()
{
    if ( !xml_ )
        return;

    parser_free_xml(xml_);
}

void Parser::Append(const PARSER_CHAR* xml_string,
                    PARSER_INT         xml_string_length) const
{
    parser_append(xml_, xml_string, xml_string_length);
}

Element* Parser::FindElement(Element*           offset,
                             PARSER_INT         max_depth,
                             const PARSER_CHAR* element_name) const
{
    return(parser_find_element(xml_, offset, max_depth, element_name));
}

Element* Parser::GetChildElement(Element* parent)
{
    if ( !parent )
        return(nullptr);

    return(parser_get_first_child_element(parent));
}

Element* Parser::GetNextElement(Element* element)
{
    if ( !element )
        return(nullptr);

    return(parser_get_next_element(element));
}

Attribute* Parser::FindElementAttribute(Element*           element,
                                        Attribute*         offset,
                                        const PARSER_CHAR* attribute_name)
{
    return(parser_find_attribute(xml_, element, offset, attribute_name));
}

AttributeType Parser::GetAttributeType(Attribute* attribute)
{
    if ( !attribute )
        return(AttributeType::kUnknown);

    PARSER_ATTRIBUTE_TYPE type = parser_get_attribute_type(attribute);

    if ( type == ATTRIBUTE_TYPE_INTEGER )
        return(AttributeType::kInteger);

    if ( type == ATTRIBUTE_TYPE_FLOAT )
        return(AttributeType::kFloat);

    if ( type == ATTRIBUTE_TYPE_STRING )
        return(AttributeType::kString);

    return(AttributeType::kUnknown);
}

std::optional<std::uint32_t> Parser::GetAttributeValue(Attribute* attribute)
{
    if ( !attribute || parser_get_attribute_type(attribute) != ATTRIBUTE_TYPE_INTEGER )
        return{};

    std::uint32_t value = static_cast<std::uint32_t>(attribute->attr_val.int_value);

    return(value);
}

} // xml_parser
