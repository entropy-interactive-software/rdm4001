#pragma once
#include "gfx/entity.hpp"
#include "map.hpp"
#include "network/bitstream.hpp"
#include "network/entity.hpp"
#include "sound.hpp"
namespace net = rdm::network;
namespace ww {
class Worldspawn : public net::Entity {
  BSPFile* file;
  rdm::ClosureId worldJob;
  rdm::ClosureId gfxJob;
  bool pendingAddToGfx;
  rdm::gfx::Entity* entity;
  std::mutex mutex;
  std::string mapName;
  std::string nextMapName;
  float roundStartTime;
  std::unique_ptr<rdm::SoundEmitter> emitter;
  std::vector<glm::vec3> mapSpawnLocations;
  int nextSpawnLocation;

  std::string mapPath(std::string name);

 public:
  enum Status {
    InGame,
    RoundBeginning,
    WaitingForPlayer,
    RoundEnding,
    RoundEnded,
    Unknown
  };

  enum GameMode {
    Deathmatch,
    Murder,
  };

  void destroyFile();
  glm::vec3 spawnLocation();

  Worldspawn(net::NetworkManager* manager, net::EntityId id);
  virtual ~Worldspawn();

  virtual void tick();
  void setNextMap(std::string name) { nextMapName = name; };
  void endRound();

  virtual const char* getTypeName() { return "Worldspawn"; };

  bool isPendingAddToGfx() { return pendingAddToGfx; }
  void loadFile(const char* file);  // call on backend

  BSPFile* getFile() { return file; }

  virtual void serialize(net::BitStream& stream);
  virtual void deserialize(net::BitStream& stream);

  GameMode getGameMode() { return gameMode; }

 private:
  Status currentStatus;
  GameMode gameMode;

  void setStatus(Status s);
};
}  // namespace ww
