# XML-parser

Work in progress. Lightweight xml-parser for embedded systems.

# Usage

Example of parsing string:

```c
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

// String to parse.

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
```