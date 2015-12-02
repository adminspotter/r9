#include "../client/texture.h"

#include <gtest/gtest.h>

#define FILE_NAME  "t_texture_parser.xml"

TEST(TextureParserTest, DTDSearchPath)
{
    texture *tex = new texture;
    XNS::SAXParser *parser = NULL;
    TextureParser *texparse = NULL;

    XNS::XMLPlatformUtils::Initialize();
    parser = new XNS::SAXParser();
    parser->setValidationScheme(XNS::SAXParser::Val_Auto);
    parser->setValidationSchemaFullChecking(true);
    parser->setDoNamespaces(true);
    texparse = new TextureParser(tex);
    texparse->dtd_path = DTD_PATH;
    texparse->dtd_name = "texture.dtd";
    parser->setDocumentHandler((XNS::DocumentHandler *)texparse);
    parser->setErrorHandler((XNS::ErrorHandler *)texparse);
    parser->setEntityResolver((R9Resolver *)texparse);
    ASSERT_NO_THROW(
        {
            parser->parse(FILE_NAME);
        });

    delete texparse;
    delete parser;
    delete tex;
}
