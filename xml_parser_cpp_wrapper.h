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

#ifndef xml_parser_cpp_wrapper_h
#define xml_parser_cpp_wrapper_h

#ifdef __cplusplus

#include <optional>
extern "C" {
#include "xml_parser.h"
}

namespace xml_parser
{

enum class AttributeType
{
    kInteger,
    kFloat,
    kString,
    kUnknown
};

using Element = const PARSER_ELEMENT;
using Attribute = const PARSER_ATTRIBUTE;

class Parser
{
    public:

    Parser(const PARSER_XML_NAME* element_name_list,
           PARSER_INT             element_name_list_length,
           const PARSER_XML_NAME* attribute_name_list,
           PARSER_INT             attribute_name_list_length);

    ~Parser();

    void Append(const PARSER_CHAR* xml_string,
                PARSER_INT         xml_string_length) const;

    Element* FindElement(Element*           offset,
                         PARSER_INT         max_depth,
                         const PARSER_CHAR* element_name) const;

    static Element* GetChildElement(Element* parent);

    static Element* GetNextElement(Element* element);

    Attribute* FindElementAttribute(Element*           element,
                                    Attribute*         offset,
                                    const PARSER_CHAR* attribute_name);

    static AttributeType GetAttributeType(Attribute* attribute);

    static std::optional<std::uint32_t> GetAttributeValue(Attribute* attribute);

    private:

    PARSER_XML* xml_;
};

} // xml_parser

#endif /* __cplusplus */
#endif /* xml_parser_cpp_wrapper_h */
