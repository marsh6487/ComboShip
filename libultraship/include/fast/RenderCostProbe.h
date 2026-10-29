#pragma once

#include "fast/RenderResourceLookup.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

namespace Fast {

struct RenderCostCounter {
    uint64_t calls = 0;
    uint64_t nanos = 0;
    void Add(uint64_t elapsed) {
        ++calls;
        nanos += elapsed;
    }
};

struct RenderOpcodeCost : RenderCostCounter {
    const char* name = nullptr; // static opcode-table string, never a game resource
};

struct RenderResourceCost {
    std::string path;
    RenderCostCounter commands;
    RenderCostCounter self{}, textureImport{}, driverDraw{}, shaderCreate{}, upload{};
    uint64_t selfUnknownCommands = 0;
};

struct RenderTextureCost {
    std::string path;
    RenderCostCounter upload;
    uint64_t bytes = 0;
    RenderCostCounter imports{};
    uint64_t cacheHits = 0, cacheMisses = 0;
    uint32_t width = 0, height = 0, format = 0, flags = 0;
    uint64_t sourceDataBytes = 0, sourceBackingBytes = 0;
    bool sourceBackingKnown = false;
    bool metadataKnown = false;
    uint64_t metadataChanges = 0;
    uint32_t uploadWidth = 0, uploadHeight = 0;
    uint64_t uploadDimensionChanges = 0, uploadDimensionsUnknown = 0;
};

struct RenderPassCost : RenderResourceCost {
    int64_t framebuffer = -1;
};

enum class RenderCostOperation { TextureImport, DriverDraw, ShaderCreate };
inline uint64_t RenderCostNow();

struct RenderCostReport {
    bool sampled = false;
    bool altRenderLookup = false;
    RenderResourceLookupStats vertexLookups, displayListLookups;
    uint64_t frameIndex = 0;
    uint64_t attemptId = 0;
    uint32_t gameTick = 0;
    int interpolationIndex = 0;
    float interpolationT = 1.0f;
    uint64_t totalNanos = 0;
    std::array<RenderOpcodeCost, 6 * 256> opcodes{};
    std::vector<RenderResourceCost> resources;
    std::vector<RenderTextureCost> textures;
    std::vector<RenderPassCost> passes;
    RenderResourceCost overflowResource{ "<resource-limit-overflow>", {} };
    RenderTextureCost overflowTexture{ "<texture-limit-overflow>", {}, 0 };
    RenderPassCost overflowPass;
    uint64_t resourceOverflow = 0;
    uint64_t textureOverflow = 0, passOverflow = 0, unknownResourceSelections = 0, unknownTextureSelections = 0;
    uint64_t invalidOpcodes = 0, selfAccountingClamps = 0;
    bool operationAttribution = false, passAttribution = false, selfAttribution = false;
    RenderCostCounter textureImport, upload, driverDraw, shaderCreate, framebufferSetup, framebufferFinish;
    uint64_t cacheHits = 0, cacheMisses = 0, cacheEvictions = 0;
    uint64_t uploadBytes = 0, vertices = 0, triangles = 0, submittedTriangles = 0;
    // Cumulative since interpreter initialization, including changes between draws.
    uint64_t cacheClearsTotal = 0, cacheDeletesTotal = 0;
    uint64_t cacheEntries = 0;
    std::string backend;
    uint32_t renderWidth = 0, renderHeight = 0, windowWidth = 0, windowHeight = 0, msaa = 0;
};

// Records one of every 31 requested frames, or every requested detailed frame. No resource ownership, cache
// policy, graphics state or command data is changed. Explicit timestamps keep
// sampling and attribution independently testable without a ROM or graphics API.
class RenderCostProbe {
  public:
    // Prime cadence avoids locking onto one phase of the usual 2/3/4/6/8
    // interpolated frames per game tick. Reports also carry their exact phase.
    static constexpr unsigned SampleEvery = 31;
    static constexpr unsigned MaxResources = 4096;
    static constexpr unsigned MaxTextures = 4096;
    static constexpr unsigned MaxPasses = 128;
    static constexpr size_t OverflowIndex = std::numeric_limits<size_t>::max();

    struct OperationToken {
        size_t owner = 0, pass = 0;
        uint64_t start = 0, frame = 0, command = 0;
        bool valid = false, outermost = false;
    };

    bool BeginFrame(bool enabled, uint64_t now, bool everyFrame = false) {
        active = enabled && ((++requestedFrames - 1) % SampleEvery == 0 || everyFrame);
        report.sampled = active;
        if (!active) {
            return false;
        }
        report = {};
        report.sampled = true;
        report.frameIndex = requestedFrames;
        report.resources.push_back({ "<unmarked-or-unresolved>", {} });
        report.textures.push_back({ "<native-or-unresolved-texture>", {}, 0 });
        report.passes.push_back({});
        report.passes.back().path = "<unknown-framebuffer>";
        report.overflowPass.path = "<framebuffer-limit-overflow>";
        resourceIndex.clear();
        textureIndex.clear();
        passIndex.clear();
        ownerStack.clear();
        currentOwner = 0;
        pendingAddress = nullptr;
        pendingResource.clear();
        currentTexture = 0;
        currentPass = 0;
        commandActive = false;
        commandExcluded = operationDepth = commandSerial = 0;
        frameStart = now;
        return true;
    }

    void EndFrame(uint64_t now) {
        if (active) {
            report.totalNanos = now >= frameStart ? now - frameStart : 0;
        }
        active = false;
    }

    bool Active() const {
        return active;
    }

    void PrepareResource(const void* address, const std::string& path) {
        if (active && address) { pendingAddress = address; pendingResource = path; }
    }
    void EnterCommand(const void* address) {
        if (pendingAddress == address && pendingAddress != nullptr) SetResource(pendingResource);
        pendingAddress = nullptr;
    }
    void SyncDepth(size_t depth) {
        if (!active) {
            return;
        }
        // A newly called/branched list starts unmarked. Shrinking restores the
        // caller's owner, including returns that discard branch sentinels.
        ownerStack.resize(depth, 0);
        currentOwner = depth == 0 ? 0 : ownerStack.back();
    }

    void SetResource(const std::string& path) {
        if (!active || ownerStack.empty()) {
            return;
        }
        if (path.empty() || path.front() == '<') {
            ++report.unknownResourceSelections;
            currentOwner = 0;
            ownerStack.back() = currentOwner;
            return;
        }
        auto found = resourceIndex.find(path);
        if (found != resourceIndex.end()) {
            currentOwner = found->second;
        } else if (report.resources.size() < MaxResources) {
            currentOwner = report.resources.size();
            resourceIndex.emplace(path, currentOwner);
            report.resources.push_back({ path, {} });
        } else {
            ++report.resourceOverflow;
            currentOwner = OverflowIndex;
        }
        ownerStack.back() = currentOwner;
    }

    void SetPass(int64_t framebuffer) {
        if (!active) return;
        report.passAttribution = true;
        auto found = passIndex.find(framebuffer);
        if (found != passIndex.end()) currentPass = found->second;
        else if (report.passes.size() < MaxPasses) {
            currentPass = report.passes.size();
            passIndex.emplace(framebuffer, currentPass);
            report.passes.push_back({});
            report.passes.back().framebuffer = framebuffer;
            report.passes.back().path = "framebuffer/" + std::to_string(framebuffer);
        } else {
            ++report.passOverflow;
            currentPass = OverflowIndex;
        }
    }

    void BeginCommand() {
        if (!active) return;
        report.selfAttribution = true;
        commandActive = true;
        commandExcluded = 0;
        commandOwner = currentOwner;
        commandPass = currentPass;
        ++commandSerial;
    }

    void AddCommand(uint8_t ucode, uint8_t opcode, const char* name, uint64_t nanos) {
        if (!active) {
            return;
        }
        if (ucode < 6) {
            auto& cost = report.opcodes[static_cast<size_t>(ucode) * 256 + opcode];
            cost.name = name;
            cost.Add(nanos);
        } else ++report.invalidOpcodes;
        auto& resource = Resource(commandActive ? commandOwner : currentOwner);
        auto& pass = Pass(commandActive ? commandPass : currentPass);
        resource.commands.Add(nanos);
        pass.commands.Add(nanos);
        if (commandActive && operationDepth == 0) {
            if (commandExcluded > nanos) ++report.selfAccountingClamps;
            const auto self = nanos >= commandExcluded ? nanos - commandExcluded : 0;
            resource.self.Add(self);
            pass.self.Add(self);
        } else {
            ++resource.selfUnknownCommands;
            ++pass.selfUnknownCommands;
        }
        commandActive = false;
    }

    size_t SelectTexture(const std::string& path) {
        const size_t previous = currentTexture;
        if (!active) return previous;
        if (path.empty() || path.front() == '<') {
            ++report.unknownTextureSelections;
            currentTexture = 0;
            return previous;
        }
        auto found = textureIndex.find(path);
        if (found != textureIndex.end()) {
            currentTexture = found->second;
        } else if (report.textures.size() < MaxTextures) {
            currentTexture = report.textures.size();
            textureIndex.emplace(path, currentTexture);
            report.textures.push_back({ path, {}, 0 });
        } else {
            ++report.textureOverflow;
            currentTexture = OverflowIndex;
        }
        return previous;
    }

    void RestoreTexture(size_t previous) {
        if (active) currentTexture = previous;
    }

    void DescribeTexture(uint32_t width, uint32_t height, uint32_t format, uint32_t flags, uint64_t sourceBytes,
                         uint64_t backingBytes = 0, bool backingKnown = false) {
        if (!active) return;
        auto& texture = Texture(currentTexture);
        if (texture.metadataKnown && (texture.width != width || texture.height != height || texture.format != format ||
                                     texture.flags != flags || texture.sourceDataBytes != sourceBytes ||
                                     texture.sourceBackingKnown != backingKnown || texture.sourceBackingBytes != backingBytes))
            ++texture.metadataChanges;
        texture.width = width;
        texture.height = height;
        texture.format = format;
        texture.flags = flags;
        texture.sourceDataBytes = sourceBytes;
        texture.sourceBackingBytes = backingBytes;
        texture.sourceBackingKnown = backingKnown;
        texture.metadataKnown = true;
    }

    void NoteTextureCache(bool hit) {
        if (!active) return;
        auto& texture = Texture(currentTexture);
        if (hit) ++texture.cacheHits;
        else ++texture.cacheMisses;
    }

    void AddTextureImport(size_t texture, uint64_t frame, uint64_t nanos) {
        if (active && frame == report.frameIndex) Texture(texture).imports.Add(nanos);
    }
    size_t CurrentTexture() const { return currentTexture; }

    OperationToken BeginOperation(RenderCostOperation, uint64_t now) {
        if (!active) return {};
        report.operationAttribution = true;
        return {commandActive ? commandOwner : 0, currentPass, now, report.frameIndex, commandSerial, true, operationDepth++ == 0};
    }

    void EndOperation(RenderCostOperation operation, OperationToken token, uint64_t now) {
        if (!active || !token.valid || token.frame != report.frameIndex) return;
        const uint64_t nanos = now >= token.start ? now - token.start : 0;
        auto& resource = Resource(token.owner);
        auto& pass = Pass(token.pass);
        if (operation == RenderCostOperation::TextureImport) {
            report.textureImport.Add(nanos); resource.textureImport.Add(nanos); pass.textureImport.Add(nanos);
        } else if (operation == RenderCostOperation::DriverDraw) {
            report.driverDraw.Add(nanos); resource.driverDraw.Add(nanos); pass.driverDraw.Add(nanos);
        } else {
            report.shaderCreate.Add(nanos); resource.shaderCreate.Add(nanos); pass.shaderCreate.Add(nanos);
        }
        if (operationDepth) --operationDepth;
        // Only the outermost operation excludes elapsed time from self. Nested
        // import/upload/draw intervals overlap and must not be subtracted twice.
        if (token.outermost && commandActive && token.command == commandSerial) commandExcluded += nanos;
    }

    void AddUpload(uint64_t bytes, uint64_t nanos, uint32_t width = 0, uint32_t height = 0) {
        if (!active) {
            return;
        }
        report.upload.Add(nanos);
        report.uploadBytes += bytes;
        auto& texture = Texture(currentTexture);
        texture.upload.Add(nanos);
        texture.bytes += bytes;
        if (width && height) {
            if (texture.uploadWidth && (texture.uploadWidth != width || texture.uploadHeight != height))
                ++texture.uploadDimensionChanges;
            texture.uploadWidth = width;
            texture.uploadHeight = height;
        } else ++texture.uploadDimensionsUnknown;
        Resource(commandActive ? commandOwner : 0).upload.Add(nanos);
        Pass(currentPass).upload.Add(nanos);
        if (commandActive && operationDepth == 0) commandExcluded += nanos;
    }

    RenderCostReport& MutableReport() {
        return report;
    }
    const RenderCostReport& Report() const {
        return report;
    }

  private:
    RenderResourceCost& Resource(size_t index) {
        return index == OverflowIndex ? report.overflowResource : report.resources[index];
    }
    RenderPassCost& Pass(size_t index) {
        return index == OverflowIndex ? report.overflowPass : report.passes[index];
    }
    RenderTextureCost& Texture(size_t index) {
        return index == OverflowIndex ? report.overflowTexture : report.textures[index];
    }
    const void* pendingAddress = nullptr;
    std::string pendingResource;
    bool active = false;
    uint64_t requestedFrames = 0, frameStart = 0;
    size_t currentOwner = 0, currentTexture = 0;
    size_t currentPass = 0, commandOwner = 0, commandPass = 0;
    bool commandActive = false;
    uint64_t commandExcluded = 0, operationDepth = 0, commandSerial = 0;
    std::vector<size_t> ownerStack;
    std::unordered_map<std::string, size_t> resourceIndex;
    std::unordered_map<std::string, size_t> textureIndex;
    std::unordered_map<int64_t, size_t> passIndex;
    RenderCostReport report;
};

class RenderTextureScope {
  public:
    RenderTextureScope(RenderCostProbe* probe, const std::string& path)
        : probe(probe && probe->Active() ? probe : nullptr), previous(this->probe ? this->probe->SelectTexture(path) : 0),
          texture(this->probe ? this->probe->CurrentTexture() : 0),
          frame(this->probe ? this->probe->Report().frameIndex : 0), start(this->probe ? RenderCostNow() : 0) {
    }
    ~RenderTextureScope() {
        if (probe && probe->Active() && frame == probe->Report().frameIndex) {
            probe->AddTextureImport(texture, frame, RenderCostNow() - start);
            probe->RestoreTexture(previous);
        }
    }
    RenderTextureScope(const RenderTextureScope&) = delete;
    RenderTextureScope& operator=(const RenderTextureScope&) = delete;

  private:
    RenderCostProbe* probe;
    size_t previous;
    size_t texture;
    uint64_t frame, start;
};

class RenderOperationScope {
  public:
    RenderOperationScope(RenderCostProbe* probe, RenderCostOperation operation)
        : probe(probe && probe->Active() ? probe : nullptr), operation(operation),
          token(this->probe ? this->probe->BeginOperation(operation, RenderCostNow()) : RenderCostProbe::OperationToken{}) {}
    ~RenderOperationScope() {
        if (probe) probe->EndOperation(operation, token, RenderCostNow());
    }
    RenderOperationScope(const RenderOperationScope&) = delete;
    RenderOperationScope& operator=(const RenderOperationScope&) = delete;
  private:
    RenderCostProbe* probe;
    RenderCostOperation operation;
    RenderCostProbe::OperationToken token;
};

inline uint64_t RenderCostNow() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count());
}

// Driver/import scopes are inclusive and can overlap command timings. A null
// counter performs no clock calls on normal, unsampled frames.
class RenderCostScope {
  public:
    explicit RenderCostScope(RenderCostCounter* target) : target(target), start(target ? RenderCostNow() : 0) {
    }
    ~RenderCostScope() {
        if (target) {
            target->Add(RenderCostNow() - start);
        }
    }
    RenderCostScope(const RenderCostScope&) = delete;
    RenderCostScope& operator=(const RenderCostScope&) = delete;

  private:
    RenderCostCounter* target;
    uint64_t start;
};
} // namespace Fast
