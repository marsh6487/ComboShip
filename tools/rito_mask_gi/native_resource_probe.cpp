#include <iostream>
#include <fstream>
#include <filesystem>
#include <iterator>
#include <cstring>
#include <tinyxml2.h>
#include "ship/resource/ResourceLoader.h"
#include "ship/utils/binarytools/MemoryStream.h"
#include "fast/resource/ResourceType.h"
#include "fast/resource/factory/DisplayListFactory.h"
#include "fast/resource/factory/VertexFactory.h"
#include "fast/resource/factory/TextureFactory.h"
#include "fast/resource/factory/MatrixFactory.h"
#include "fast/resource/type/DisplayList.h"
#include "fast/resource/type/Vertex.h"
#include "fast/resource/type/Texture.h"
class HeaderProbe : public Ship::ResourceLoader {
public: using ResourceLoader::ReadResourceInitDataLegacy;
};
int main(int argc,char**argv) {
 HeaderProbe header;header.RegisterResourceFactory(std::make_shared<Fast::ResourceFactoryXMLDisplayListV0>(),RESOURCE_FORMAT_XML,"DisplayList",(uint32_t)Fast::ResourceType::DisplayList,0);
 header.RegisterResourceFactory(std::make_shared<Fast::ResourceFactoryXMLVertexV0>(),RESOURCE_FORMAT_XML,"Vertex",(uint32_t)Fast::ResourceType::Vertex,0);
 header.RegisterResourceFactory(std::make_shared<Fast::ResourceFactoryBinaryTextureV0>(),RESOURCE_FORMAT_BINARY,"Texture",(uint32_t)Fast::ResourceType::Texture,0);
 header.RegisterResourceFactory(std::make_shared<Fast::ResourceFactoryBinaryMatrixV0>(),RESOURCE_FORMAT_BINARY,"Matrix",(uint32_t)Fast::ResourceType::Matrix,0);
 size_t count=0;
 for(int i=1;i<argc;i++) for(auto const&entry:std::filesystem::directory_iterator(argv[i])) {
  if(!entry.is_regular_file())continue;
  std::ifstream input(entry.path(),std::ios::binary);auto file=std::make_shared<Ship::File>();file->Buffer=std::make_shared<std::vector<char>>(std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>());
  auto init=std::make_shared<Ship::ResourceInitData>();init->Path=entry.path().string();init->ResourceVersion=0;
  if(file->Buffer->at(0)=='<') { auto xml=std::make_shared<tinyxml2::XMLDocument>();xml->Parse(file->Buffer->data(),file->Buffer->size());if(xml->Error())return 2; init->Format=RESOURCE_FORMAT_XML;init->Type=std::string(xml->RootElement()->Name())=="DisplayList"?(uint32_t)Fast::ResourceType::DisplayList:(uint32_t)Fast::ResourceType::Vertex; }
  else init=header.ReadResourceInitDataLegacy(entry.path().string(),file);if(!init)return 2;
  std::shared_ptr<Ship::IResource> resource;
  if(init->Format==RESOURCE_FORMAT_XML) {
   auto xml=std::make_shared<tinyxml2::XMLDocument>();xml->Parse(file->Buffer->data(),file->Buffer->size());if(xml->Error())return 3;file->Reader=xml;
   if(init->Type==(uint32_t)Fast::ResourceType::DisplayList)resource=Fast::ResourceFactoryXMLDisplayListV0().ReadResource(file,init);
   else if(init->Type==(uint32_t)Fast::ResourceType::Vertex)resource=Fast::ResourceFactoryXMLVertexV0().ReadResource(file,init);
  } else {
   auto stream=std::make_shared<Ship::MemoryStream>(file->Buffer,64);auto reader=std::make_shared<Ship::BinaryReader>(stream);reader->SetEndianness(init->ByteOrder);file->Reader=reader;
   if(init->Type==(uint32_t)Fast::ResourceType::Matrix)resource=Fast::ResourceFactoryBinaryMatrixV0().ReadResource(file,init);
   else resource=Fast::ResourceFactoryBinaryTextureV0().ReadResource(file,init);
  }
  if(!resource || !resource->GetRawPointer() || !resource->GetPointerSize()){std::cerr<<"FAILED "<<entry.path()<<"\n";return 4;}
  std::cout<<"PASS "<<entry.path().filename()<<" type="<<std::hex<<init->Type<<std::dec<<" bytes="<<resource->GetPointerSize()<<"\n";++count;
 }
 std::cout<<"PASS resources="<<count<<" actual native libultraship factories\n";
}
