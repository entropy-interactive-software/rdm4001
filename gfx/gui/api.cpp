#include "api.hpp"

#include <algorithm>
#include <numeric>
#include <string>

#include "gfx/engine.hpp"
#include "gui.hpp"
#include "script/context.hpp"
#include "script/my_basic.h"
namespace rdm::gfx::gui {

static int _BasGetValue(struct mb_interpreter_t* s, void** l) {
  int result = MB_FUNC_OK;

  char* componentName;
  char* id;

  mb_check(mb_attempt_open_bracket(s, l));
  mb_check(mb_pop_string(s, l, &componentName));
  mb_check(mb_pop_string(s, l, &id));
  mb_check(mb_attempt_close_bracket(s, l));

  script::Context* context;
  mb_get_userdata(s, (void**)&context);

  auto component =
      context->getEngine()->getGuiManager()->getComponentByName(componentName);
  if (!component) {
    Log::printf(LOG_ERROR, "GUI could not find component %s", component);
    return MB_FUNC_ERR;
  }

  auto it = component.value()->elements.find(id);
  if (it == component.value()->elements.end()) {
    Log::printf(LOG_ERROR, "GUI could not find id %s", id);
    return MB_FUNC_ERR;
  }

  mb_push_string(
      s, l, mb_memdup(it->second.value.data(), it->second.value.size() + 1));

  return result;
}

static int _BasSetValue(struct mb_interpreter_t* s, void** l) {
  int result = MB_FUNC_OK;

  char* componentName;
  char* id;
  mb_value_t value;

  mb_check(mb_attempt_open_bracket(s, l));
  mb_check(mb_pop_string(s, l, &componentName));
  mb_check(mb_pop_string(s, l, &id));
  mb_check(mb_pop_value(s, l, &value));
  mb_check(mb_attempt_close_bracket(s, l));

  script::Context* context;
  mb_get_userdata(s, (void**)&context);

  auto component =
      context->getEngine()->getGuiManager()->getComponentByName(componentName);
  if (!component) {
    Log::printf(LOG_ERROR, "GUI could not find component %s", component);
    return MB_FUNC_ERR;
  }

  auto it = component.value()->elements.find(id);
  if (it == component.value()->elements.end()) {
    Log::printf(LOG_ERROR, "GUI could not find id %s", id);
    return MB_FUNC_ERR;
  }

  switch (value.type) {
    case MB_DT_INT:
      it->second.value = std::to_string(value.value.integer);
      break;
    case MB_DT_STRING:
      it->second.value = value.value.string;
      break;
    default:
      return MB_FUNC_ERR;
  }

  it->second.dirty = true;

  return result;
}

static int _BasOnPressed(struct mb_interpreter_t* s, void** l) {
  int result = MB_FUNC_OK;

  char* componentName;
  char* id;
  char* runtime;

  mb_check(mb_attempt_open_bracket(s, l));
  mb_check(mb_pop_string(s, l, &componentName));
  mb_check(mb_pop_string(s, l, &id));
  mb_check(mb_pop_string(s, l, &runtime));
  mb_check(mb_attempt_close_bracket(s, l));

  script::Context* context;
  mb_get_userdata(s, (void**)&context);

  auto component =
      context->getEngine()->getGuiManager()->getComponentByName(componentName);
  if (!component) {
    Log::printf(LOG_ERROR, "GUI could not find component %s", component);
    return MB_FUNC_ERR;
  }

  auto it = component.value()->elements.find(id);
  if (it == component.value()->elements.end()) {
    Log::printf(LOG_ERROR, "GUI could not find id %s", id);
    return MB_FUNC_ERR;
  }

  std::string _runtime = runtime;
  std::transform(_runtime.begin(), _runtime.end(), _runtime.begin(), ::toupper);
  it->second.mouseDown.listen([s, l, _runtime] {
    mb_value_t routine;
    mb_value_t args[1];
    mb_value_t ret;
    mb_get_routine(s, l, _runtime.c_str(), &routine);
    mb_make_nil(ret);
    mb_eval_routine(s, l, routine, args, 0, &ret);
  });

  return MB_FUNC_OK;
}

static int _BasGetInValue(struct mb_interpreter_t* s, void** l) {
  int result = MB_FUNC_OK;

  char* componentName;
  char* id;
  mb_value_t value;

  mb_check(mb_attempt_open_bracket(s, l));
  mb_check(mb_pop_string(s, l, &componentName));
  mb_check(mb_pop_string(s, l, &id));
  mb_check(mb_attempt_close_bracket(s, l));

  script::Context* context;
  mb_get_userdata(s, (void**)&context);

  auto component =
      context->getEngine()->getGuiManager()->getComponentByName(componentName);
  if (!component) {
    Log::printf(LOG_ERROR, "GUI could not find component %s", component);
    return MB_FUNC_ERR;
  }

  auto it = component.value()->inVars.find(id);
  if (it == component.value()->inVars.end()) {
    Log::printf(LOG_ERROR, "GUI could not find id %s", id);
    return MB_FUNC_ERR;
  }

  switch (it->second.type) {
    case Component::Var::String:
      break;
    case Component::Var::Float: {
      float v = *(float*)it->second.value;
      mb_push_real(s, l, v);
    } break;
    case Component::Var::Int: {
      int v = *(int*)it->second.value;
      mb_push_int(s, l, v);
    } break;
  }

  return result;
}

static int _BasSetVisible(struct mb_interpreter_t* s, void** l) {
  char* componentName;
  int visible;

  mb_check(mb_attempt_open_bracket(s, l));
  mb_check(mb_pop_string(s, l, &componentName));
  mb_check(mb_pop_int(s, l, &visible));
  mb_check(mb_attempt_close_bracket(s, l));

  script::Context* context;
  mb_get_userdata(s, (void**)&context);

  auto component =
      context->getEngine()->getGuiManager()->getComponentByName(componentName);
  if (!component) {
    Log::printf(LOG_ERROR, "GUI could not find component %s", component);
    return MB_FUNC_ERR;
  }

  component.value()->domRoot.visible = visible;

  return MB_FUNC_OK;
}

static int _BasSetColor(struct mb_interpreter_t* s, void** l) {
  char* componentName;
  char* id;
  glm::vec3 color;

  mb_check(mb_attempt_open_bracket(s, l));
  mb_check(mb_pop_string(s, l, &componentName));
  mb_check(mb_pop_string(s, l, &id));
  mb_check(mb_pop_real(s, l, &color.x));
  mb_check(mb_pop_real(s, l, &color.y));
  mb_check(mb_pop_real(s, l, &color.z));
  mb_check(mb_attempt_close_bracket(s, l));

  script::Context* context;
  mb_get_userdata(s, (void**)&context);

  auto component =
      context->getEngine()->getGuiManager()->getComponentByName(componentName);
  if (!component) {
    Log::printf(LOG_ERROR, "GUI could not find component %s", component);
    return MB_FUNC_ERR;
  }

  auto it = component.value()->elements.find(id);
  if (it == component.value()->elements.end()) {
    Log::printf(LOG_ERROR, "GUI could not find id %s", id);
    return MB_FUNC_ERR;
  }

  it->second.color = color;

  return MB_FUNC_OK;
}

void API::registerApi(struct mb_interpreter_t* s) {
  mb_begin_module(s, "GUI");
  mb_register_func(s, "SETVALUE", _BasSetValue);
  mb_register_func(s, "GETVALUE", _BasGetValue);
  mb_register_func(s, "GETINVALUE", _BasGetInValue);
  mb_register_func(s, "ONPRESSED", _BasOnPressed);
  mb_register_func(s, "SETVISIBLE", _BasSetVisible);
  mb_register_func(s, "SETCOLOR", _BasSetColor);
  mb_end_module(s);
}
}  // namespace rdm::gfx::gui
