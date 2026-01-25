#include "font.hpp"

#include <string.h>

#include <cctype>
#include <codecvt>
#include <cstdint>
#include <format>
#include <locale>
#include <stdexcept>
#include <string>

#include "filesystem.hpp"
#include "freetype/freetype.h"
#include "gfx/base_types.hpp"
#include "gfx/engine.hpp"
#include "gfx/gui/ngui.hpp"
#include "logging.hpp"
namespace rdm::gfx::gui {
glm::ivec2 Font::getTextSize(const char* text) {
  glm::ivec2 p;
  glm::ivec2 s;
  p.y = maxLineHeight;
  while (*text != 0) {
    if (*text == '\n') {
      p.y += maxLineHeight;
      p.x = 0;
    } else {
      uint32_t glyphIndex = FT_Get_Char_Index(face, *text);

      Chara c = glyphMap[glyphIndex];
      p.x += c.advance.x;
    }

    s = glm::max(p, s);
    text++;
  }

  return p;
}

bool Font::renderGlyph(uint32_t glyph) {
  if (glyphMap.find(glyph) != glyphMap.end()) return false;

  FT_Error error = FT_Load_Glyph(face, glyph, FT_LOAD_DEFAULT);
  if (face->glyph->format != FT_GLYPH_FORMAT_BITMAP) {
    error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
  }

  Chara chara;
  chara.height = face->glyph->bitmap.rows;
  chara.width = face->glyph->bitmap.width;
  chara.pitch = face->glyph->bitmap.pitch;
  chara.advance =
      glm::ivec2(face->glyph->advance.x >> 6, face->glyph->advance.y >> 6);
  chara.bearing = glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top);
  maxLineHeight = std::max(chara.height, maxLineHeight);
  size_t bufSize = chara.width * chara.height;
  chara.pixData = (unsigned char*)malloc(chara.width * bufSize);
  memcpy(chara.pixData, face->glyph->bitmap.buffer, bufSize);
  glyphMap[glyph] = chara;
  needsAtlasRender = true;

  return true;
}

#define FONT_ATLAS_MAX_TEXTURE_SIZE 512
void Font::renderAtlas() {
  atlasLayout.clear();

  // uv layouts in atlasLayout are not normalized
  glm::vec2 uvBegin(0);
  glm::vec2 uvRowSize(0);
  glm::ivec2 atlasSize(0);

  for (auto pair : glyphMap) {
    glm::vec2 uvSize(pair.second.width, pair.second.height);
    uvRowSize = glm::max(uvRowSize, uvSize);

    CharaLayout cl;
    cl.uvBegin = uvBegin;
    cl.uvSize = uvSize;

    int advance = std::min(pair.second.advance.x, (int)uvSize.x);

    atlasSize =
        glm::max(glm::ivec2(uvSize + uvBegin) + glm::ivec2(10), atlasSize);
    if (uvBegin.x + advance >= FONT_ATLAS_MAX_TEXTURE_SIZE) {
      uvBegin.x = 0;
      uvBegin.y += uvRowSize.y;
    } else {
      uvBegin.x += advance;
    }
    if (uvBegin.y + uvSize.y >= FONT_ATLAS_MAX_TEXTURE_SIZE) {
      throw std::runtime_error("Run out of space for font atlas");
    }
    atlasLayout[pair.first] = cl;
  }

  Log::printf(LOG_DEBUG, "%ix%i", atlasSize.x, atlasSize.y);
  unsigned char* imgBuffer = (unsigned char*)malloc(atlasSize.x * atlasSize.y);
  memset(imgBuffer, 0, atlasSize.x * atlasSize.y);

  // place, then normalize uvs
  for (auto& pair : atlasLayout) {
    Chara chara = glyphMap[pair.first];
    for (int i = 0; i < chara.height; i++) {
      int xOrigin = pair.second.uvBegin.x;
      int yOrigin = pair.second.uvBegin.y + (chara.height - i - 1);

      int offsetPix = chara.pitch * i;

      int offset = xOrigin + (yOrigin * atlasSize.x);
      memcpy(imgBuffer + offset, chara.pixData + offsetPix, chara.width);
    }

    pair.second.uvBegin /= atlasSize;
    pair.second.uvSize /= atlasSize;
  }

  glyphAtlas->destroyAndCreate();
  glyphAtlas->upload2d(atlasSize.x, atlasSize.y, DtUnsignedByte, BaseTexture::R,
                       imgBuffer);

  free(imgBuffer);

  needsAtlasRender = false;
}

void Font::buildBuffer(const char* text, gfx::BaseBuffer* vBuffer,
                       gfx::BaseBuffer* iBuffer, int* outNumElements,
                       gfx::BaseArrayPointers* ap, glm::vec2 normalization) {
  // TODO: propertly iterate through text as if it were Unicode
  std::vector<NGuiVertex> vertices;
  std::vector<uint32_t> indices;
  glm::vec2 characterStride(0);
  while (*text != 0) {
    NGuiVertex v1, v2, v3, v4;

    if (*text == '\n') {
      characterStride.x = 0;
      characterStride.y += maxLineHeight;
    } else {
      uint32_t glyphIndex = FT_Get_Char_Index(face, *text);

      CharaLayout cl = atlasLayout[glyphIndex];
      Chara c = glyphMap[glyphIndex];

      glm::vec2 ourStride = characterStride;
      ourStride.x += c.bearing.x;
      ourStride.y -= c.height - c.bearing.y;
      v1.position = glm::vec2(ourStride.x, ourStride.y);
      v1.uv = glm::vec2(cl.uvBegin.x, cl.uvBegin.y);
      v2.position = glm::vec2(ourStride.x + c.width, ourStride.y);
      v2.uv = glm::vec2(cl.uvBegin.x + cl.uvSize.x, cl.uvBegin.y);
      v3.position = glm::vec2(ourStride.x, ourStride.y + c.height);
      v3.uv = glm::vec2(cl.uvBegin.x, cl.uvBegin.y + cl.uvSize.y);
      v4.position = glm::vec2(ourStride.x + c.width, ourStride.y + c.height);
      v4.uv = glm::vec2(cl.uvBegin.x + cl.uvSize.x, cl.uvBegin.y + cl.uvSize.y);
      characterStride += c.advance;

      uint32_t offset = (uint32_t)vertices.size();
      vertices.push_back(v1);
      vertices.push_back(v2);
      vertices.push_back(v3);
      vertices.push_back(v4);

      indices.push_back(offset);
      indices.push_back(offset + 1);
      indices.push_back(offset + 2);

      indices.push_back(offset + 2);
      indices.push_back(offset + 1);
      indices.push_back(offset + 3);
    }

    text++;
  }

  vBuffer->upload(BaseBuffer::Array, BaseBuffer::DynamicDraw,
                  vertices.size() * sizeof(NGuiVertex), vertices.data());
  iBuffer->upload(BaseBuffer::Element, BaseBuffer::DynamicDraw,
                  indices.size() * sizeof(uint32_t), indices.data());
  *outNumElements = indices.size();
  if (ap) {
    ap->deleteAttribs();
    ap->addAttrib(BaseArrayPointers::Attrib(
        DtFloat, 0, 2, sizeof(NGuiVertex),
        (void*)offsetof(NGuiVertex, position), vBuffer));
    ap->addAttrib(BaseArrayPointers::Attrib(DtFloat, 1, 2, sizeof(NGuiVertex),
                                            (void*)offsetof(NGuiVertex, uv),
                                            vBuffer));
    ap->upload();
  }
}

Font::~Font() {}

FontCache::FontCache(gfx::Engine* engine) {
  this->engine = engine;
  FT_Init_FreeType(&ftLibrary);
}

FontCache::~FontCache() {}

std::string FontCache::toFontName(std::string font, int ptsize) {
  return std::format("{}-{}", font, ptsize);
}

Font* FontCache::get(std::string fontName, int ptsize) {
  std::string _fontName = toFontName(fontName, ptsize);
  auto it = fonts.find(_fontName);
  if (it != fonts.end()) {
    return &fonts[_fontName];
  } else {
    common::OptionalData ds =
        common::FileSystem::singleton()->readFile(fontName.c_str());
    if (ds) {
      void* dcpy = malloc(ds->size());
      memcpy(dcpy, ds->data(), ds->size());

      FT_Face face;
      FT_New_Memory_Face(ftLibrary, (const FT_Byte*)dcpy, ds->size(), 0, &face);
      float pxsize = ptsize;

      FT_Set_Pixel_Sizes(face, 0, pxsize);
      FT_Select_Charmap(face, FT_ENCODING_UNICODE);

      Font& f = fonts[_fontName];
      f.face = face;

      for (int i = 0; i < UINT8_MAX; i++) {
        if (isprint(i)) {
          f.renderGlyph(FT_Get_Char_Index(f.face, i));
        }
      }
      f.glyphAtlas = engine->getDevice()->createTexture();
      f.renderAtlas();

      return &f;
    } else {
      return NULL;
    }
  }
  return NULL;
}
}  // namespace rdm::gfx::gui
