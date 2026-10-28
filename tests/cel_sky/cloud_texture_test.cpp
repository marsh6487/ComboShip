// Execute the actual raw-cloud resource resolver. Invalid overrides must be
// rejected before the display list reads width*height*4 bytes; valid pixels
// must remain alive until delayed DL execution even after cache replacement.
#include <cstdint>
#include <cstdio>
#include <memory>
#include <unordered_map>
namespace Fast {
enum class TextureType { RGBA32bpp, CI4bpp };
struct Texture { uint8_t* ImageData; uint32_t ImageDataSize, Width, Height, Flags = 0; TextureType Type = TextureType::RGBA32bpp; };
}
constexpr int TEX_FLAG_LOAD_AS_IMG = 1;
struct WWCloudTexture { const uint8_t* data; int width, height; };
std::shared_ptr<Fast::Texture> resource;
std::shared_ptr<Fast::Texture> ResourceMgr_GetResourceByNameHandlingMQ(const char*) { return resource; }
/* PRODUCTION_CLOUD_TEXTURE */
int main() {
    uint8_t pixels[64*64*4]{};
    resource = std::make_shared<Fast::Texture>(Fast::Texture{pixels, sizeof pixels, 64, 64});
    resource->ImageDataSize = 1;
    if (FetchTex("cloud",512).data) { std::fputs("FAIL: truncated cloud override admitted\n",stderr); return 1; }
    resource->ImageDataSize = sizeof pixels; resource->Type = Fast::TextureType::CI4bpp;
    if (FetchTex("cloud",512).data) { std::fputs("FAIL: palette-index cloud override treated as RGBA32\n",stderr); return 1; }
    resource->Type = Fast::TextureType::RGBA32bpp; resource->Width = 63;
    if (FetchTex("cloud",512).data) { std::fputs("FAIL: non-power-of-two cloud admitted\n",stderr); return 1; }
    resource->Width = 64;
    auto texture = FetchTex("cloud",512);
    if (texture.data != pixels || texture.width != 64 || texture.height != 64) { std::fputs("FAIL: valid cloud rejected\n",stderr); return 1; }
    std::weak_ptr<Fast::Texture> owner = resource; resource.reset();
    if (owner.expired()) { std::fputs("FAIL: queued cloud pixels lost their resource owner\n",stderr); return 1; }
    if (FetchTex("missing",512).data) { std::fputs("FAIL: absent cloud admitted\n",stderr); return 1; }
    std::puts("cel cloud resource validation/lifetime: PASS");
}
