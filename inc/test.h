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

#ifndef TEST_H
#define TEST_H

static const char test_1[]= 
{
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"\n"
"<ELEM_type_1 testElementId='0' intAttribute='20' floatAttribute='1.23' stringAttribute=\"TEST\">\n"
"\n"
"    <!-- Single line comment -->\n"
"    <ELEM_type_2 testElementId='1'>\n"
"        <ELEM_type_3 testElementId='11'/>\n"
"        <ELEM_type_3 testElementId='12'/>\n"
"        <ELEM_type_3 testElementId='13'/>\n"
"\n"
"        <!-- Single line comment -->\n"
"        <ELEM_type_3 testElementId='14'>\n"
"            <!-- Multiline\n"
"            comment -->\n"
"        </ELEM_type_3>\n"
"    </ELEM_type_2>\n"
"\n"
"    <ELEM_type_2 testElementId='2'>Child element with string content</ELEM_type_2>\n"
"</ELEM_type_1>\n"
""
};

#endif
