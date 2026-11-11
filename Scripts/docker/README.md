# Container build (stub)

Nothing here has been run yet. It is the plan for building the Linux package on a GPU server.

1. Link your GitHub account to your Epic Games account: https://www.unrealengine.com/en-US/ue-on-github
2. Create a GitHub token with `read:packages` and run `docker login ghcr.io` (keep the token in your own secret store; never commit it).
3. From the repo root:

   ```bash
   docker compose -f Scripts/docker/compose.yaml --profile manual build mars-build
   ```

4. To pull the packaged game out of the build image:

   ```bash
   id=$(docker create mars-hello-world-ue5:build)
   docker cp "$id":/tmp/project/Build/Linux ./Build/
   docker rm "$id"
   ```

Disk: the dev image is very large (tens of GB). Check free space before pulling.
