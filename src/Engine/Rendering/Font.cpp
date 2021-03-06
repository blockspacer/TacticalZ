#include "Rendering/Font.h"


Font::Font(std::string path)
{
    typedef boost::tokenizer<boost::char_separator<char>> tokenizer;
    boost::char_separator<char> sep(",");
    tokenizer tok(path, sep);


    tokenizer::iterator it = tok.begin();
    std::string filePath = "";

    if (it != tok.end()) {
        filePath = (*it).c_str();
        it++;
        if (it != tok.end()) {
            if((*it).c_str() == "") {
                throw std::runtime_error("");
            }

            try {
                FontSize = boost::lexical_cast<int>((*it).c_str());
            } catch (boost::bad_lexical_cast const&) {
                LOG_ERROR("input string did not have a valid font resolution");
                throw std::runtime_error("");
            }
        }
    } else {
        throw std::runtime_error("");;
    }


    FT_Library library;
    FT_Face face;

    if (FT_Init_FreeType(&library)) {
        LOG_ERROR("FreeType error: init failed");
        throw std::runtime_error("");;
    }

    if (FT_New_Face(library, filePath.c_str(), 0, &face)) {
        LOG_ERROR("FreeType error: loading font");
        throw std::runtime_error("");;
    }

    FT_Set_Char_Size(face, 0, FontSize*64, 300, 300); // temp
    FT_Set_Pixel_Sizes(face, 0, FontSize);            //

    if (FT_Load_Char(face, 'X', FT_LOAD_RENDER)) {
        LOG_ERROR("FreeType error: loading char");
        throw std::runtime_error("");;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (GLubyte c = 0; c < 128; c++) {
       

        //Load character glyph
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            continue;
        }

        //Generate texture
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
            );
        // Set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // Now store character for later use
        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            face->glyph->advance.x
        };

        m_Characters.insert(std::pair<GLchar, Character>(c, character));
    }

    
    FT_Done_Face(face);
    FT_Done_FreeType(library);
    GLERROR("Font Load");
}

Font::~Font()
{
    for (auto c : m_Characters) {
        glDeleteTextures(1, &c.second.TextureID);
    }
}
