#include "soh/Enhancements/randomizer/particle_cmc.h"
#include "combo/rando/ContainerCategoryColors.h"
#include "tests/test_require.h"
#include <cstdio>
static const unsigned char expected[3][7][4]={
 {{255,255,255,255},{170,50,0,255},{250,0,0,255},{255,255,0,255},{180,180,180,255},{0,0,0,255},{250,185,40,255}},
 {{255,255,255,255},{250,75,0,255},{0,0,255,255},{0,200,255,255},{130,130,130,255},{255,250,190,255},{255,220,135,255}},
 {{135,135,135,255},{255,160,100,255},{255,125,125,255},{0,200,255,255},{100,100,100,255},{255,255,255,255},{250,220,180,255}}};
int main(){
 using namespace ComboRando::ContainerCategoryColors;
 for(int component=0;component<4;++component)for(int category=0;category<7;++category){
  auto native=Randomizer_GetParticleCMCColor(static_cast<GetItemCategory>(category),static_cast<ParticleCMCColorType>(component));
  const auto& color=expected[component<3?component:0][category];
  REQUIRE(native.r==color[0]&&native.g==color[1]&&native.b==color[2]&&native.a==color[3]);
  auto shared=Get(static_cast<Category>(category),component);
  REQUIRE(shared.r==native.r&&shared.g==native.g&&shared.b==native.b&&shared.a==native.a);
 }
 REQUIRE(Get(Category::Mask,0).r!=Get(Category::Junk,0).r);
 REQUIRE(Get(Category::StrayFairy,0).g!=Get(Category::Junk,0).g);
 REQUIRE(Get(Category::Mask,0).b!=Get(Category::StrayFairy,0).b);
 puts("PASS actual OoT CSMC all7categories/all3RGBAcomponents and fallback unchanged; distinct MM mask/fairy semantics");
}
