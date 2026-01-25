#include "ngui_elements.hpp"

#include "font.hpp"
#include "game.hpp"
#include "ngui.hpp"
#include "ngui_window.hpp"
#include "world.hpp"

namespace rdm::gfx::gui {
void TextLabel::updateText() {
  if (!dirty) return;

  Font* toUse = font;
  if (!toUse) {
    toUse = getGuiManager()->getFontCache()->get(NGUI_UI_FONT);
  }
  if (!toUse) return;
  dirty = false;

  toUse->buildBuffer(text.c_str(), vBuffer.get(), iBuffer.get(),
                     &numTextElements, ap.get());
  setMinSize(toUse->getTextSize(text.c_str()));
  setSize(getSize());
  color = glm::vec3(1.0);
}

void TextLabel::elementRender(NGuiRenderer* renderer) {
  updateText();

  renderer->setColor(color);
  glm::vec2 p = getPosition();
  p.y -= getDisplaySize().y;

  Font* toUse = font;
  if (!toUse) {
    toUse = getGuiManager()->getFontCache()->get(NGUI_UI_FONT);
  }

  if (!numTextElements) return;
  if (!toUse) return;

  RenderCommand command(BaseDevice::Triangles, iBuffer.get(), numTextElements,
                        ap.get(), NULL, NULL);

  glm::vec2 target = glm::vec2(p.x, p.y);
  glm::vec2 window = renderer->getEngine()->getTargetResolution();
  glm::ivec2 textSize = toUse->getTextSize(text.c_str());
  if (target.x < 0) {
    target.x = (window.x + target.x) - textSize.x;
  }
  if (target.y < 0) {
    target.y = (window.y + target.y) - textSize.y;
  }
  target = glm::round(target);

  command.setOffset(target);
  command.setTexture(0, toUse->getTexture());
  command.setScale(glm::vec2(1));

  renderer->getList().add(command);
}

void TextInput::elementRender(NGuiRenderer* renderer) {
  std::string newText;
  if (std::string(textData).empty() && !emptyText.empty()) {
    newText = emptyText;
    setColor(glm::vec3(0.5));
  } else {
    newText = prefix + textData;
    setColor(glm::vec3(1.0));
  }
  if (text != newText) {
    dirty = true;
    text = newText;
  }

  updateText();

  setSize(getSize());

  glm::vec2 p = getPosition();
  p.y -= getMinSize().y;

  int status = renderer->mouseDownZone(p, getDisplaySize());
  bool value = false;
  switch (status) {
    default:
    case -1:
      renderer->setColor(glm::vec3(0.2));
      break;
    case 0:
      renderer->setColor(glm::vec3(0.0));
      break;
    case 1:
      renderer->setColor(glm::vec3(1.0));
      value = true;
      break;
  }

  renderer->image(getGuiManager()->getEngine()->getWhiteTexture(), p,
                  getDisplaySize());

  if (value) {
    if (!debounce) {
      getGuiManager()->setCurrentText(textData, 65535);
      debounce = true;
    }
  } else {
    if (status == -2 && getGuiManager()->isCurrentText(textData)) {
      getGuiManager()->setCurrentText(NULL, 0);
    }

    if (debounce) {
      debounce = false;
    }
  }
  TextLabel::elementRender(renderer);
}

void Button::elementRender(NGuiRenderer* renderer) {
  updateText();

  glm::vec2 p = getPosition();
  p.y -= getMinSize().y;
  renderer->setColor(glm::vec3(0.5));
  int status = renderer->mouseDownZone(p, getSize());
  bool value = false;
  switch (status) {
    default:
    case -1:
      renderer->setColor(glm::vec3(0.2));
      break;
    case 0:
      renderer->setColor(glm::vec3(0.0));
      break;
    case 1:
      renderer->setColor(glm::vec3(1.0));
      value = true;
      break;
  }

  if (value) {
    if (!debounce) {
      if (pressed.has_value()) pressed.value()();
      debounce = true;
    }
  } else {
    if (debounce) {
      debounce = false;
    }
  }

  renderer->image(getGuiManager()->getEngine()->getWhiteTexture(), p,
                  getMinSize());

  setSize(getSize());
  TextLabel::elementRender(renderer);
}

void Image::elementRender(NGuiRenderer* renderer) {
  glm::vec2 p = getPosition();
  p.y -= getSize().y;
  if (texture) {
    renderer->image(texture, p, getSize());
  }
}

ImageButton::ImageButton(NGuiManager* manager) : Image(manager) {
  textureOver = NULL;
}

void ImageButton::elementRender(NGuiRenderer* renderer) {
  setTexture(getGuiManager()
                 ->getEngine()
                 ->getWorld()
                 ->getGame()
                 ->getResourceManager()
                 ->load<resource::Texture>("engine/gui/button1.png"));

  Image::elementRender(renderer);

  setMinSize(glm::vec2(32));
  setSize(glm::vec2(32));
  glm::vec2 p = getPosition();
  p.y -= 32;
  if (textureOver) {
    renderer->image(textureOver->getTexture(), p, glm::vec3(32));
  }
}
}  // namespace rdm::gfx::gui
