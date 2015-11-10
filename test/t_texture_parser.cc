#include "../client/texture.h"

#include <gtest/gtest.h>

#define FILE_NAME  "t_texture_parser.xml"

TEST(TextureParserTest, DTDSearchPath)
{
    texture *tex = new texture;
    XNS::SAXParser *parser = NULL;
    TextureParser *texparse = NULL;

    XNS::XMLPlatformUtils::Initialize();
    parser->setValidationScheme(XNS::SAXParser::Val_Auto);
    parser->setValidationSchemaFullChecking(true);
    parser->setDoNamespaces(true);
    texparse = new TextureParser(tex);
    texparse->dtd_path = XNS::XMLString::transcode("../client");
    parser->setDocumentHandler((XNS::DocumentHandler *)texparse);
    parser->setErrorHandler((XNS::ErrorHandler *)texparse);
    parser->setEntityResolver((R9Resolver *)texparse);
    parser->parse(FILE_NAME);
}
