# AGENTS.md

- Project: CPU path tracer in C++20, built with CMake. Scene: axis-aligned cube (Lambertian) at origin + thin square frame (4 thin boxes, ~2×2 ring) at `z ≈ -3` emitting red (HDR); camera orbits the scene over time.
- Everything is computed on CPU by design — no GPU shaders/hardware acceleration. Frame-rate drops at high `--spp`/`--res` are expected, not bugs.
- SDL2 is an optional dependency: if `find_package(SDL2)` fails, the renderer must fall back to writing PPM frames to `frames/`. Don't assume a window will open.
- Build: out-of-source `build/` (never commit it).
  - Configure: `cmake -B build` (this machine: use the README "Local toolchain" line — local CMake/Ninja + `C:/msys64/ucrt64/bin/g++.exe`)
  - Build: `cmake --build build -j`
  - Test: `ctest --test-dir build` (single test: `ctest --test-dir build -R <name>`)
- Layout: `src/math` (Vec3/Ray), `src/scene` (geometry, materials, camera), `src/render` (tracer, display), `tests/`, `src/main.cpp` (CLI + animation loop).
- Radiance is HDR through the tracer (emission > 1.0); tone-map/clamp only at display or PPM write time.
- Tracer correctness relies on next-event estimation sampling the frame directly — a thin frame is rarely hit by random bounces; don't remove NEE.
- Before declaring the renderer correct, run the full ctest suite plus a PPM smoke check: frame pixels are bright red, the cube face facing the frame is lit, the far side is in shadow. Recipe: run `pathtracer --width 320 --height 320 --spp 16 --frames 0 --out <dir>`, let it render ~10s until the camera is behind the frame (t > 9s), then `smoke_stats <last frame>` — expect brightRed in the thousands, `cube front` lit and red-dominant, both background boxes black.
- Keep this file updated as the build system, compiler flags, and test setup are finalized.

# Task Distribution Matrix (Routing Contract)

You are the lead coordinating agent (Plan/Build mode) powered by Qwen. Your task is to decompose complex requirements and assign highly specialized sub-tasks to sub-agents.

## Available sub-agents:
1. `@tester` — Use this agent ALWAYS when the main code is written and requires test coverage or regression testing.
2. `@scout` — Built-in agent. Call this agent if you need to analyze external documentation or cached dependencies.

## Invocation instructions:
If the task is complex:
1. Create a high-level plan.
2. Delegate test writing to the sub-agent by sending a command in this format: `call @tester to generate tests for src/auth.py`.
3. Wait for the sub-agent's response, analyze the result, and conclude the session.

## Status (as of 2026-10-05, after PPM smoke check)

- Repo at https://github.com/pinknyashka/pathtracing (public; first commit pushed, follow-up commit pending push); local `origin` = that URL.
  - Push with a token that has `public_repo`/`repo` scope (a read-only token gets 403): `git -c credential.helper= push https://x-access-token:<TOKEN>@github.com/pinknyashka/pathtracing.git main` — the `credential.helper=` override stops the token being saved to Windows Credential Manager.
- Toolchain blocker RESOLVED offline (no msys2 package changes): `D:\Projects\__tools\mingw_7_2_0` on the machine PATH shadowed ucrt64's runtime DLLs, so gcc-15 frontends crashed at startup with `0xC00000FD`. Fix: User PATH now starts with `C:\msys64\ucrt64\bin`, plus 8 DLLs staged next to cc1/cc1plus. Full report + prevention runbook: `docs/toolchain-incident-2026-10-05.md`.
- NEE self-blocking fixed in `src/render/tracer.h`: the visibility test now skips emissive boxes (it used to block on the sampled bar's own front face), and the emission contribution uses `cosEmit/d²`. Verified with a 1024-spp probe: lit.x=0.0503, shadow=0.0, framePix=(4.0, 0.18, 0.12); `test_tracer` expects lit.x in 0.035–0.065.
- CLI parser fixed: `parseArgs` called its `value()` lambda twice (each call bumps the arg index, so every option value was skipped — `--width 320` parsed as 0). Parser moved to `src/cli_args.h`, `--accumulate` branch restored, covered by `tests/test_args.cpp`.
- ctest 6/6 pass: `test_vec3`, `test_box`, `test_camera`, `test_light`, `test_tracer`, `test_args`.
- PPM smoke check PASS (320x320, 16 spp, camera behind the frame at t≈9.6s, analyzed with `build/smoke_stats.exe`): 2298 bright-red frame px; the cube face facing the frame is lit and red-dominant (maxR≈13 = tone-mapped lit.x≈0.05; ~65% of face px lit, the rest in bar shadow); top face + background black.
- OUTSTANDING: push the follow-up commit (needs VPN + a `public_repo` token).

## Toolchain notes (this machine)

- Compiler: msys2 ucrt64 GCC 15.2.0 at `C:\msys64\ucrt64\bin\g++.exe`; CMake/Ninja are local at `D:\Projects\__tools\{cmake,ninja}` (configure line in README).
- **PATH pitfall**: the machine PATH contains `D:\Projects\__tools\mingw_7_2_0\mingw64\bin` (2017-era libgmp/libmpfr/libmpc/libwinpthread + binutils). If `C:\msys64\ucrt64\bin` is not ahead of it on PATH, cc1/cc1plus load the 2017 DLLs and crash at startup with `0xC00000FD`, no diagnostics. `C:\msys64\ucrt64\bin` is first in the **User** PATH (set 2026-10-05).
- **Stale-session gotcha**: processes started before a PATH change (including this CLI) keep the old PATH. In shell commands, re-derive it first: `$u=[Environment]::GetEnvironmentVariable("Path","User"); $m=[Environment]::GetEnvironmentVariable("Path","Machine"); $env:PATH="$u;$m"`.
- Backstop: the frontends' own directory (`C:\msys64\ucrt64\lib\gcc\x86_64-w64-mingw32\15.2.0\`) holds staged copies of libwinpthread-1, libgcc_s_seh-1, libgmp-10, libmpfr-6, libmpc-3, libisl-23, zlib1, libzstd. After any msys2 gcc update (new version dir), re-stage them — see `docs/toolchain-incident-2026-10-05.md` section 6.
- No VPN → msys2 mirrors and GitHub push are unreachable; never plan a fix that requires `pacman -Sy` or a push.
