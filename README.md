# XML-parser
Work in progress. Lightweight xml-parser with minimal system requirements.


# Usage

Example program:

```c
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

// Test string to parse

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

int main(void)
{
    PARSER_XML*  xml;
    PARSER_CHAR  out[1024];
    PARSER_INT   bw;
    PARSER_ERROR error;

    // Initialize xml-struct.

    xml= parser_begin();
    if ( !xml )
    {
        printf("%s %d: XML-struct initialization failed.\n", __FUNCTION__, __LINE__);
        return(1);
    }

    // Parse xml-string.

    error= parser_parse_string(xml, test_1, elements, sizeof(elements)/sizeof(PARSER_XML_NAME));
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
```