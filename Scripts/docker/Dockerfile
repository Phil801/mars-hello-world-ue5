# Mars Hello World - containerised Linux build (STUB, not yet run).
#
# The official Unreal Engine container images live at ghcr.io/epicgames/unreal-engine and are PRIVATE:
# you need a GitHub account linked to an Epic Games account (https://www.unrealengine.com/en-US/ue-on-github)
# and a GitHub personal access token with read:packages to `docker login ghcr.io`.
# Images are large (dev image ~ 40+ GB). Pick the tag that matches the engine you target, e.g.
#   ghcr.io/epicgames/unreal-engine:dev-5.8   (full editor + tools; needed to build/cook)
#   ghcr.io/epicgames/unreal-engine:runtime   (slim runtime base for running a packaged Linux build)
ARG UE_TAG=dev-5.8
FROM ghcr.io/epicgames/unreal-engine:${UE_TAG} AS builder

# The dev images run as user "ue4" with the engine at /home/ue4/UnrealEngine.
COPY --chown=ue4:ue4 . /tmp/project
WORKDIR /tmp/project
RUN bash Scripts/build_linux.sh /home/ue4/UnrealEngine

# Optional slim image that just holds the packaged game (headless/offscreen run needs a GPU + Vulkan).
FROM ghcr.io/epicgames/unreal-engine:runtime AS game
COPY --from=builder --chown=ue4:ue4 /tmp/project/Build/Linux/Linux /home/ue4/project
CMD ["/home/ue4/project/MarsHelloWorld.sh", "-RenderOffscreen"]
