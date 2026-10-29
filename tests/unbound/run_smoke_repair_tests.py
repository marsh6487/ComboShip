#!/usr/bin/env python3
"""Native reproductions of the October 29 message-rebind and mesh-logger crashes.

Runs extracted production functions and the actual archive-reorder message block.
Archive I/O and logging sinks are boundary doubles; tinyxml2 and union layouts
are real. This is not a configured game/runtime test.
"""
from pathlib import Path
import os
import subprocess
import tempfile
import unittest

from run_native_tests import function

ROOT = Path(__file__).resolve().parents[2]


def native(body, name, libraries=()):
    with tempfile.TemporaryDirectory(prefix="smoke-repair-") as tmp:
        source, binary = Path(tmp) / "test.cpp", Path(tmp) / "test"
        source.write_text(body)
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-O1", "-g",
                        "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                        "-fno-omit-frame-pointer", "-fno-pie", "-no-pie", str(source),
                        *libraries, "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True,
                       env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"}, timeout=15)


class SmokeRepairs(unittest.TestCase):
    def test_archive_reorder_restores_token_and_font_without_rebuilding_owned_tables(self):
        source = (ROOT / "soh/soh/z_message_OTR.cpp").read_text()
        loader = (ROOT / "soh/mods/transformation_masks/assets/mm_asset_loader.cpp").read_text()
        start = loader.index("                auto resMgr =", loader.index("// Refresh ONLY"))
        end = loader.index("                MMASSETS_LOG", start)
        reorder = loader[start:end]
        body = r'''
#include <array>
#include <cassert>
#include <cstdint>
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>
using u16=uint16_t; using u32=uint32_t;
struct MessageTableEntry { u16 textId; uint8_t typePos; const char* segment; u32 msgSize; };
namespace SOH { struct MessageEntry { u16 id; uint8_t textboxType,textboxYPos; std::string msg; }; }
constexpr u16 kTerminatorId=0xFFFF;
constexpr char kMessageEnd='\x02';
enum MessageLanguage { MSG_NES,MSG_GER,MSG_FRA,MSG_JPN,MSG_STAFF,MSG_LANGUAGE_COUNT };
'''
        body += function(source, "struct MessageTable {") + ";\n"
        body += r'''
std::array<MessageTable,MSG_LANGUAGE_COUNT> sTables;
bool sInitialized=false;
MessageTableEntry *sNesMessageEntryTablePtr=nullptr,*sGerMessageEntryTablePtr=nullptr,
 *sFraMessageEntryTablePtr=nullptr,*sJpnMessageEntryTablePtr=nullptr,*sStaffMessageEntryTablePtr=nullptr;
char* _message_0xFFFC_nes=nullptr;
struct LanguageSpec { MessageLanguage language; };
const LanguageSpec kLanguages[]={{MSG_NES},{MSG_GER},{MSG_FRA},{MSG_JPN},{MSG_STAFF}};
int loads=0;
bool LoadBase(MessageTable& table,const LanguageSpec&) {
    ++loads;table.Set(0xB4,0,"Gold Skulltula token!");table.Set(0xFFFC,0,"ABC");return true;
}
void LoadOverrides(MessageTable&,const LanguageSpec&) {}
struct ResourceManager { void UnloadResource(const char*) {} };
struct Context { ResourceManager rm; ResourceManager* GetResourceManager() { return &rm; } };
struct OTRGlobals { static OTRGlobals* Instance; Context contextStorage; Context* context=&contextStorage; };
OTRGlobals globals; OTRGlobals* OTRGlobals::Instance=&globals;
'''
        for signature in ("void PublishTables(", 'extern "C" void OTRMessage_Init(',
                          'extern "C" MessageTableEntry* OTRMessage_Find(',
                          'extern "C" void OTRMessage_ResetForResume('):
            body += function(source, signature) + "\n"
        body += "void Reorder() {\n" + reorder + "}\n"
        body += r'''
int main() {
    OTRMessage_Init();assert(loads==5);
    auto* table=sNesMessageEntryTablePtr;
    const char* token=OTRMessage_Find(table,0xB4)->segment;
    const char* font=_message_0xFFFC_nes;
    for(int i=0;i<10;++i) {
        Reorder();
        assert(sNesMessageEntryTablePtr==table && "archive reorder lost the published message table");
        assert(sGerMessageEntryTablePtr && sFraMessageEntryTablePtr && sJpnMessageEntryTablePtr && sStaffMessageEntryTablePtr);
        assert(OTRMessage_Find(table,0xB4)->segment==token);
        assert(std::string(token)=="Gold Skulltula token!\x02");
        assert(_message_0xFFFC_nes==font && std::string(font)=="ABC\x02");
        assert(loads==5 && "archive reorder must retain process-owned message storage");
    }
    OTRMessage_ResetForResume();assert(loads==10 && sNesMessageEntryTablePtr);
    Reorder();assert(loads==10 && _message_0xFFFC_nes);
}
'''
        native(body, "message-rebind")

    def test_single_background_logger_uses_single_union_member(self):
        source = (ROOT / "soh/soh/resource/logging/SceneCommandLoggers.cpp").read_text()
        header = (ROOT / "soh/soh/resource/type/scenecommand/SetMesh.h").read_text()
        # Use the native union/struct definitions, with only the base resource/log sink doubled.
        layouts = header[header.index("typedef struct {"):header.index("class SetMesh :")]
        body = r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <tinyxml2.h>
using u8=uint8_t; using u16=uint16_t; using u32=uint32_t; using f32=float;
using Gfx=char; struct Vec3f { float x,y,z; };
namespace Ship { struct IResource { virtual ~IResource()=default; }; }
std::string logged;
#define SPDLOG_INFO(fmt,path,xml) (logged=(xml))
'''
        body += layouts
        body += "struct SetMesh:Ship::IResource { uint8_t data=0; MeshHeader meshHeader{};\n"
        body += function(header, "    void SetSingleImage(") + "\n};\n"
        body += function(source, "const char* TrimOTRSignature(") + "\n"
        body += function(source, "void LogMeshAsXML(") + "\n"
        body += r'''
int main() {
    auto mesh=std::make_shared<SetMesh>();
    PolygonDlist dlist{};
    mesh->meshHeader.base.type=1;
    mesh->meshHeader.polygon1.format=1;
    mesh->meshHeader.polygon1.dlist=(Gfx*)&dlist;
    BgImage image{};image.source=(void*)"__OTR__backgrounds/title";
    image.width=320;image.height=240;image.fmt=2;image.siz=1;
    mesh->SetSingleImage(image); // unk_0C=0 leaves the overlapping multi.list null, as in the crash.
    LogMeshAsXML(mesh);
    tinyxml2::XMLDocument doc;assert(doc.Parse(logged.c_str())==tinyxml2::XML_SUCCESS);
    auto* polygon=doc.RootElement()->FirstChildElement("Polygon");
    assert(polygon->IntAttribute("BgImageCount")==1);
    auto* bg=polygon->FirstChildElement("BgImage");assert(bg && !bg->NextSiblingElement());
    assert(std::string(bg->Attribute("ImagePath"))=="backgrounds/title");
    assert(bg->IntAttribute("Width")==320 && bg->IntAttribute("Height")==240);
    image.unk_0C=0xDEADBEEF;mesh->SetSingleImage(image);LogMeshAsXML(mesh);
    assert(doc.Parse(logged.c_str())==tinyxml2::XML_SUCCESS);
    assert(doc.RootElement()->FirstChildElement()->FirstChildElement()->UnsignedAttribute("Unknown_0C")==0xDEADBEEF);
    // Multi-background rooms retain their actual ids/count and per-image data.
    BgImage images[2]={image,image};images[0].id=3;images[1].id=9;
    mesh->meshHeader.polygon1.format=2;mesh->meshHeader.polygon1.multi.count=2;
    mesh->meshHeader.polygon1.multi.list=images;LogMeshAsXML(mesh);
    assert(doc.Parse(logged.c_str())==tinyxml2::XML_SUCCESS);
    polygon=doc.RootElement()->FirstChildElement();assert(polygon->IntAttribute("BgImageCount")==2);
    bg=polygon->FirstChildElement();assert(bg->IntAttribute("Id")==3);
    assert(bg->NextSiblingElement()->IntAttribute("Id")==9);
}
'''
        native(body, "mesh-log", ["-ltinyxml2"])


if __name__ == "__main__":
    unittest.main()
