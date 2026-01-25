#include "resource.hpp"

#include <assimp/anim.h>
#include <assimp/material.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/quaternion.h>
#include <assimp/scene.h>
#include <assimp/texture.h>
#include <assimp/types.h>
#include <assimp/vector3.h>
#include <lua.h>
#include <sys/types.h>

#include <stdexcept>

#include "filesystem.hpp"
#include "game.hpp"
#include "gfx/base_device.hpp"
#include "gfx/base_types.hpp"
#include "gfx/camera.hpp"
#include "gfx/engine.hpp"
#include "gfx/lighting.hpp"
#include "gfx/mesh.hpp"
#include "gfx/viewport.hpp"
#include "logging.hpp"
#include "object.hpp"
#include "object_property.hpp"
#include "script/script_api.hpp"
#include "settings.hpp"
#include "worker.hpp"
namespace rdm {
RDM_REFLECTION_BEGIN_DESCRIBED(ResourceManager);
RDM_REFLECTION_PROPERTY_FUNCTION(
    ResourceManager, LoadTexture, [](lua_State* L) {
      ResourceManager* rmgr =
          script::ObjectBridge::getDescribed<ResourceManager>(L, 1);
      resource::Texture* texture =
          rmgr->load<resource::Texture>(lua_tostring(L, 2));
      script::ObjectBridge::pushDescribed(L, texture);
      return 1;
    });
RDM_REFLECTION_PROPERTY_FUNCTION(ResourceManager, LoadModel, [](lua_State* L) {
  ResourceManager* rmgr =
      script::ObjectBridge::getDescribed<ResourceManager>(L, 1);
  resource::Model* model = rmgr->load<resource::Model>(lua_tostring(L, 2));
  script::ObjectBridge::pushDescribed(L, model);
  return 1;
});
RDM_REFLECTION_END_DESCRIBED();

RDM_REFLECTION_BEGIN_DESCRIBED(BaseResource);
RDM_REFLECTION_PROPERTY_STRING(BaseResource, Name, &BaseResource::getName,
                               NULL);
RDM_REFLECTION_END_DESCRIBED();

namespace resource {
RDM_REFLECTION_BEGIN_DESCRIBED(BaseGfxResource);
}

ResourceManager::ResourceManager() {
  missingTexture = load<resource::Texture>(RESOURCE_MISSING_TEXTURE);
  missingModel = load<resource::Model>(RESOURCE_MISSING_MODEL);

  resource::Texture::TextureSettings ts;
  ts.minFiltering = gfx::BaseTexture::Nearest;
  ts.maxFiltering = gfx::BaseTexture::Nearest;
  missingTexture->setTextureSettings(ts);
  previewViewport = NULL;
}

void BaseResource::loadData() {
  if (broken) return;

  common::OptionalData data =
      common::FileSystem::singleton()->readFile(getName().c_str());
  if (data) {
    onLoadData(data);
    isDataReady = true;
    Log::printf(LOG_DEBUG, "Loaded resource data %s", name.c_str());
  } else {
    Log::printf(LOG_ERROR, "Could not load resource path %s", name.c_str());
    broken = true;
  }
  needsData = false;
}

static CVar rsc_load_quota("rsc_load_quota", "5", CVARF_SAVE | CVARF_GLOBAL);

void ResourceManager::startTaskForResource(BaseResource* br) {
  br->setNeedsData(false);
  WorkerManager::singleton()->run([br] { br->loadData(); });
}

void ResourceManager::tick() {
  int quota_amt = 0;
  for (auto& [rscId, rsc] : resources) {
    if (rsc->getNeedsData()) {
      quota_amt++;

      if (quota_amt > rsc_load_quota.getInt()) break;
    }
  }
}

static CVar r_upload_quota("r_upload_quota", "100", CVARF_SAVE | CVARF_GLOBAL);

void ResourceManager::tickGfx(gfx::Engine* engine) {
  int quota_amt = 0;
  for (auto& [rscId, rsc] : resources) {
    if (resource::BaseGfxResource* rscg =
            dynamic_cast<resource::BaseGfxResource*>(rsc.get())) {
      if (!rscg->getReady() && rscg->getDataReady()) {
        Log::printf(LOG_DEBUG, "Loaded gfx resource for %s",
                    rscg->getName().c_str());
        rscg->gfxUpload(engine);
        quota_amt++;

        if (quota_amt > r_upload_quota.getInt()) {
          break;
        }
      }
    }
  }
}

static BaseResource* selectedResource = NULL;
static resource::Model::Animator* animator = NULL;

void ResourceManager::deleteGfxResources() {
  for (auto& [rscId, rsc] : resources) {
    if (resource::BaseGfxResource* rscg =
            dynamic_cast<resource::BaseGfxResource*>(rsc.get())) {
      rscg->gfxDelete();
    }
  }
}

resource::BaseGfxResource::BaseGfxResource(ResourceManager* manager,
                                           std::string name)
    : BaseResource(manager, name) {
  isReady = false;
}
};  // namespace rdm
