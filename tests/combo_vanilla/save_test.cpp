#include "combo/vanilla/ComboVanillaFile.h"
#include "combo/vanilla/ComboVanillaSave.h"
#include <cassert>
#include <fstream>
#include <iostream>

using nlohmann::json;
using namespace ComboVanilla;

static json Oot() {
    return { { "fileType", 0 }, { "version", 1 },
             { "sections", { { "base", { { "version", 4 }, { "data", {
                 { "playerName", json::array({23,10,22,14,1,2,3,4}) }, { "healthCapacity", 48 }
             } } } } } } };
}
static json Mm() {
    return { { "type", "2S2H_SAVE" }, { "version", 9 }, { "newCycleSave", { { "save", {
        { "shipSaveInfo", { { "saveType", 0 } } },
        { "saveInfo", { { "playerData", { { "playerName", json::array({23,10,22,14,1,2,3,4}) } } } } }
    } } } } };
}

int main(int argc, char **argv) {
  assert(argc == 2);
  const std::filesystem::path directory = argv[1];
  const auto file = directory / "slot.combosav";
  assert(WriteFile(file, "previous"));
  assert(WriteFile(file, "complete pair"));
  std::string contents;
  std::getline(std::ifstream(file), contents);
  assert(contents == "complete pair");
  std::filesystem::create_directory(file.string() + ".temp");
  assert(!WriteFile(file, "must not replace previous"));
  std::getline(std::ifstream(file), contents);
  assert(contents == "complete pair");
  std::filesystem::remove(file.string() + ".temp");
  const json prior = {{"comboVersion", 1},
                      {"comboRelease", "9.3.0"},
                      {"slot", 1},
                      {"combo", {{"rando", {{"masterSeed", 12}}}}},
                      {"oot", {{"previous", true}}},
                      {"mm", {{"previous", true}}}};
  assert(FileMode(prior) == Mode::Randomizer);
  assert(FileMode(json::object()) ==
         Mode::Invalid); // Missing seed is never vanilla.
  auto bad = prior;
  bad["combo"]["mode"] = "unrecognized";
  assert(FileMode(bad) == Mode::Invalid);
  bad = prior;
  bad["combo"]["mode"] = "vanilla-nei";
  bad["combo"]["vanilla"]["version"] = 1;
  assert(FileMode(bad) ==
         Mode::Invalid); // Seed and explicit Vanilla cannot coexist.
  bad["combo"].erase("rando");
  assert(FileMode(bad) == Mode::Vanilla);
  bad["combo"]["vanilla"]["version"] = 2;
  assert(FileMode(bad) == Mode::Invalid);
  bad["combo"]["vanilla"]["version"] = "1";
  assert(FileMode(bad) == Mode::Invalid);

  Creation malformed;
    assert(malformed.Begin(1,prior));
    auto incomplete = Oot(); incomplete["sections"] = json::object();
    assert(malformed.Stage(0,incomplete) && malformed.Stage(1,Mm()));
    assert(!malformed.Ready());
    malformed.Abort();
    Creation creation;
  assert(!creation.Begin(-1, prior));
  assert(!creation.Begin(3, prior));
  assert(creation.Begin(1, prior));
  assert(!creation.Begin(0, prior));
  assert(FileMode(creation.Document()) == Mode::Vanilla);
  assert(!creation.Document()["combo"].contains("rando"));
  assert(!creation.Stage(2, Oot()));
  assert(!creation.Stage(0, json::array()));
  assert(creation.Stage(0, Oot()));
  int writes = 0;
  json disk = prior;
  auto writer = [&](const json &document) {
    ++writes;
    disk = document;
    return true;
  };
  assert(!creation.Commit(
      writer)); // Half-pair never reaches the filesystem boundary.
  assert(writes == 0 && disk == prior);
  assert(creation.Stage(1, Mm()));
  auto seededMm = Mm();
  seededMm["newCycleSave"]["save"]["shipSaveInfo"]["saveType"] = 1;
  assert(creation.Stage(1, seededMm));
  assert(!creation.Commit(writer));
  assert(writes == 0 && disk == prior);
  assert(creation.Stage(1, Mm()));
  assert(!creation.Commit([](const json &) { return false; }));
  assert(disk == prior && creation.Active());
  assert(creation.Commit(writer));
  assert(writes == 1 && !creation.Active());
  assert(FileMode(disk) == Mode::Vanilla && disk["slot"] == 1);
  assert(!creation.Commit(writer) && writes == 1);
  assert(!creation.Stage(0, Oot()));

  assert(creation.Begin(0, prior));
  assert(creation.Stage(0, Oot()));
  creation.Abort();
  assert(!creation.Active() && !creation.Commit(writer) && writes == 1);
  assert(disk["slot"] == 1);
  std::cout << "PASS explicit/legacy identity, paired staging/abort, native "
               "type bounds and atomic replacement failure\n";
}
