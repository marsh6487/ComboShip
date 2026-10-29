#include "fast/RenderCostProbe.h"

#include <cassert>
#include <iostream>
#include <memory>
#include <string>

int main() {
    Fast::RenderCostProbe probe;
    assert(!probe.BeginFrame(false, 100));
    assert(!probe.Active());
    assert(!probe.Report().sampled);

    // Cadence counts actual requested rendered frames, not disabled/MM frames.
    for (unsigned frame = 1; frame <= 2 * Fast::RenderCostProbe::SampleEvery + 1; ++frame) {
        bool expected = (frame - 1) % Fast::RenderCostProbe::SampleEvery == 0;
        assert(probe.BeginFrame(true, frame * 1000) == expected);
        assert(probe.Active() == expected);
        probe.EndFrame(frame * 1000 + 700);
        assert(probe.Report().sampled == expected);
        if (expected) {
            assert(probe.Report().frameIndex == frame);
            assert(probe.Report().totalNanos == 700);
        }
        assert(!probe.Active());
    }

    // At 60 FPS / 20 Hz, sampling must cover each of the three interpolation
    // phases rather than repeatedly observing only the first drawn frame.
    Fast::RenderCostProbe phases;
    bool seen[3]{};
    for (unsigned frame = 0; frame < 3 * Fast::RenderCostProbe::SampleEvery; ++frame) {
        if (phases.BeginFrame(true, frame * 1000)) {
            seen[frame % 3] = true;
        }
        phases.EndFrame(frame * 1000 + 700);
    }
    assert(seen[0] && seen[1] && seen[2]);

    Fast::RenderCostProbe owners;
    assert(owners.BeginFrame(true, 100));
    owners.SyncDepth(1);
    owners.SetResource("scene/root");
    owners.AddCommand(4, 0x31, "G_DL_OTR_HASH", 10);
    owners.SyncDepth(2);
    owners.AddCommand(4, 0xE7, "G_RDPPIPESYNC", 20); // unmarked child must not inherit root
    owners.SetResource("model/head");
    owners.AddCommand(4, 5, "G_TRI1", 30);
    owners.SyncDepth(3); // branch sentinel/new target
    owners.SetResource("model/eyes");
    owners.AddCommand(4, 5, "G_TRI1", 40);
    owners.SyncDepth(1); // branch + parent return
    owners.AddCommand(4, 5, "G_TRI1", 50);
    {
        Fast::RenderTextureScope texture(&owners, "texture/hair");
        owners.AddUpload(4096, 5);
        {
            Fast::RenderTextureScope nested(&owners, "texture/mask");
            owners.AddUpload(16, 2);
        }
        owners.AddUpload(4096, 5);
    }
    owners.EndFrame(1000);
    const auto& r = owners.Report();
    assert(r.resources.size() == 4);
    assert(r.resources[0].commands.nanos == 20);
    assert(r.resources[1].path == "scene/root" && r.resources[1].commands.nanos == 60);
    assert(r.resources[2].path == "model/head" && r.resources[2].commands.nanos == 30);
    assert(r.resources[3].path == "model/eyes" && r.resources[3].commands.nanos == 40);
    assert(r.opcodes[4 * 256 + 5].nanos == 120 && r.opcodes[4 * 256 + 5].calls == 3);
    assert(r.upload.nanos == 12 && r.upload.calls == 3 && r.uploadBytes == 8208);
    assert(r.textures[1].path == "texture/hair" && r.textures[1].bytes == 8192);
    assert(r.textures[2].path == "texture/mask" && r.textures[2].bytes == 16);
    assert(r.totalNanos == 900);
    // Nested upload time is not added a second time to command or frame totals.
    assert(r.resources[1].commands.nanos == 60);

    Fast::RenderCostProbe dense;
    assert(dense.BeginFrame(true, 0, true));
    dense.SyncDepth(1);
    for (unsigned i = 0; i < 1000; ++i) {
        dense.SetResource("dense/resource/" + std::to_string(i));
        dense.BeginCommand();
        dense.SelectTexture("dense/texture/" + std::to_string(i));
        dense.AddUpload(4, 1, 1, 1);
        dense.AddCommand(4, 5, "G_TRI1", 1);
    }
    assert(dense.Report().resources.size() == 1001 && dense.Report().textures.size() == 1001);
    assert(!dense.Report().resourceOverflow && !dense.Report().textureOverflow);
    for (unsigned i = 0; i < 1000; ++i) {
        assert(dense.Report().resources[i + 1].path == "dense/resource/" + std::to_string(i));
        assert(dense.Report().textures[i + 1].path == "dense/texture/" + std::to_string(i));
        assert(dense.Report().resources[i + 1].commands.calls == 1);
        assert(dense.Report().textures[i + 1].bytes == 4);
    }
    dense.EndFrame(10000);

    Fast::RenderCostProbe bounded;
    assert(bounded.BeginFrame(true, 1000));
    bounded.SyncDepth(1);
    for (unsigned i = 0; i < 4103; ++i) {
        bounded.SetResource("resource/" + std::to_string(i));
        bounded.AddCommand(4, 5, "G_TRI1", 1);
    }
    assert(bounded.Report().resources.size() == 4096);
    assert(bounded.Report().resourceOverflow == 8);
    assert(bounded.Report().resources[0].commands.calls == 0);
    assert(bounded.Report().overflowResource.commands.calls == 8);
    // Invalid microcode must not index outside fixed arrays.
    bounded.AddCommand(255, 255, "invalid", 1);
    bounded.EndFrame(900); // defensively reject a backward test clock
    assert(bounded.Report().totalNanos == 0);

    // A later sampled frame clears owners and counters, retaining no asset pointers.
    for (unsigned i = 0; i < Fast::RenderCostProbe::SampleEvery; ++i) {
        bounded.BeginFrame(true, 2000);
        bounded.EndFrame(2100);
    }
    assert(bounded.Report().sampled);
    assert(bounded.Report().resources.size() == 1);
    assert(bounded.Report().uploadBytes == 0 && bounded.Report().resourceOverflow == 0);
    Fast::RenderCostProbe continuous;
    assert(continuous.BeginFrame(true, 100, true));
    continuous.EndFrame(200);
    assert(continuous.BeginFrame(true, 300, true)); // second frame must not disappear
    continuous.SyncDepth(1);
    continuous.SetResource("parent");
    int address = 0;
    continuous.PrepareResource(&address, "child_filepath");
    continuous.AddCommand(4, 39, "G_DL", 5); // caller owns the call itself
    continuous.SyncDepth(2);
    continuous.EnterCommand(&address);
    continuous.AddCommand(4, 5, "G_TRI1", 7);
    continuous.SyncDepth(1);
    continuous.AddCommand(4, 5, "G_TRI1", 11);
    continuous.EndFrame(500000300);
    assert(continuous.Report().totalNanos == 500000000);
    assert(continuous.Report().resources[1].commands.nanos == 16);
    assert(continuous.Report().resources[2].commands.nanos == 7);

    Fast::RenderCostProbe detailed;
    assert(detailed.BeginFrame(true, 0, true));
    detailed.SyncDepth(1);
    detailed.SetResource("parent");
    detailed.SetPass(7);
    detailed.BeginCommand();
    auto importing = detailed.BeginOperation(Fast::RenderCostOperation::TextureImport, 100);
    auto drawing = detailed.BeginOperation(Fast::RenderCostOperation::DriverDraw, 120);
    detailed.EndOperation(Fast::RenderCostOperation::DriverDraw, drawing, 150);
    detailed.SelectTexture("texture/packed-n64");
    detailed.DescribeTexture(32, 16, 2, 0, 1024, 1056, true);
    detailed.NoteTextureCache(false);
    detailed.AddUpload(2048, 30, 32, 16);
    detailed.EndOperation(Fast::RenderCostOperation::TextureImport, importing, 180);
    auto shader = detailed.BeginOperation(Fast::RenderCostOperation::ShaderCreate, 190);
    detailed.EndOperation(Fast::RenderCostOperation::ShaderCreate, shader, 200);
    detailed.AddCommand(4, 5, "G_TRI1", 120);
    const auto& parent = detailed.Report().resources[1];
    assert(parent.commands.nanos == 120 && parent.self.nanos == 30 && parent.self.calls == 1);
    assert(parent.textureImport.nanos == 80 && parent.driverDraw.nanos == 30 && parent.shaderCreate.nanos == 10);
    assert(parent.upload.nanos == 30 && !parent.selfUnknownCommands);
    assert(detailed.Report().passes[1].framebuffer == 7 && detailed.Report().passes[1].self.nanos == 30);
    assert(detailed.Report().textures[1].sourceDataBytes == 1024 &&
           detailed.Report().textures[1].sourceBackingBytes == 1056);
    assert(detailed.Report().textures[1].bytes == 2048 && detailed.Report().textures[1].uploadWidth == 32);
    detailed.DescribeTexture(64, 16, 2, 0, 2048);
    detailed.NoteTextureCache(true);
    detailed.AddUpload(4096, 5, 64, 16);
    assert(detailed.Report().textures[1].metadataChanges == 1 &&
           detailed.Report().textures[1].uploadDimensionChanges == 1);
    assert(detailed.Report().textures[1].cacheHits == 1 && detailed.Report().textures[1].cacheMisses == 1);
    assert(detailed.Report().resources[0].upload.nanos == 5); // Outside a command has no list owner.
    detailed.SetResource("<unresolved-marker>");
    detailed.AddCommand(4, 5, "G_TRI1", 10);
    assert(detailed.Report().resources[0].commands.nanos == 10 &&
           detailed.Report().resources[0].selfUnknownCommands == 1);
    assert(detailed.Report().unknownResourceSelections == 1);
    detailed.SetResource("parent");
    detailed.BeginCommand();
    detailed.AddUpload(1, 20);
    detailed.AddCommand(4, 5, "G_TRI1", 10);
    assert(detailed.Report().selfAccountingClamps == 1);
    for (unsigned i = 0; i < 4101; ++i) {
        detailed.SelectTexture("texture/" + std::to_string(i));
        detailed.AddUpload(4, 1, 1, 1);
    }
    assert(detailed.Report().textures.size() == 4096);
    assert(detailed.Report().textureOverflow == 7 && detailed.Report().overflowTexture.upload.calls == 7);
    for (unsigned i = 0; i < Fast::RenderCostProbe::MaxPasses + 5; ++i) {
        detailed.SetPass(i);
        detailed.BeginCommand();
        detailed.AddCommand(4, 5, "G_TRI1", 1);
    }
    assert(detailed.Report().passes.size() == Fast::RenderCostProbe::MaxPasses);
    assert(detailed.Report().passOverflow > 0 && detailed.Report().overflowPass.commands.calls > 0);
    auto oldOperation = detailed.BeginOperation(Fast::RenderCostOperation::TextureImport, 300);
    auto oldTexture = std::make_unique<Fast::RenderTextureScope>(&detailed, "texture/old");
    detailed.EndFrame(400);
    detailed.BeginFrame(true, 500, true);
    detailed.EndOperation(Fast::RenderCostOperation::TextureImport, oldOperation, 600);
    oldTexture.reset();
    detailed.AddUpload(4, 1, 1, 1);
    assert(detailed.Report().textures.size() == 1 && detailed.Report().textures[0].bytes == 4);
    assert(detailed.Report().textureImport.calls == 0 && !detailed.Report().resourceOverflow);
    detailed.EndFrame(700);

    Fast::RenderCostProbe disabled;
    disabled.BeginFrame(false, 0);
    disabled.SelectTexture("ignored");
    disabled.DescribeTexture(1, 1, 1, 1, 1);
    disabled.NoteTextureCache(false);
    disabled.AddUpload(4, 1, 1, 1);
    assert(disabled.Report().textures.empty() && disabled.Report().upload.calls == 0);
    std::cout << "Render cost probe: cadence, attribution, bounds, reset and accounting passed\n";
}
