#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

#include <glm/ext/vector_int2.hpp>
#include <map>
#include <memory>
#include <string>

#include "gfx/base_types.hpp"

namespace rdm::gfx {
class Engine;
};

namespace rdm::gfx::gui {
class Font {
  friend class FontCache;

  struct Chara {
    int width, height, pitch;
    glm::ivec2 advance;
    glm::ivec2 bearing;
    int origin;
    unsigned char* pixData;
  };
  struct CharaLayout {
    glm::vec2 uvBegin;
    glm::vec2 uvSize;
  };
  std::map<uint32_t, Chara> glyphMap;
  std::map<uint32_t, CharaLayout> atlasLayout;
  std::unique_ptr<BaseTexture> glyphAtlas;
  int maxLineHeight;

  FT_Face face;
  bool needsAtlasRender;

  bool renderGlyph(uint32_t glyph);
  void renderAtlas();

 public:
  ~Font();

  glm::ivec2 getTextSize(const char* text);

  BaseTexture* getTexture() { return glyphAtlas.get(); }

  void buildBuffer(const char* text, gfx::BaseBuffer* vBuffer,
                   gfx::BaseBuffer* iBuffer, int* outNumElements,
                   gfx::BaseArrayPointers* ap,
                   glm::vec2 normalization = glm::vec2(0));
};

class FontCache {
  std::map<std::string, Font> fonts;
  gfx::Engine* engine;
  FT_Library ftLibrary;

 public:
  FontCache(gfx::Engine* engine);
  ~FontCache();

  std::string toFontName(std::string font, int ptsize);

  Font* get(std::string fontName, int ptsize);
};
};  // namespace rdm::gfx::gui
